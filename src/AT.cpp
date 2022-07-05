/*
 * ----------------------------------------------------------------------------
 *                        _ _           _
 *                       (_) |         | |
 *                        _| |     __ _| |__  ___
 *                       | | |    / _` | '_ \/ __|
 *                       | | |___| (_| | |_) \__ \
 *                       |_|______\__,_|_.__/|___/
 *
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <pontus@ilabs.se> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return - Pontus Oldberg
 * ----------------------------------------------------------------------------
 */
// Enable if you need debug print outs on the serial terminal.
#define DEBUG     1

#include <debug.h>
#include <AT.h>
#include <EspATMQTT.h>

static const char *STR_OK         = "OK";
static const char *STR_ERROR      = "ERROR";
static const char *STR_ERR_CODE   = "ERR CODE:";
AT_Class::AT_Class(HardwareSerial* serial) {
   _serial = serial;
}

//------------------------------------------------------------------------------
/*
 * Read exactly one line from the serial port.
 * Each line that is read is appended to the internal buffer to create a
 * internal processable response. CR and LF's are removed and replaced with
 * a '|' character to separate the lines in the buffer. This can then later
 * be process using strok or a similar function.
 */
size_t AT_Class::readLine() {
  size_t cnt = 0;
  char ch = 0;

  do {
    ch = read();
    if (ch >= 0 && ch != '\r' && ch != '\n') {
      buff[wx++] = ch;
      cnt++;
    }
  } while (ch != '\n' && ch != -1);

  buff[wx++] = '|';
  buff[wx] = '\0';
  cnt += 2;
  line++;

  return cnt;
}

//------------------------------------------------------------------------------
/*
 * Read all lines until the end of the AT command sequence
 * All lines are stored in buff[] separated with a | character so you can use
 * strtok later on.
 * The asynch parameter can be used to ensure that a message that is
 * asynchronous to the OK or ERROR response is always received within the
 * given timeout value. If not, a ESP_AT_SUB_CMD_TIMEOUT error will be reported
 * back to the caller.
 * Important note: the asynchronous message could possibly still come in after
 * the timeout has elapsed in case of unstable or extremely slow networks.
 * In this case it is important to make sure it is taken care of in the mqtt
 * process() function.
 */
at_status_t AT_Class::waitReply(const char *asynch, uint32_t timeout) {
  size_t tx = 0;
  at_status_t errno = ESP_AT_SUB_OK;
  bool asynchFound = false;
  bool err = false;

  wx = 0;
  line = 0;
  do {
    tx = wx;
    readLine();
    if (strstr(&buff[tx], STR_ERR_CODE)) {
      errno = strtol(&buff[tx+9], NULL, 16);
      dprintf("Error code %08x detected\n", errno);
    }
    if (asynch && strstr(&buff[tx], asynch)) {
      asynchFound = true;
    }
  } while (!strstr(&buff[tx], STR_OK) && !strstr(&buff[tx], STR_ERROR));
  dprintf("Read line %d \"%s\"\r\n", line, &buff[0]);

  if (strstr(&buff[tx], STR_ERROR))
    err = true;

  if (asynch && !asynchFound) {
    // Asynchronous marker not found, need to wait for it.
    uint32_t to = millis();
    while (!_serial->available() && (millis() - to < timeout))
      yield();
    if (!_serial->available())
      return ESP_AT_SUB_CMD_TIMEOUT;
    wx = 0;
    line = 0;
    readLine();
    dprintf("Read line %d \"%s\"\r\n", line, &buff[0]);
    wx++;
  }

  if (err) {
    // An error was detected and we should return here.
    // In case SYSLOG was enabled the errno variable will hold the error
    // message, if not we just return it as a common error.
    if (errno) {
      return errno;
    } else {
      return ESP_AT_SUB_COMMON_ERROR;
    }
  }

  // We do not want the last string separator character.
  buff[--wx] = '\0';

  return ESP_AT_SUB_OK;
}


//------------------------------------------------------------------------------
at_status_t AT_Class::sendCommand(const char *cmd, const char *param,
                                    char **result, const char *asynch,
                                    uint32_t timeout) {
  at_status_t res;

  sprintf(cmdBuff, "AT%s%s", cmd, param);
  _serial->println(cmdBuff);
  dprintf("S:\'%s\'\n", cmdBuff);

  uint32_t to = millis();
  while (!_serial->available() && (millis() - to < timeout))
    yield();
  if (!_serial->available())
    return ESP_AT_SUB_CMD_TIMEOUT;

  res = waitReply(asynch, timeout);
  if (res != ESP_AT_SUB_OK)
    return res;

  int i = 0;
  if (result) {
    char *ptr;
    // The result is available in the first entry of the returned string
    if (!asynch)
      ptr = strstr(&buff[0], cmd) + strlen(cmd) + 1;
    else
      ptr = &buff[0];
    while (*ptr != '|' && *ptr != '\n') {
      resBuff[i++] = *ptr++;
    }
    resBuff[i] = '\0';
    *result = resBuff;
  }

  return ESP_AT_SUB_OK;
}


//------------------------------------------------------------------------------
at_status_t AT_Class::waitPrompt(uint32_t timeout) {
  char ch;
  //_serial->setTimeout(timeout);
  do {
    if (_serial->available()) {
      ch = _serial->read();
    }
  } while (ch != '>');

  return ESP_AT_SUB_OK;
}


//------------------------------------------------------------------------------
at_status_t AT_Class::waitString(const char *str, uint32_t timeout) {
  size_t tx = 0;

  wx = 0;
  line = 0;
  do {
    tx = wx;
    readLine();
    dprintf("L:\'%s\'\n", &buff[tx]);
  } while (!strstr(&buff[tx], str));
  return ESP_AT_SUB_OK;
}


//------------------------------------------------------------------------------
at_status_t AT_Class::sendString(const char *str, size_t len) {
  _serial->write(str, len);
  return ESP_AT_SUB_OK;
}


//------------------------------------------------------------------------------
at_status_t AT_Class::sendString(char *str, size_t len) {
  _serial->write(str, len);
  return ESP_AT_SUB_OK;
}


//------------------------------------------------------------------------------
at_status_t AT_Class::sendString(const char *str) {
  _serial->write(str, strlen(str));
  return ESP_AT_SUB_OK;
}


//------------------------------------------------------------------------------
char *AT_Class::getBuff() {
  return buff;
}


//------------------------------------------------------------------------------
char AT_Class::read() {
  char ch;

  do {
    ch = _serial->read();
  } while (ch == 255);

  return ch;
}

//------------------------------------------------------------------------------
int AT_Class::available() {
  return _serial->available();
}

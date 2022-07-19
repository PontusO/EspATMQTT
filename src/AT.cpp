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
  Copyright (c) 2022 iLabs - Pontus Oldberg

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
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
static const char *STR_BUSY       = "busy p...";

/*******************************************************************************
 *
 * The class constructor is used to set the serial port to be used to
 * communicate with the ESP-AT device.
 *
 * @param[in] - serial The serial port that the ESP device is connected to.
 *
 ******************************************************************************/
AT_Class::AT_Class(HardwareSerial* serial) {
   _serial = serial;
}

/*******************************************************************************
 *
 * Read exactly one line from the serial port.
 * Each line that is read is appended to the internal buffer to create a
 * internal processable response. CR and LF's are removed and replaced with
 * a '|' character to separate the lines in the buffer. This can then later
 * be process using strtok or a similar function.
 *
 * @return The number of characters read from the serial port.
 *
 ******************************************************************************/
size_t AT_Class::readLine(uint32_t timeout) {
  size_t cnt = 0;
  char ch = 0;
  uint32_t to = millis();

  do {
    ch = read();
    if (ch >= 0 && ch != '\r' && ch != '\n') {
      buff[wx++] = ch;
      cnt++;
    }
  } while (ch != '\n' && ch != -1 && (millis() - to < timeout));

  if (millis() - to >= timeout)
    return 0;

  buff[wx++] = '|';
  buff[wx] = '\0';
  cnt += 2;
  line++;

  return cnt;
}

/*******************************************************************************
 *
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
 *
 * @param[in] - asynch
 *          Any asynchronous data that is required. Can be NULL if
 *          no asynchronous data is expected.
 * @param[in] - timeout
 *          The maximum amount of time (in milliseconds) that is allowed
 *          before a response is expected. If data has not arrived within the
 *          alloted time the function will return with a timeout error message.
 *
 * @return - The status of the operation, See #status_code_e for more
 *           information.
 *
 ******************************************************************************/
at_status_t AT_Class::waitReply(const char *asynch, uint32_t timeout) {
  size_t tx = 0;
  at_status_t errno = ESP_AT_SUB_OK;
  bool asynchFound = false;
  bool err = false;

  wx = 0;
  line = 0;
  do {
    tx = wx;
    if (!readLine(timeout))
      return ESP_AT_SUB_CMD_TIMEOUT;

    if (strstr(&buff[tx], STR_BUSY)) {
      // So the ESP-AT interpreter is still busy executing the previously
      // send command. This means we need to wait and retry
      errno = ESP_AT_SUB_CMD_RETRY;
      err = true;
    }
    if (strstr(&buff[tx], STR_ERR_CODE)) {
      err = true;
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


/*******************************************************************************
 *
 * This function sends the specified AT command and parameters to the ESP-AT
 * device on the serial line. The function adds the "AT" part of the command
 * so this must not be a part of the command string when calling the function.
 *
 * It then waits for the AT response to come back from the ESP-AT device and
 * returns with the result of the operation. It also detects if the ESP-AT
 * device does not return a reply within the specified time limit.
 *
 * @param[in] - cmd
 *          The AT command that should be executed (without the AT part).
 *          No <CR> or <LF> characters must be present in this string.
 *          Example: "+GMR" (Returns the firmware version)
 * @param[in] - param
 *          This is the parameter part of the AT command. It should hold
 *          all the necessary and relevant parameters of the AT command.
 *          No <CR> or <LF> characters must be present in this string.
 *          Example "=0,1,"alpn.ilabs.se""
 * @param[out] - result
 *          When an AT command returns a parameter as a result of the operation
 *          the AT handler takes care of this and can be returned to the caller.
 *          By supplying a non NULL ptrptr the caller can get access to this
 *          parameter. If this parameter is NULL the result is simply discarded.
 * @param[in] - asynch
 *          Some AT commands return their result completely asynchronous to the
 *          OK or ERROR reply and also with a completely different URC tag.
 *          For instance the command "AT+MQTTCONN=" can return the response
 *          "+MQTTCONNECTED:" both before and after the OK response.
 *          In this case by setting this parameter to "+MQTTCONNECTED:" the
 *          function will look for this data both when waiting for the OK
 *          response as well as after the OK response has been received or a
 *          timeout has occured.
 * @param[in] - timeout
 *          The amount of time, in milliseconds, that the function will wait
 *          for a reply from the ESP-AT device. This value is also used when
 *          waiting for an asynchronous response.
 *
 * @return - The status of the operation, See #status_code_e for more
 *           information.
 *
 ******************************************************************************/
at_status_t AT_Class::sendCommand(const char *cmd, const char *param,
                                    char **result, const char *asynch,
                                    uint32_t timeout) {
  at_status_t res;
  uint32_t to = millis();

  snprintf(cmdBuff, ESP_AT_CMDBUFF_LENGTH, "AT%s%s", cmd, param);
  do {
    dprintf("S:\'%s\'\n", cmdBuff);
    _serial->println(cmdBuff);

    while (!_serial->available() && (millis() - to < timeout))
      yield();
    if (!_serial->available())
      return ESP_AT_SUB_CMD_TIMEOUT;

    res = waitReply(asynch, timeout);
    if (res == ESP_AT_SUB_CMD_RETRY) {
      dprintf("Retrying last command !\n", NULL);
      delay(250); // Make sure we have a nice little delay before retrying
    }
  } while (res == ESP_AT_SUB_CMD_RETRY && (millis() - to < timeout));
  if ((millis() - to) >= timeout)
    return ESP_AT_SUB_CMD_TIMEOUT;
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

/*******************************************************************************
 *
 * Waits for a prompt of the character '>' to arrive on the serial port.
 *
 * @param[in] - timeout The time allowed, in millisecond, for the prompt to arrive
 *
 * @return - The status of the operation, See #status_code_e for more
 *           information.
 *
 ******************************************************************************/
at_status_t AT_Class::waitPrompt(uint32_t timeout) {
  char ch;
  uint32_t to = millis();

  while (!_serial->available() && ((millis() - to) < timeout));
  if (!_serial->available())
    return ESP_AT_SUB_CMD_TIMEOUT;

  ch = _serial->read();
  if (ch != '>')
    return ESP_AT_SUB_CMD_ERROR;

  return ESP_AT_SUB_OK;
}

/*******************************************************************************
 *
 * Wait for a specific string to arrive on the serial port.
 *
 * @param[in] - str
 *           Holds the string that we are looking for.
 * @param[in] - timeout
 *           The maximum time allowed to wait for the string to arrive.
 *
 * @return - The status of the operation, See #status_code_e for more
 *           information.
 *
 ******************************************************************************/
at_status_t AT_Class::waitString(const char *str, uint32_t timeout) {
  size_t tx = 0;
  uint32_t to = millis();

  // First make sure there are characters in the buffer and that it did not
  // take to long for them to arrive.
  while (!_serial->available() && ((millis() - to) < timeout));
  if (!_serial->available())
    return ESP_AT_SUB_CMD_TIMEOUT;

  wx = 0;
  line = 0;
  do {
    tx = wx;
    readLine();
    dprintf("L:\'%s\'\n", &buff[tx]);
  } while (!strstr(&buff[tx], str) && ((millis() - to) < timeout));

  if ((millis() - to) >= timeout)
    return ESP_AT_SUB_CMD_TIMEOUT;
  return ESP_AT_SUB_OK;
}

/*******************************************************************************
 *
 * Send a generic string on the serial port. Can be used to send anything
 * to the connected ESP-AT device.
 *
 * @param[in] - str (const char *)
 *           The string/data that we want to send. Does not need to be '\0'
 *           terminated.
 * @param[in] - len
 *           The length of the string/data that we want to send.
 *
 * @return - The status of the operation, See #status_code_e for more
 *           information.
 *
 ******************************************************************************/
at_status_t AT_Class::sendString(const char *str, size_t len) {
  _serial->write(str, len);
  return ESP_AT_SUB_OK;
}

/*******************************************************************************
 *
 * Send a generic string on the serial port. Can be used to send anything
 * to the connected ESP-AT device.
 *
 * @param[in] - str (char *)
 *           The string/data that we want to send. Does not need to be '\0'
 *           terminated.
 * @param[in] - len
 *           The length of the string/data that we want to send.
 *
 * @return - The status of the operation, See #status_code_e for more
 *           information.
 *
 ******************************************************************************/
at_status_t AT_Class::sendString(char *str, size_t len) {
  _serial->write(str, len);
  return ESP_AT_SUB_OK;
}

/*******************************************************************************
 *
 * Send a generic string on the serial port. Can be used to send anything
 * to the connected ESP-AT device.
 *
 * @param[in] - str (const char *)
 *           The string/data that we want to send. Must be '\0' terminated.
 *
 * @return - The status of the operation, See #status_code_e for more
 *           information.
 *
 ******************************************************************************/
at_status_t AT_Class::sendString(const char *str) {
  _serial->write(str, strlen(str));
  return ESP_AT_SUB_OK;
}

/*******************************************************************************
 *
 * Returns a pointer to the internal receive buffer. Can be used by the caller
 * to directly access the recevied data string from the ESP-AT device.
 *
 * @return - A pointer to the internal data receive buffer.
 *
 ******************************************************************************/
char *AT_Class::getBuff() {
  return buff;
}

/*******************************************************************************
 *
 * Reads a byte from the serial port. The function ensures that a valid byte
 * has been received from the ESP-AT device before returning the data.
 * If a timeout situation occurs, it will return 0xff.
 *
 * @return - The data byte received from the ESP-AT device.
 *
 ******************************************************************************/
char AT_Class::read(uint32_t timeout) {
  char ch;
  uint32_t to = millis();

  do {
    ch = _serial->read();
  } while (ch == 255 && (millis() - to < timeout));

  return ch;
}

/*******************************************************************************
 *
 * Writes a byte to the serial port.
 *
 ******************************************************************************/
void AT_Class::write(char ch) {
  _serial->write(ch);
}

/*******************************************************************************
 *
 * Checks to see if there are any data available on the serial port.
 *
 * @return - The number of bytes found available in the serial read buffer.
 *
 ******************************************************************************/
int AT_Class::available() {
  return _serial->available();
}

/*******************************************************************************
 *
 * Sets the serial port to be used in this class.
 *
 ******************************************************************************/
void AT_Class::setSerial(HardwareSerial* port) {
  _serial = port;
}

/*******************************************************************************
 *
 * Returns the current instance of the serial port. Can be used by friendly
 * classes.
 *
 * @return - The currently used hardware serialport
 *
 ******************************************************************************/
HardwareSerial* AT_Class::getSerial() {
  return _serial;
}

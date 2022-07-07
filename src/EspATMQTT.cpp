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
#include <EspATMQTT.h>
#include <AT.h>

const char *MQTT_RESP_SUBRECV           = "+MQTTSUBRECV:";
const char *MQTT_RESP_CONNECTED         = "+MQTTCONNECTED:";

const char *MQTT_CMD_USERCFG            = "+MQTTUSERCFG";
const char *MQTT_CMD_CLIENTID           = "+MQTTCLIENTID";
const char *MQTT_CMD_USERNAME           = "+MQTTUSERNAME";
const char *MQTT_CMD_PASSWORD           = "+MQTTPASSWORD";
const char *MQTT_CMD_CONNCFG            = "+MQTTCONNCFG";
const char *MQTT_CMD_ALPN               = "+MQTTALPN";
const char *MQTT_CMD_CONN               = "+MQTTCONN";
const char *MQTT_CMD_PUB                = "+MQTTPUB";
const char *MQTT_CMD_PUBRAW             = "+MQTTPUBRAW";
const char *MQTT_CMD_SUB                = "+MQTTSUB";
const char *MQTT_CMD_UNSUB              = "+MQTTUNSUB";
const char *MQTT_CMD_CLEAN              = "+MQTTCLEAN";

const char *MQTT_STRING_MQTTPUB         = "+MQTTPUB:";
const char *MQTT_STRING_CIPSNTPTIME     = "AT+CIPSNTPTIME?\r\n";

const char *AT_CMD_SYSLOG               = "+SYSLOG";
const char *AT_CMD_CIPSNTPCFG           = "+CIPSNTPCFG";
const char *AT_CMD_CIPSNTPTIME          = "+CIPSNTPTIME";

const char *AT_RESP_CIPSNTPTIME         = "+CIPSNTPTIME:";

/*******************************************************************************
 *
 * The module constructor allows you to select what serial port that should be
 * used to communicate with the ESP-AT module.
 *
 * @param - serial The serial port that is connected to the ESP-AT module.
 *
 ******************************************************************************/
EspATMQTT::EspATMQTT(HardwareSerial* serial) {
  at = new AT_Class(serial);
}

/*******************************************************************************
 *
 * The begin method initializes the system before usage. It must be called
 * before running any other methods of this library.
 *
 * @return - The status of the operation, @see mqtt_status_t for more
 *           information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::begin() {
  char *strResult;

  topicSubscriptions = 0;
  connected = false;
  connected_cb = NULL;
  ntpTimeValid = false;
  connType = AT_CONN_UNCONNECTED;
  // First we need to make sure that SYSLOG has been enabled to get all the
  // error codes.
  while (at->sendCommand(AT_CMD_SYSLOG, "?", &strResult) != ESP_AT_SUB_OK)
    delay(100);

  // Check
  dprintf("Syslog String = %s\n", strResult);
  int result = strtol(strResult, NULL, 10);
  dprintf("Syslog int = %d\n", result);
  if (!result) {
    // And set if not already set
    at->sendCommand(AT_CMD_SYSLOG, "=1", NULL);
  }

  // Make sure it got set
  at->sendCommand(AT_CMD_SYSLOG, "?", &strResult);
  dprintf("Syslog String = %s\n", strResult);
  result = strtol(strResult, NULL, 10);
  dprintf("Syslog int = %d\n", result);
  if (!result) {
    dprintf("Could not update SYSLOG, please check your system.", NULL);
    return ESP_AT_SUB_CMD_ERROR;
  }
  return ESP_AT_SUB_OK;
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::userConfig(uint32_t linkID, mqtt_scheme_t scheme,
                         const char *clientID, const char *userName,
                         const char *password, uint32_t certKeyID, uint32_t caID,
                         const char *path) {

 snprintf(buff, MQTT_BUFFER_SIZE, "=%d,%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\"", linkID,
         (uint32_t)scheme, clientID, userName, password, certKeyID, caID, path);

 return at->sendCommand(MQTT_CMD_USERCFG, buff, NULL);
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::userConfig(uint32_t linkID, mqtt_scheme_t scheme,
                         char *clientID, const char *userName,
                         const char *password, uint32_t certKeyID, uint32_t caID,
                         const char *path) {

 snprintf(buff, MQTT_BUFFER_SIZE, "=%d,%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\"", linkID,
         (uint32_t)scheme, clientID, userName, password, certKeyID, caID, path);

 return at->sendCommand(MQTT_CMD_USERCFG, buff, NULL);
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::userConfig(uint32_t linkID, mqtt_scheme_t scheme,
                         char *clientID, char *userName, char *password,
                         uint32_t certKeyID, uint32_t caID, const char *path) {

 snprintf(buff, MQTT_BUFFER_SIZE, "=%d,%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\"", linkID,
         (uint32_t)scheme, clientID, userName, password, certKeyID, caID, path);

 return at->sendCommand(MQTT_CMD_USERCFG, buff, NULL);
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::clientID(uint32_t linkID, const char *clientID) {

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, clientID);
  return at->sendCommand(MQTT_CMD_CLIENTID, buff, NULL);
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::clientID(uint32_t linkID, char *clientID) {

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, clientID);
  return at->sendCommand(MQTT_CMD_CLIENTID, buff, NULL);
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::username(uint32_t linkID, const char *username) {

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, username);
  return at->sendCommand(MQTT_CMD_USERNAME, buff, NULL);
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::username(uint32_t linkID, char *username) {

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, username);
  return at->sendCommand(MQTT_CMD_USERNAME, buff, NULL);
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::password(uint32_t linkID, const char *password) {

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, password);
  return at->sendCommand(MQTT_CMD_PASSWORD, buff, NULL);
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::password(uint32_t linkID, char *password) {

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, password);
  return at->sendCommand(MQTT_CMD_PASSWORD, buff, NULL);
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::connectionConfig(uint32_t linkID, uint32_t keepalive,
                         uint32_t disable_clean_session, const char* lwt_topic,
                         const char* lwt_message, uint32_t lwt_qos,
                         uint32_t lwt_retain) {

  // Some inital error checking
  if (strlen(lwt_topic) > 128)
    return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_TOPIC_IS_OVERLENGTH;
  if (strlen(lwt_message) > 64)
    return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_DATA_IS_OVERLENGTH;
  if (keepalive > 7200)
    return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_KEEPALIVE_VALUE_IS_WRONG;
  if (disable_clean_session > 1)
    return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_DISABLE_CLEAN_SESSION_VALUE_IS_WRONG;
  if (lwt_qos > 2)
    return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_LWT_QOS_VALUE_IS_WRONG;
  if (lwt_retain > 1)
    return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_LWT_RETAIN_VALUE_IS_WRONG;

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,%d,%d,\"%s\",\"%s\",%d,%d", linkID,
           keepalive, disable_clean_session, lwt_topic, lwt_message, lwt_qos, lwt_retain);
  return at->sendCommand(MQTT_CMD_CONNCFG, buff, NULL);
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::setALPN(uint32_t linkID, const char *alpn1,
                                       const char *alpn2, const char *alpn3,
                                       const char *alpn4, const char *alpn5) {
  int noAlpns = 5;
  if (!alpn5) noAlpns--;
  if (!alpn4) noAlpns--;
  if (!alpn3) noAlpns--;
  if (!alpn2) noAlpns--;
  if (!alpn1) noAlpns--;

  switch(noAlpns) {
    case 0:
      snprintf(buff, MQTT_BUFFER_SIZE, "=%d,0", linkID);
      break;
    case 1:
      snprintf(buff, MQTT_BUFFER_SIZE, "=%d,1,\"%s\"", linkID, alpn1);
      break;
    case 2:
      snprintf(buff, MQTT_BUFFER_SIZE, "=%d,2,\"%s\",\"%s\"",
               linkID, alpn1, alpn2);
      break;
    case 3:
      snprintf(buff, MQTT_BUFFER_SIZE, "=%d,3,\"%s\",\"%s\",\"%s\"",
               linkID, alpn1, alpn2, alpn3);
      break;
    case 4:
      snprintf(buff, MQTT_BUFFER_SIZE, "=%d,4,\"%s\",\"%s\",\"%s\",\"%s\"",
               linkID, alpn1, alpn2, alpn3, alpn4);
      break;
    case 5:
      snprintf(buff, MQTT_BUFFER_SIZE, "=%d,5,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"",
                     linkID, alpn1, alpn2, alpn3, alpn4, alpn5);
      break;
  }
  return at->sendCommand(MQTT_CMD_ALPN, buff, NULL);
}

//------------------------------------------------------------------------------
// Connect is one of the methods that has an asynchronus confirmation response
// i.e. the confirmation that the client is connected to the server comes
// after the "OK" response.
// Depending on the outcome of the connection the method returns wether it
// was synchronously connected = MQTT_RESP_CONNECTED (in which case it is
// connected when the function returns or if the connection is still pending
// in which case the caller can rely on the connection callback or just check
// isConnected().
// Increasing the timeout also increases the chance that the device connects
// synchronously but may cause unwanted delays.
//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::connect(uint32_t linkID, const char *host,
                                 uint32_t port, uint32_t reconnect,
                                 uint32_t timeout, connected_cb_t cb) {
  mqtt_status_t ret;
  char *result;

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\",%d,%d", linkID, host, port, reconnect);
  ret = at->sendCommand(MQTT_CMD_CONN, buff, &result, MQTT_RESP_CONNECTED, timeout);
  if (ret == ESP_AT_SUB_OK) {
    dprintf("Connection result: %s\n", result);
    if (strstr(result, MQTT_RESP_CONNECTED)) {
      connected = true;
      ret = ESP_AT_SUB_CMD_CONN_SYNCH;
    } else {
      connected_cb = cb;
      ret = ESP_AT_SUB_CMD_CONN_ASYNCH;
    }
  } else if (ret == ESP_AT_SUB_CMD_TIMEOUT) {
    connected_cb = cb;
    ret = ESP_AT_SUB_CMD_CONN_ASYNCH;
  }

  return ret;
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::pubString(uint32_t linkID, const char *topic,
                         const char *data, uint32_t qos, uint32_t retain) {
  if (connected) {
    snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\",\"%s\",%d,%d", linkID, topic, data, qos, retain);
    return at->sendCommand(MQTT_CMD_PUB, buff, NULL);
  }
  return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_IN_DISCONNECTED_STATE;
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::pubString(uint32_t linkID, const char *topic, char *data,
                         uint32_t qos, uint32_t retain) {
  if (connected) {
    snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\",\"%s\",%d,%d", linkID, topic, data, qos, retain);
    return at->sendCommand(MQTT_CMD_PUB, buff, NULL);
  }
  return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_IN_DISCONNECTED_STATE;
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::pubRaw(uint32_t linkID, const char *topic, const char *data,
                         uint32_t qos, uint32_t retain) {
  mqtt_status_t status;

  if (connected) {
    size_t len = strlen(data);

    snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\",%d,%d,%d", linkID, topic, len, qos, retain);
    status = at->sendCommand(MQTT_CMD_PUBRAW, buff, NULL);
    if (status != ESP_AT_SUB_OK) {
      return status;
    }
    at->waitPrompt(100);
    at->sendString(data, len);
    at->waitString(MQTT_STRING_MQTTPUB, 100);

    char *lBuff = at->getBuff();
    if (strstr(lBuff, "FAIL")) {
      return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_FAILED_TO_PUBLISH_RAW;
    }
    return ESP_AT_SUB_OK;
  }
  return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_IN_DISCONNECTED_STATE;
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::pubRaw(uint32_t linkID, const char *topic, char *data,
                         uint32_t qos, uint32_t retain) {
  mqtt_status_t status;

  if (connected) {
    size_t len = strlen(data);

    snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\",%d,%d,%d", linkID, topic, len, qos, retain);
    status = at->sendCommand(MQTT_CMD_PUBRAW, buff, NULL);
    if (status != ESP_AT_SUB_OK) {
      return status;
    }
    at->waitPrompt(100);
    at->sendString(data, len);
    at->waitString(MQTT_STRING_MQTTPUB, 100);

    char *lBuff = at->getBuff();
    if (strstr(lBuff, "FAIL")) {
      return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_FAILED_TO_PUBLISH_RAW;
    }
    return ESP_AT_SUB_OK;
  }
  return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_IN_DISCONNECTED_STATE;
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::subscribeTopic(subscription_cb_t cb, uint32_t linkID,
              const char * topic, uint32_t qos) {
  if (connected) {
    snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\",%d", linkID, topic, qos);
    subscription_cb = cb;
    topicSubscriptions++;
    return at->sendCommand(MQTT_CMD_SUB, buff, NULL);
  }
  return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_IN_DISCONNECTED_STATE;
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::subscribeTopic(subscription_cb_t cb, uint32_t linkID,
              char * topic, uint32_t qos) {
  if (connected) {
    snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\",%d", linkID, topic, qos);
    subscription_cb = cb;
    topicSubscriptions++;
    return at->sendCommand(MQTT_CMD_SUB, buff, NULL);
  }
  return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_IN_DISCONNECTED_STATE;
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::unSubscribeTopic(uint32_t linkID, const char * topic) {
  if (connected) {
    snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, topic);
    if (topicSubscriptions) {
      topicSubscriptions--;
      if (!topicSubscriptions)
        subscription_cb = NULL;
    }
    return at->sendCommand(MQTT_CMD_UNSUB, buff, NULL);
  }
  return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_IN_DISCONNECTED_STATE;
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::unSubscribeTopic(uint32_t linkID, char * topic) {
  if (connected) {
    snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, topic);
    if (topicSubscriptions) {
      topicSubscriptions--;
      if (!topicSubscriptions)
        subscription_cb = NULL;
    }
    return at->sendCommand(MQTT_CMD_UNSUB, buff, NULL);
  }
  return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_IN_DISCONNECTED_STATE;
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::close(uint32_t linkID) {
  if (connected) {
    snprintf(buff, MQTT_BUFFER_SIZE, "=%d", linkID);
    return at->sendCommand(MQTT_CMD_CLEAN, buff, NULL);
  }
  return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_IN_DISCONNECTED_STATE;
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::enableNTPTime(bool enable, validDateTime_cb_t cb,
                                       uint32_t timezone, const char *ts1,
                                       const char *ts2, const char *ts3) {
  if (!enable) {
    snprintf(buff, MQTT_BUFFER_SIZE, "=0,0");
    validDateTime_cb = NULL;
  } else {
    validDateTime_cb = cb;
    int noTs = 3;

    if (!ts3) noTs--;
    if (!ts2) noTs--;
    if (!ts1) noTs--;

    switch (noTs) {
      case 0:
        snprintf(buff, MQTT_BUFFER_SIZE, "=1,%d", timezone);
        break;
      case 1:
        snprintf(buff, MQTT_BUFFER_SIZE, "=1,%d,\"%s\"", timezone, ts1);
        break;
      case 2:
        snprintf(buff, MQTT_BUFFER_SIZE, "=1,%d,\"%s\",\"%s\"", timezone, ts1, ts2);
        break;
      case 3:
        snprintf(buff, MQTT_BUFFER_SIZE, "=1,%d,\"%s\",\"%s\",\"%s\"",
                 timezone, ts1, ts2, ts3);
        break;
    }
  }
  return at->sendCommand(AT_CMD_CIPSNTPCFG, buff, NULL);
}

//------------------------------------------------------------------------------
bool EspATMQTT::isConnected() {
  return connected;
}

//------------------------------------------------------------------------------
mqtt_status_t EspATMQTT::getNTPTime(char **time) {
  return at->sendCommand(AT_CMD_CIPSNTPTIME, "?", time);
}

//------------------------------------------------------------------------------
void EspATMQTT::process() {
  static uint32_t ntpTimer = millis();

  // First do timers
  if ((millis() - ntpTimer > 1000) && (ntpTimeValid == false)) {
    ntpTimer = millis();
    // Trigger to the current time. Will later be caught in the URC handler.
    at->sendString(MQTT_STRING_CIPSNTPTIME);
  }

  // URC handler
  if (at->available()) {
  char ch;
    // The MQTTSUBRECV URC does not have a line ending which means we need to
    // to perform immediate parsing while reading the line.
    ch = at->read();
    if (ch == '+') {
      int ptr = 0;

      buff[ptr++] = ch;
      do {
        ch = at->read();
        buff[ptr++] = ch;
      } while (ch != ':');
      buff[ptr] = '\0';
      if (strstr(&buff[0], MQTT_RESP_SUBRECV)) {
        // We need to read each section of this line until we have read the
        // length of the data after which we can read the remainder of the line.
        // This is so dumb, it is a line based protocol and it should have had
        // cr/lf at the end of the line just like everything else.
        do {
          ch = at->read();
          buff[ptr++] = ch;
        } while (ch != ',' && ch != -1);

        // Read the topic
        do {
          ch = at->read();
          buff[ptr++] = ch;
        } while (ch != ',' && ch != -1);

        // And at last we get to the number of characters in the response
        int lenPtr = ptr;   // Save a pointer to the length
        do {
          ch = at->read();
          buff[ptr++] = ch;
        } while (ch != ',' && ch != -1);

        // Read all the data
        int len = strtol(&buff[lenPtr], NULL, 10);
        while(len--) {
          ch = at->read();
          buff[ptr++] = ch;
        }
        buff[ptr] = '\0';
        dprintf("Received: '%s'\n", &buff[0]);

        // Tokenize the incoming data for processing
        const char s[2] = ",";
        char *tok = strtok(&buff[0], s);      // First section with response and clientID
        char *topic = strtok(NULL, s) + 1;    // Second with topic minus first " character
        *(topic + strlen(topic) - 1) = '\0';  // Remove last " character
        tok = strtok(NULL, s);                // Third field with data length
        // Can't tokenize the data segment as it will corrupt data if a comma
        // is found. Instead we look for a zero and point to the next character.
        while (*tok++ != '\0');
        if (subscription_cb) {
          subscription_cb(topic, tok);
        }
      } else if (strstr(&buff[0], AT_RESP_CIPSNTPTIME)) {
        ptr = 0;
        do {
          ch = at->read();
          if (ch != '\r' && ch != '\n')
            buff[ptr++] = ch;
        } while (ch != '\n');
        buff[ptr] = '\0';
        dprintf("Received URC: %s\n", &buff[0]);
        // Now check the year  "Tue Jul  5 07:31:56 2022"
        if (!strstr(&buff[20], "1970")) {
          // Inform client that a valid time/date has been received.
          if (validDateTime_cb)
            ntpTimeValid = true;
            validDateTime_cb(&buff[0]);
        }
      } else if (strstr(&buff[0], MQTT_RESP_CONNECTED)) {
        ptr = 0;
        do {
          ch = at->read();
          if (ch != '\r' && ch != '\n')
            buff[ptr++] = ch;
        } while (ch != '\n');
        buff[ptr] = '\0';
        dprintf("Received URC: %s\n", &buff[0]);
        connected = true;
        if (connected_cb)
          connected_cb(&buff[0]);
      } else {
        do {
          ch = at->read();
          if (ch != '\r' && ch != '\n')
            buff[ptr++] = ch;
        } while (ch != '\n');
        buff[ptr] = '\0';
        dprintf("Unhandled out of bound response: %s\n", &buff[0]);
      }
    }
  }
}

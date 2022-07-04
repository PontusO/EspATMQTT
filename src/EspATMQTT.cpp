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
#include <EspATMQTT.h>
#include <AT.h>
#include <cstring>

const char *MQTT_RESP_SUBRECV           = "+MQTTSUBRECV:";

const char *MQTT_CMD_SUB                = "+MQTTSUB";
const char *MQTT_CMD_PUBRAW             = "+MQTTPUBRAW";
const char *MQTT_CMD_PUB                = "+MQTTPUB";
const char *MQTT_CMD_USERCFG            = "+MQTTUSERCFG";
const char *MQTT_CMD_CONN               = "+MQTTCONN";

const char *MQTT_STRING_MQTTPUB         = "+MQTTPUB:";

const char *AT_CMD_SYSLOG               = "+SYSLOG";
//------------------------------------------------------------------------------
EspATMQTT::EspATMQTT(HardwareSerial* serial) {
  at = new AT_Class(serial);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::begin() {
  char *strResult;

  // First we need to make sure that SYSLOG has been enabled to get all the
  // error codes.
  while (at->sendCommand("+GMR", "", NULL) != ESP_AT_SUB_OK)
    delay(100);

  while (at->sendCommand(AT_CMD_SYSLOG, "?", &strResult) != ESP_AT_SUB_OK)
    delay(100);

  // Check
  int result = strtol(strResult, NULL, 10);
  if (!result) {
    // And set if not already set
    at->sendCommand(AT_CMD_SYSLOG, "=1", NULL);
  }

  // Make sure it got set
  at->sendCommand(AT_CMD_SYSLOG, "?", &strResult);
  result = strtol(strResult, NULL, 10);
  if (!result) {
    Serial.printf("Could not update SYSLOG, please check your system.");
    return ESP_AT_SUB_CMD_ERROR;
  }
  return ESP_AT_SUB_OK;
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::UserConfig(uint32_t linkID, mqtt_scheme_t scheme,
                         const char *clientID, const char *userName,
                         const char *password, uint32_t certKeyID, uint32_t caID,
                         const char *path) {

 sprintf(buff, "=%d,%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\"", linkID,
         (uint32_t)scheme, clientID, userName, password, certKeyID, caID, path);

 return at->sendCommand(MQTT_CMD_USERCFG, buff, NULL);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::UserConfig(uint32_t linkID, mqtt_scheme_t scheme,
                         char *clientID, const char *userName,
                         const char *password, uint32_t certKeyID, uint32_t caID,
                         const char *path) {

 sprintf(buff, "=%d,%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\"", linkID,
         (uint32_t)scheme, clientID, userName, password, certKeyID, caID, path);

 return at->sendCommand(MQTT_CMD_USERCFG, buff, NULL);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::UserConfig(uint32_t linkID, mqtt_scheme_t scheme,
                         char *clientID, char *userName, char *password,
                         uint32_t certKeyID, uint32_t caID, const char *path) {

 sprintf(buff, "=%d,%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\"", linkID,
         (uint32_t)scheme, clientID, userName, password, certKeyID, caID, path);

 return at->sendCommand(MQTT_CMD_USERCFG, buff, NULL);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::Connect(uint32_t linkID, const char *host,
                                 uint32_t port, uint32_t reconnect) {
  sprintf(buff, "=%d,\"%s\",%d,%d", linkID, host, port, reconnect);
  return at->sendCommand(MQTT_CMD_CONN, buff, NULL);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::pubString(uint32_t linkID, const char *topic,
                         const char *data, uint32_t qos, uint32_t retain) {
  sprintf(buff, "=%d,\"%s\",\"%s\",%d,%d", linkID, topic, data, qos, retain);
  return at->sendCommand(MQTT_CMD_PUB, buff, NULL);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::pubString(uint32_t linkID, const char *topic, char *data,
                         uint32_t qos, uint32_t retain) {
  sprintf(buff, "=%d,\"%s\",\"%s\",%d,%d", linkID, topic, data, qos, retain);
  return at->sendCommand(MQTT_CMD_PUB, buff, NULL);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::pubRaw(uint32_t linkID, const char *topic, const char *data,
                         uint32_t qos, uint32_t retain) {
  status_code_t status;

  size_t len = strlen(data);

  sprintf(buff, "=%d,\"%s\",%d,%d,%d", linkID, topic, len, qos, retain);
  status = at->sendCommand(MQTT_CMD_PUBRAW, buff, NULL);
  if (status != ESP_AT_SUB_OK) {
    return status;
  }
  at->waitPrompt(100);
  at->sendString(data, len);
  at->waitString(MQTT_STRING_MQTTPUB, 100);

  char *lBuff = at->getBuff();
  if (strstr(lBuff, "FAIL")) {
    return (status_code_t)AT_MQTT_FAILED_TO_PUBLISH_RAW;
  }
  return ESP_AT_SUB_OK;
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::pubRaw(uint32_t linkID, const char *topic, char *data,
                         uint32_t qos, uint32_t retain) {
  status_code_t status;

  size_t len = strlen(data);

  sprintf(buff, "=%d,\"%s\",%d,%d,%d", linkID, topic, len, qos, retain);
  status = at->sendCommand(MQTT_CMD_PUBRAW, buff, NULL);
  if (status != ESP_AT_SUB_OK) {
    return status;
  }
  at->waitPrompt(100);
  at->sendString(data, len);
  at->waitString(MQTT_STRING_MQTTPUB, 100);

  char *lBuff = at->getBuff();
  if (strstr(lBuff, "FAIL")) {
    return (status_code_t)AT_MQTT_FAILED_TO_PUBLISH_RAW;
  }
  return ESP_AT_SUB_OK;
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::subscribeTopic(subscription_cb_t cb, uint32_t linkID,
              const char * topic, uint32_t qos) {
  sprintf(buff,"=%d,\"%s\",%d", linkID, topic, qos);
  subscription_callback = cb;
  return at->sendCommand(MQTT_CMD_SUB, buff, NULL);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::subscribeTopic(subscription_cb_t cb, uint32_t linkID,
              char * topic, uint32_t qos) {
  sprintf(buff,"=%d,\"%s\",%d", linkID, topic, qos);
  subscription_callback = cb;
  return at->sendCommand(MQTT_CMD_SUB, buff, NULL);
}

//------------------------------------------------------------------------------
void EspATMQTT::process() {

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

        // Tokenize the data as required
        const char s[2] = ",";
        char *tok = strtok(&buff[0], s);      // First section with response and clientID
        char *topic = strtok(NULL, s) + 1;    // Second with topic minus first " character
        *(topic + strlen(topic) - 1) = '\0';  // Remove last " character
        tok = strtok(NULL, s);                // Third field with data length
        // Can't tokenize the data segment as it will corrupt data if a comma
        // is found. Instead we look for a zero and point to the next character.
        while (*tok++ != '\0');
        if (subscription_callback) {
          subscription_callback(topic, tok);
        }
      }
    }
  }
}

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
    delay(10);

  while (at->sendCommand("+SYSLOG", "?", &strResult) != ESP_AT_SUB_OK)
    delay(10);

  // Check
  int result = strtol(strResult, NULL, 10);
  if (!result) {
    // And set if not already set
    at->sendCommand("+SYSLOG", "=1", NULL);
  }

  // Make sure it got set
  at->sendCommand("+SYSLOG", "?", &strResult);
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

 return at->sendCommand("+MQTTUSERCFG", buff, NULL);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::UserConfig(uint32_t linkID, mqtt_scheme_t scheme,
                                    const char *clientID) {

 sprintf(buff, "=%d,%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\"", linkID,
         (uint32_t)scheme, clientID, "", "", 0, 0, "");

 return at->sendCommand("+MQTTUSERCFG", buff, NULL);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::Connect(uint32_t linkID, const char *host,
                                 uint32_t port, uint32_t reconnect)
{
  sprintf(buff, "=%d,\"%s\",%d,%d", linkID, host, port, reconnect);
  return at->sendCommand("+MQTTCONN", buff, NULL);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::pubString(uint32_t linkID, const char *topic,
                         const char *data, uint32_t qos, uint32_t retain)
{
  sprintf(buff, "=%d,\"%s\",\"%s\",%d,%d", linkID, topic, data, qos, retain);
  return at->sendCommand("+MQTTPUB", buff, NULL);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::pubString(uint32_t linkID, const char *topic, char *data,
                         uint32_t qos, uint32_t retain)
{
  sprintf(buff, "=%d,\"%s\",\"%s\",%d,%d", linkID, topic, data, qos, retain);
  return at->sendCommand("+MQTTPUB", buff, NULL);
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::pubRaw(uint32_t linkID, const char *topic, const char *data,
                         uint32_t qos, uint32_t retain)
{
  status_code_t status;

  size_t len = strlen(data);

  sprintf(buff, "=%d,\"%s\",%d,%d,%d", linkID, topic, len, qos, retain);
  status = at->sendCommand("+MQTTPUBRAW", buff, NULL);
  if (status != ESP_AT_SUB_OK) {
    return status;
  }
  at->waitPrompt(100);
  at->sendString(data, len);
  at->waitString("+MQTTPUB:", 100);

  char *lBuff = at->getBuff();
  if (strstr(lBuff, "FAIL")) {
    return (status_code_t)AT_MQTT_FAILED_TO_PUBLISH_RAW;
  }
  return ESP_AT_SUB_OK;
}

//------------------------------------------------------------------------------
status_code_t EspATMQTT::pubRaw(uint32_t linkID, const char *topic, char *data,
                         uint32_t qos, uint32_t retain)
{
  status_code_t status;

  size_t len = strlen(data);

  sprintf(buff, "=%d,\"%s\",%d,%d,%d", linkID, topic, len, qos, retain);
  status = at->sendCommand("+MQTTPUBRAW", buff, NULL);
  if (status != ESP_AT_SUB_OK) {
    return status;
  }
  at->waitPrompt(100);
  at->sendString(data, len);
  at->waitString("+MQTTPUB:", 100);

  char *lBuff = at->getBuff();
  if (strstr(lBuff, "FAIL")) {
    return (status_code_t)AT_MQTT_FAILED_TO_PUBLISH_RAW;
  }
  return ESP_AT_SUB_OK;
}

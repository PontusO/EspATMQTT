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
   * @return - The status of the operation, See #mqtt_error_e and
   *      #status_code_e for more information.
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

/*******************************************************************************
 *
 * Setup the connection scheme and how to connect to the server/broker.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - scheme
 *      Specifies what security scheme that ius used when connecting to the
 *      broker. See #mqtt_scheme_e for more details.
 *
 * @param[in] - clientID
 *      The client ID is a string identifier that uniquely identifies the
 *      client.<br>Maximum length is 256 characters.
 *
 * @param[in] - userName
 *      The user name is used to login to the requested broker.<br>
 *      Maxmium length of this is 64 characters.
 *
 * @param[in] - password
 *      The password is used to login to the requested broker.<br>
 *      Maxmium length of this is 64 characters.
 *
 * @param[in] - certKeyID
 *      This is an index to the client certificate key. With the current
 *      ESP-AT firmware (V2.4.0.0) this only supports one key with index 0.<br>
 *      Future versions may support more than one client certificate so this
 *      can change and the API fully supports this.
 *
 * @param[in] - caID
 *      This is an index to the CA root certificate. With the current ESP-AT
 *      firmware (V2.4.0.0) this only supports one key with index 0.<br>
 *      Future versions may support more than one client certificate so this
 *      can change and the API fully supports this.
 *
 * @param[in] - path
 *      At this point we don't know exactly what this parameter is for. It
 *      might be a preparation to support cert/key/ca in the file system
 *      but ther is no information about this from Espressif.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::userConfig(uint32_t linkID, mqtt_scheme_t scheme,
                         const char *clientID, const char *userName,
                         const char *password, uint32_t certKeyID, uint32_t caID,
                         const char *path) {

 snprintf(buff, MQTT_BUFFER_SIZE, "=%d,%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\"", linkID,
         (uint32_t)scheme, clientID, userName, password, certKeyID, caID, path);

 return at->sendCommand(MQTT_CMD_USERCFG, buff, NULL);
}

/*******************************************************************************
 *
 * Setup the connection scheme and how to connect to the server/broker.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - scheme
 *      Specifies what security scheme that ius used when connecting to the
 *      broker. See #mqtt_scheme_e for more details.
 *
 * @param[in] - clientID
 *      The client ID is a string identifier that uniquely identifies the
 *      client.<br>Maximum length is 256 characters.
 *
 * @param[in] - userName
 *      The user name is used to login to the requested broker.<br>
 *      Maxmium length of this is 64 characters.
 *
 * @param[in] - password
 *      The password is used to login to the requested broker.<br>
 *      Maxmium length of this is 64 characters.
 *
 * @param[in] - certKeyID
 *      This is an index to the client certificate key. With the current
 *      ESP-AT firmware (V2.4.0.0) this only supports one key with index 0.<br>
 *      Future versions may support more than one client certificate so this
 *      can change and the API fully supports this.
 *
 * @param[in] - caID
 *      This is an index to the CA root certificate. With the current ESP-AT
 *      firmware (V2.4.0.0) this only supports one key with index 0.<br>
 *      Future versions may support more than one client certificate so this
 *      can change and the API fully supports this.
 *
 * @param[in] - path
 *      At this point we don't know exactly what this parameter is for. It
 *      might be a preparation to support cert/key/ca in the file system
 *      but ther is no information about this from Espressif.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::userConfig(uint32_t linkID, mqtt_scheme_t scheme,
                         char *clientID, const char *userName,
                         const char *password, uint32_t certKeyID, uint32_t caID,
                         const char *path) {

 snprintf(buff, MQTT_BUFFER_SIZE, "=%d,%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\"", linkID,
         (uint32_t)scheme, clientID, userName, password, certKeyID, caID, path);

 return at->sendCommand(MQTT_CMD_USERCFG, buff, NULL);
}

/*******************************************************************************
 *
 * Setup the connection scheme and how to connect to the server/broker.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - scheme
 *      Specifies what security scheme that ius used when connecting to the
 *      broker. See #mqtt_scheme_e for more details.
 *
 * @param[in] - clientID
 *      The client ID is a string identifier that uniquely identifies the
 *      client.<br>Maximum length is 256 characters.
 *
 * @param[in] - userName
 *      The user name is used to login to the requested broker.<br>
 *      Maxmium length of this is 64 characters.
 *
 * @param[in] - password
 *      The password is used to login to the requested broker.<br>
 *      Maxmium length of this is 64 characters.
 *
 * @param[in] - certKeyID
 *      This is an index to the client certificate key. With the current
 *      ESP-AT firmware (V2.4.0.0) this only supports one key with index 0.<br>
 *      Future versions may support more than one client certificate so this
 *      can change and the API fully supports this.
 *
 * @param[in] - caID
 *      This is an index to the CA root certificate. With the current ESP-AT
 *      firmware (V2.4.0.0) this only supports one key with index 0.<br>
 *      Future versions may support more than one client certificate so this
 *      can change and the API fully supports this.
 *
 * @param[in] - path
 *      At this point we don't know exactly what this parameter is for. It
 *      might be a preparation to support cert/key/ca in the file system
 *      but ther is no information about this from Espressif.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::userConfig(uint32_t linkID, mqtt_scheme_t scheme,
                         char *clientID, char *userName, char *password,
                         uint32_t certKeyID, uint32_t caID, const char *path) {

 snprintf(buff, MQTT_BUFFER_SIZE, "=%d,%d,\"%s\",\"%s\",\"%s\",%d,%d,\"%s\"", linkID,
         (uint32_t)scheme, clientID, userName, password, certKeyID, caID, path);

 return at->sendCommand(MQTT_CMD_USERCFG, buff, NULL);
}

/*******************************************************************************
 *
 * Sets a new client ID for the connection. This method must be called after
 * performing a userConfig but before connecting to the broker.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - clientID
 *      A string that uniquely identifies the client to the broker. In the
 *      current version of the ESP-AT fw there is a limitation of 256 characters
 *      for the entire AT command which means that you can only have a clientID
 *      here of up to 236 characters.<br>If you need all 256 characters that
 *      is supported you need to set this in the userConfig command.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::clientID(uint32_t linkID, const char *clientID) {

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, clientID);
  return at->sendCommand(MQTT_CMD_CLIENTID, buff, NULL);
}

/*******************************************************************************
 *
 * Sets a new client ID for the connection. This method must be called after
 * performing a userConfig but before connecting to the broker.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - clientID
 *      A string that uniquely identifies the client to the broker. In the
 *      current version of the ESP-AT fw there is a limitation of 256 characters
 *      for the entire AT command which means that you can only have a clientID
 *      here of up to 236 characters.<br>If you need all 256 characters that
 *      is supported you need to set this in the userConfig command.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::clientID(uint32_t linkID, char *clientID) {

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, clientID);
  return at->sendCommand(MQTT_CMD_CLIENTID, buff, NULL);
}

/*******************************************************************************
 *
 * Sets the username used in authenticating the client with the broker.
 * This method must be called after performing a userConfig but before
 * connecting to the broker.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - username
 *      A string that holds the username used for the authentication. In the
 *      current version of the ESP-AT fw there is a limitation of 256 characters
 *      for the entire AT command which means that you can only have a clientID
 *      here of up to 236 characters.<br>If you need all 256 characters that
 *      is supported you need to set this in the userConfig command.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::username(uint32_t linkID, const char *username) {

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, username);
  return at->sendCommand(MQTT_CMD_USERNAME, buff, NULL);
}

/*******************************************************************************
 *
 * Sets the username used in authenticating the client with the broker.
 * This method must be called after performing a userConfig but before
 * connecting to the broker.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - username
 *      A string that holds the username used for the authentication. In the
 *      current version of the ESP-AT fw there is a limitation of 256 characters
 *      for the entire AT command which means that you can only have a clientID
 *      here of up to 236 characters.<br>If you need all 256 characters that
 *      is supported you need to set this in the userConfig command.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::username(uint32_t linkID, char *username) {

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, username);
  return at->sendCommand(MQTT_CMD_USERNAME, buff, NULL);
}

/*******************************************************************************
 *
 * Sets the password used in authenticating the client with the broker.
 * This method must be called after performing a userConfig but before
 * connecting to the broker.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - password
 *      A string that holds the password used for the authentication. In the
 *      current version of the ESP-AT fw there is a limitation of 256 characters
 *      for the entire AT command which means that you can only have a clientID
 *      here of up to 236 characters.<br>If you need all 256 characters that
 *      is supported you need to set this in the userConfig command.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::password(uint32_t linkID, const char *password) {

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, password);
  return at->sendCommand(MQTT_CMD_PASSWORD, buff, NULL);
}

/*******************************************************************************
 *
 * Sets the password used in authenticating the client with the broker.
 * This method must be called after performing a userConfig but before
 * connecting to the broker.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - password
 *      A string that holds the password used for the authentication. In the
 *      current version of the ESP-AT fw there is a limitation of 256 characters
 *      for the entire AT command which means that you can only have a clientID
 *      here of up to 236 characters.<br>If you need all 256 characters that
 *      is supported you need to set this in the userConfig command.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::password(uint32_t linkID, char *password) {

  snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\"", linkID, password);
  return at->sendCommand(MQTT_CMD_PASSWORD, buff, NULL);
}

/*******************************************************************************
 *
 * Sets the configuration of the current MQTT connection.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - keepalive
 *      Timeout of MQTT ping. Unit: second. Range [0,7200].<br>
 *      The default value is 0, which will be force-changed to 120 s.
 *
 * @param[in] - disable_clean_session
 *      Sets the MQTT clean session propery.<br>
 *      For more details about this parameter we recommend reading the section
 *      Clean Session in MQTT Version 3.1.1.<br>
 *      - 0: enable clean session.<br>
 *      - 1: disable clean session.<br>
 *
 * @param[in] - lwt_topic
 *      Sets the topic of the last will and testament message. Maximum number of
 *      characters allowed for this topic is 128.
 *
 * @param[in] - lwt_message
 *      Sets the last will and testament message. Maximum length of the message
 *      is 64 characters.
 *
 * @param[in] - lwt_qos
 *      Sets the Quality of Service plan for the last will and testament
 *      message. Can be set to 0, 1 or and the default setting is 0.
 *
 * @param[in] - lwt_retain
 *      Sets the retain parameter of the last will and testament message. Can
 *      be set to 0 or 1 and the default setting is 0.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
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

/*******************************************************************************
 *
 * This function sets the ALPN (Application Layer Protocol Negotiation) of the
 * link. Up to 5 different protocols can be specified in one command. If no
 * protocol is specified the ESP-AT stack will clear out all active ALPNs.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - alpn1-alpn5
 *      ALPN specifier. For instance Amazon uses "x-amzn-mqtt-ca".<br>
 *      Other service providers will be using different protocol names and
 *      the user need to change this accordingly.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
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

/*******************************************************************************
 *
 * Connects to the specified server/broker with the previously defined
 * transport scheme. If an active connection is established within the
 * specified timeout it reports ESP_AT_SUB_CMD_CONN_SYNCH back but if a timeout
 * occured before an active connection could be made it reports
 * ESP_AT_SUB_CMD_CONN_ASYNCH and the caller needs to either check the
 * connection with #isConnected() or setup a callback function that will be
 * called when the connection is made active
 *
 * Tip:  Specifying a very short timout (~50mS) will in most cases cause the
 *       method to return with ESP_AT_SUB_CMD_CONN_ASYNCH and a callback
 *       will later be called when the connection is established. This can
 *       be used to your advantage if you do not want to let the system hang
 *       while waiting for a connection to be made. As an example, connecting
 *       to AWS can take several seconds which would potentially lock your
 *       system all that time. One downside with this is that the calling
 *       application will need to keep track of the callback and possibly
 *       generate an error condition if it doesn't arrive within a specified
 *       time.<br>
 *       The later can be avoided by actually specifying a very large timeout,
 *       maybe 10 seconds or more, and try to force a synchronous connection.
 *       In that case a timeout condition can be detected when
 *       ESP_AT_SUB_CMD_CONN_ASYNCH is returned.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - host
 *      The host name of the MQTT broker. This can be specified as an IP
 *      address or a URL. Maximum length of this string is 128 bytes.
 *
 * @param[in] - port
 *      The port where the broker will listen to requests. The default port
 *      is 1883 which is the default for a mosquitto server.
 *
 * @param[in] - reconnect
 *      Used to tell the ESP-AT device to automaticall reconnect id a connection
 *      is lost.<br>
 *      - 0 = Do not reconnect.<br>
 *      - 1 = Automatically reconnect when a connection is lost.<br>
 *
 * @param[in] - timeout
 *      The caller can specify a connection timeout value here. The default
 *      timeout is 2 seconds.<br>
 *      The method will initially try to connect synchronously, i.e return with
 *      an active connection. If a timeout occurs during the connection process
 *      it will initialize a call back function (if specified) which is called
 *      by the process() command when a connection is established.
 *
 * @param[in] - cb
 *      This is the callback function that shall be called by the #process()
 *      method when a connection is made assuming the #connect() method reported
 *      ESP_AT_SUB_CMD_CONN_ASYNCH back.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
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

/*******************************************************************************
 *
 * Publish a string to a specified topic.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - topic
 *      The topic where the message shold be published.
 *
 * @param[in] - message
 *      The message that should be published to the specified topic.
 *
 * @param[in] - qos
 *      The Quality of Service value that should be used for this message.<br>
 *      The following values are recognized:<br>
 *      - 0 - At most once:<br>The minimal QoS level is zero. This service level
 *            guarantees a best-effort delivery. There is no guarantee of
 *            delivery. The recipient does not acknowledge receipt of the
 *            message and the message is not stored and re-transmitted by the
 *            sender. QoS level 0 is often called “fire and forget” and provides
 *            the same guarantee as the underlying TCP protocol.
 *      - 1 - At least once:<br>QoS level 1 guarantees that a message is
 *            delivered at least one time to the receiver. The sender stores the
 *            message until it gets a PUBACK packet from the receiver that
 *            acknowledges receipt of the message. It is possible for a message
 *            to be sent or delivered multiple times.
 *      - 2 - Exactly once:<br>QoS 2 is the highest level of service in MQTT.
 *            This level guarantees that each message is received only once by
 *            the intended recipients. QoS 2 is the safest and slowest quality
 *            of service level.
 *
 * @param[in] - retain
 *      A retained message is a normal MQTT message with the retained flag set
 *      to true (= 1). The broker stores the last retained message and the
 *      corresponding QoS for that topic. See #mqtt_retain_e for more details.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::pubString(uint32_t linkID, const char *topic,
                         const char *data, uint32_t qos, uint32_t retain) {
  if (connected) {
    snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\",\"%s\",%d,%d", linkID, topic, data, qos, retain);
    return at->sendCommand(MQTT_CMD_PUB, buff, NULL);
  }
  return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_IN_DISCONNECTED_STATE;
}

/*******************************************************************************
 *
 * Publish a string to a specified topic.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - topic
 *      The topic where the message shold be published.
 *
 * @param[in] - message
 *      The message that should be published to the specified topic.
 *
 * @param[in] - qos
 *      The Quality of Service value that should be used for this message.<br>
 *      The following values are recognized:<br>
 *      - 0 - At most once:<br>The minimal QoS level is zero. This service level
 *            guarantees a best-effort delivery. There is no guarantee of
 *            delivery. The recipient does not acknowledge receipt of the
 *            message and the message is not stored and re-transmitted by the
 *            sender. QoS level 0 is often called “fire and forget” and provides
 *            the same guarantee as the underlying TCP protocol.
 *      - 1 - At least once:<br>QoS level 1 guarantees that a message is
 *            delivered at least one time to the receiver. The sender stores the
 *            message until it gets a PUBACK packet from the receiver that
 *            acknowledges receipt of the message. It is possible for a message
 *            to be sent or delivered multiple times.
 *      - 2 - Exactly once:<br>QoS 2 is the highest level of service in MQTT.
 *            This level guarantees that each message is received only once by
 *            the intended recipients. QoS 2 is the safest and slowest quality
 *            of service level.
 *
 * @param[in] - retain
 *      A retained message is a normal MQTT message with the retained flag set
 *      to true (= 1). The broker stores the last retained message and the
 *      corresponding QoS for that topic. See #mqtt_retain_e for more details.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::pubString(uint32_t linkID, const char *topic, char *data,
                         uint32_t qos, uint32_t retain) {
  if (connected) {
    snprintf(buff, MQTT_BUFFER_SIZE, "=%d,\"%s\",\"%s\",%d,%d", linkID, topic, data, qos, retain);
    return at->sendCommand(MQTT_CMD_PUB, buff, NULL);
  }
  return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_IN_DISCONNECTED_STATE;
}

/*******************************************************************************
 *
 * Publish raw data to a specified topic.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - topic
 *      The topic where the message shold be published.
 *
 * @param[in] - message
 *      The message that should be published to the specified topic. Must be a
 *      '\0' terminated string.
 *
 * @param[in] - qos
 *      The Quality of Service value that should be used for this message.<br>
 *      The following values are recognized:<br>
 *      - 0 - At most once:<br>The minimal QoS level is zero. This service level
 *            guarantees a best-effort delivery. There is no guarantee of
 *            delivery. The recipient does not acknowledge receipt of the
 *            message and the message is not stored and re-transmitted by the
 *            sender. QoS level 0 is often called “fire and forget” and provides
 *            the same guarantee as the underlying TCP protocol.
 *      - 1 - At least once:<br>QoS level 1 guarantees that a message is
 *            delivered at least one time to the receiver. The sender stores the
 *            message until it gets a PUBACK packet from the receiver that
 *            acknowledges receipt of the message. It is possible for a message
 *            to be sent or delivered multiple times.
 *      - 2 - Exactly once:<br>QoS 2 is the highest level of service in MQTT.
 *            This level guarantees that each message is received only once by
 *            the intended recipients. QoS 2 is the safest and slowest quality
 *            of service level.
 *
 * @param[in] - retain
 *      A retained message is a normal MQTT message with the retained flag set
 *      to true (= 1). The broker stores the last retained message and the
 *      corresponding QoS for that topic. See #mqtt_retain_e for more details.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
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

/*******************************************************************************
 *
 * Publish raw data to a specified topic.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - topic
 *      The topic where the message shold be published.
 *
 * @param[in] - message
 *      The message that should be published to the specified topic. Must be a
 *      '\0' terminated string.
 *
 * @param[in] - qos
 *      The Quality of Service value that should be used for this message.<br>
 *      The following values are recognized:<br>
 *      - 0 - At most once:<br>The minimal QoS level is zero. This service level
 *            guarantees a best-effort delivery. There is no guarantee of
 *            delivery. The recipient does not acknowledge receipt of the
 *            message and the message is not stored and re-transmitted by the
 *            sender. QoS level 0 is often called “fire and forget” and provides
 *            the same guarantee as the underlying TCP protocol.
 *      - 1 - At least once:<br>QoS level 1 guarantees that a message is
 *            delivered at least one time to the receiver. The sender stores the
 *            message until it gets a PUBACK packet from the receiver that
 *            acknowledges receipt of the message. It is possible for a message
 *            to be sent or delivered multiple times.
 *      - 2 - Exactly once:<br>QoS 2 is the highest level of service in MQTT.
 *            This level guarantees that each message is received only once by
 *            the intended recipients. QoS 2 is the safest and slowest quality
 *            of service level.
 *
 * @param[in] - retain
 *      A retained message is a normal MQTT message with the retained flag set
 *      to true (= 1). The broker stores the last retained message and the
 *      corresponding QoS for that topic. See #mqtt_retain_e for more details.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
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

/*******************************************************************************
 *
 * Subscribe to messages from a specific topic.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - topic
 *      The topic for which we are listening to.
 *
 * @param[in] - qos
 *      The Quality of Service value that should be used for this message.<br>
 *      The following values are recognized:<br>
 *      - 0 - At most once:<br>The minimal QoS level is zero. This service level
 *            guarantees a best-effort delivery. There is no guarantee of
 *            delivery. The recipient does not acknowledge receipt of the
 *            message and the message is not stored and re-transmitted by the
 *            sender. QoS level 0 is often called “fire and forget” and provides
 *            the same guarantee as the underlying TCP protocol.
 *      - 1 - At least once:<br>QoS level 1 guarantees that a message is
 *            delivered at least one time to the receiver. The sender stores the
 *            message until it gets a PUBACK packet from the receiver that
 *            acknowledges receipt of the message. It is possible for a message
 *            to be sent or delivered multiple times.
 *      - 2 - Exactly once:<br>QoS 2 is the highest level of service in MQTT.
 *            This level guarantees that each message is received only once by
 *            the intended recipients. QoS 2 is the safest and slowest quality
 *            of service level.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
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

/*******************************************************************************
 *
 * Subscribe to messages from a specific topic.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - topic
 *      The topic for which we are listening to.
 *
 * @param[in] - qos
 *      The Quality of Service value that should be used for this message.<br>
 *      The following values are recognized:<br>
 *      - 0 - At most once:<br>The minimal QoS level is zero. This service level
 *            guarantees a best-effort delivery. There is no guarantee of
 *            delivery. The recipient does not acknowledge receipt of the
 *            message and the message is not stored and re-transmitted by the
 *            sender. QoS level 0 is often called “fire and forget” and provides
 *            the same guarantee as the underlying TCP protocol.
 *      - 1 - At least once:<br>QoS level 1 guarantees that a message is
 *            delivered at least one time to the receiver. The sender stores the
 *            message until it gets a PUBACK packet from the receiver that
 *            acknowledges receipt of the message. It is possible for a message
 *            to be sent or delivered multiple times.
 *      - 2 - Exactly once:<br>QoS 2 is the highest level of service in MQTT.
 *            This level guarantees that each message is received only once by
 *            the intended recipients. QoS 2 is the safest and slowest quality
 *            of service level.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
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

/*******************************************************************************
 *
 * Unsubscribe from messages with a specific topic.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - topic
 *      The topic that should be unsubscribed from.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
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

/*******************************************************************************
 *
 * Unsubscribe from messages with a specific topic.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @param[in] - topic
 *      The topic that should be unsubscribed from.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
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

/*******************************************************************************
 *
 * Close (all) conection/s with the specified link ID.
 *
 * @param[in] - linkID
 *      The link ID used for this connection. Only linkID 0 is currently
 *      supported by the ESP-AT stack.<br>Using the library DEFAULT_LINK_ID will
 *      make it easy to migrate if this changes in the future.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::close(uint32_t linkID) {
  if (connected) {
    snprintf(buff, MQTT_BUFFER_SIZE, "=%d", linkID);
    return at->sendCommand(MQTT_CMD_CLEAN, buff, NULL);
  }
  return ESP_AT_SUB_CMD_PROCESSING | AT_MQTT_IN_DISCONNECTED_STATE;
}

/*******************************************************************************
 *
 * Enables the internal NTP Time client. Up to 3 different time servers can
 * be specified to support quick multi regional time resolution. If no external
 * time servers are specified on of the following time servers will be used
 * instead:<br>
 *   - “cn.ntp.org.cn”
 *   - “ntp.sjtu.edu.cn”
 *   - “us.pool.ntp.org”
 *
 * When using a secure connection mechanism (TLS) the NTP client can be used
 * to acquire the current date and time which is required by the TLS protocol.
 * When the time has been acquired from a time server a call back is
 * generated and the connection to the mqtt server can be made.
 *
 * @param[in] - enable
 *      Enable or disable the NTP client. A "0" in this parameter will disable
 *      the client completely and a "1" is required if the NTP client should
 *      be enabled.
 *
 * @param[in] - cb
 *      A pointer to a callback method that is called when a valid date and time
 *      has been acquired from an NTP server. If this is NULL, no call back
 *      will be made and the caller has to examine the time to determine when
 *      a valid date/time has been acquired.
 *
 * @param[in] - timezone
 *      This parameter support the following two formats:<br>
 *      - Value range -12 to 14. It marks most of the time zones by offset from
 *        Coordinated Universal Time (UTC) in whole hours (UTC–12:00 to
 *        UTC+14:00).<br>
 *      - The second format is UTC offset. The UTC offset specifies the time
 *        value you must add to the UTC time to get a local time value. It has
 *        a syntax like [+|-][hh]mm. This is negative if the local time zone is
 *        on the west of the Prime Meridian and positive if it is on the east.
 *        The hour(hh) must be between -12 and 14, and the minute(mm) between 0
 *        and 59. For example, if you want to set the timezone to New Zealand
 *        (Chatham Islands) which is in UTC+12:45, you should set the parameter
 *        to 1245. Please refer to UTC offset wiki
 *        (https://en.wikipedia.org/wiki/Time_zone#List_of_UTC_offsets) for more
 *        information.
 *
 * @param[in] - ts1 - ts3
 *        Time server URL's, for instance "time.google.com" or "0.pool.ntp.org"
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
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

/*******************************************************************************
 *
 * Get the current time from the ESP-AT internal NTP Client
 *
 * @param[out] - time
 *      The current time as reported by the ESP-AT internal NTP client. If the
 *      NTP client has not been enabled an error will be returned.
 *      The time is reported back as a character string (An Example:
 *      "Tue Oct 19 17:47:56 2021") and the caller need to decode it.
 *
 * @return - The status of the operation, See #mqtt_error_e and
 *      #status_code_e for more information.
 *
 ******************************************************************************/
mqtt_status_t EspATMQTT::getNTPTime(char **time) {
  return at->sendCommand(AT_CMD_CIPSNTPTIME, "?", time);
}

/*******************************************************************************
 *
 * Checks to see if the mqtt client is connected and returns true if it is.
 *
 * @return - true or false depending on the current connection status.
 *
 ******************************************************************************/
bool EspATMQTT::isConnected() {
  return connected;
}

/*******************************************************************************
 *
 * The process method must be placed in the main loop in order to process
 * MQTT events that are reported by the ESP-AT device. If not present the
 * library will not function as expected.
 *
 ******************************************************************************/
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

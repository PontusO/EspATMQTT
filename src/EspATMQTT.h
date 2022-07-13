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

/** @file */

#ifndef _H_ESPATMQTT_
#define _H_ESPATMQTT_

#include <inttypes.h>
#include <AT.h>

#define MQTT_BUFFER_SIZE              1024

/**
 * MQTT configuration schemes
 */
enum mqtt_scheme_e {
  ESP_MQTT_SCHEME_MQTT_OVER_TCP                   = 1, /**< MQTT over plain TCP. */
  ESP_MQTT_SCHEME_MQTT_OVER_TLS_NCV               = 2, /**< MQTT over TLS (no certificate verification). */
  ESP_MQTT_SCHEME_MQTT_OVER_TLS_VSC               = 3, /**< MQTT over TLS (verify server certificate). */
  ESP_MQTT_SCHEME_MQTT_OVER_TLS_PCC               = 4, /**< MQTT over TLS (provide client certificate). */
  ESP_MQTT_SCHEME_MQTT_OVER_TLS_VSCPCC            = 5, /**< MQTT over TLS (verify server certificate and provide client certificate). */
  ESP_MQTT_SCHEME_MQTT_OVER_WS_TCP                = 6, /**< MQTT over WebSocket (based on TCP). */
  ESP_MQTT_SCHEME_MQTT_OVER_WS_TCP_SEC_TLS_NCV    = 7, /**< MQTT over WebSocket Secure (based on TLS, no certificate verification). */
  ESP_MQTT_SCHEME_MQTT_OVER_WS_TCP_SEC_TLS_VSC    = 8, /**< MQTT over WebSocket Secure (based on TLS, verify server certificate). */
  ESP_MQTT_SCHEME_MQTT_OVER_WS_TCP_SEC_TLS_PCC    = 9, /**< MQTT over WebSocket Secure (based on TLS, provide client certificate). */
  ESP_MQTT_SCHEME_MQTT_OVER_WS_TCP_SEC_TLS_VSCPCC = 10 /**< MQTT over WebSocket Secure (based on TLS, verify server certificate and provide client certificate). */
};
typedef enum mqtt_scheme_e mqtt_scheme_t;

/**
 *
 * The caller should use these error codes to determine the cause of an MQTT
 * error. Using the MQTT_ERROR(x) macro simplifies separating the MQTT specific
 * part of the error code from the AT error code.
 *
 * Be aware of that the documentation from Espressif regarding these error
 * codes is vague and the explanations here might not be 100% correct.
 *
 */
enum mqtt_error_e {
  AT_MQTT_NO_CONFIGURED                   = 0x6001, /**< MQTT layer not configured yet */
  AT_MQTT_NOT_IN_CONFIGURED_STATE         = 0x6002, /**< MQTT layer not in a configured state (same as above ?)  */
  AT_MQTT_UNINITIATED_OR_ALREADY_CLEAN    = 0x6003, /**< Tried to perform clean when the MQTT layer was already clean */
  AT_MQTT_ALREADY_CONNECTED               = 0x6004, /**< Tried to connect when already connected */
  AT_MQTT_MALLOC_FAILED                   = 0x6005, /**< Failed to allocate memory internally */
  AT_MQTT_NULL_LINK                       = 0x6006, /**< ? */
  AT_MQTT_NULL_PARAMTER                   = 0x6007, /**< A NULL parameter was found when valid data was expected */
  AT_MQTT_PARAMETER_COUNTS_IS_WRONG       = 0x6008, /**< The number of parameters is incorrect */
  AT_MQTT_TLS_CONFIG_ERROR                = 0x6009, /**< Something is wrong with the TLS configuration */
  AT_MQTT_PARAM_PREPARE_ERROR             = 0x600A, /**< ? */
  AT_MQTT_CLIENT_START_FAILED             = 0x600B, /**< Failed to start MQTT client */
  AT_MQTT_CLIENT_PUBLISH_FAILED           = 0x600C, /**< Failed to publish data to broker */
  AT_MQTT_CLIENT_SUBSCRIBE_FAILED         = 0x600D, /**< Failed to subscribe to topic at broker */
  AT_MQTT_CLIENT_UNSUBSCRIBE_FAILED       = 0x600E, /**< Failed to unsubscribe from topic */
  AT_MQTT_CLIENT_DISCONNECT_FAILED        = 0x600F, /**< Failed to disconnect from the broker */
  AT_MQTT_LINK_ID_READ_FAILED             = 0x6010, /**< Failed to read the link ID */
  AT_MQTT_LINK_ID_VALUE_IS_WRONG          = 0x6011, /**< Wrong link ID value was used */
  AT_MQTT_SCHEME_READ_FAILED              = 0x6012, /**< Failed to read the security scheme used */
  AT_MQTT_SCHEME_VALUE_IS_WRONG           = 0x6013, /**< Incorrect scheme value was used */
  AT_MQTT_CLIENT_ID_READ_FAILED           = 0x6014, /**< Failed to read the client ID */
  AT_MQTT_CLIENT_ID_IS_NULL               = 0x6015, /**< The client ID can't be NULL */
  AT_MQTT_CLIENT_ID_IS_OVERLENGTH         = 0x6016, /**< The client ID is to long */
  AT_MQTT_USERNAME_READ_FAILED            = 0x6017, /**< Failed to read the user name */
  AT_MQTT_USERNAME_IS_NULL                = 0x6018, /**< The user name can't be NULL */
  AT_MQTT_USERNAME_IS_OVERLENGTH          = 0x6019, /**< The user name is to long */
  AT_MQTT_PASSWORD_READ_FAILED            = 0x601A, /**< Failed to read the password */
  AT_MQTT_PASSWORD_IS_NULL                = 0x601B, /**< The password can't be NULL */
  AT_MQTT_PASSWORD_IS_OVERLENGTH          = 0x601C, /**< The password is to long */
  AT_MQTT_CERT_KEY_ID_READ_FAILED         = 0x601D, /**< The cert key ID could not be read */
  AT_MQTT_CERT_KEY_ID_VALUE_IS_WRONG      = 0x601E, /**< The cert key ID is incorrect */
  AT_MQTT_CA_ID_READ_FAILED               = 0x601F, /**< The CA ID could not be read */
  AT_MQTT_CA_ID_VALUE_IS_WRONG            = 0x6020, /**< The CA ID value is incorrect */
  AT_MQTT_CA_LENGTH_ERROR                 = 0x6021, /**< The CA length is incorrect */
  AT_MQTT_CA_READ_FAILED                  = 0x6022, /**< Failed to read the CA */
  AT_MQTT_CERT_LENGTH_ERROR               = 0x6023, /**< Incorrect client certificate length */
  AT_MQTT_CERT_READ_FAILED                = 0x6024, /**< Failed to read client certificate */
  AT_MQTT_KEY_LENGTH_ERROR                = 0x6025, /**< Incorrect client key length */
  AT_MQTT_KEY_READ_FAILED                 = 0x6026, /**< Failed to read client key */
  AT_MQTT_PATH_READ_FAILED                = 0x6027, /**< Failed to read from path */
  AT_MQTT_PATH_IS_NULL                    = 0x6028, /**< The path must not be NULL */
  AT_MQTT_PATH_IS_OVERLENGTH              = 0x6029, /**< The path length is to long */
  AT_MQTT_VERSION_READ_FAILED             = 0x602A, /**< Failed to read version */
  AT_MQTT_KEEPALIVE_READ_FAILED           = 0x602B, /**< Failed to read keepalive value */
  AT_MQTT_KEEPALIVE_IS_NULL               = 0x602C, /**< Keepalive value must not be NULL */
  AT_MQTT_KEEPALIVE_VALUE_IS_WRONG        = 0x602D, /**< Keepalive value is incorrect */
  AT_MQTT_DISABLE_CLEAN_SESSION_READ_FAILED  = 0x602E, /**< Failed to read the clean session parameter */
  AT_MQTT_DISABLE_CLEAN_SESSION_VALUE_IS_WRONG = 0x602F, /**< Incorrect value of the clean session value */
  AT_MQTT_LWT_TOPIC_READ_FAILED           = 0x6030, /**< Failed to read the lwt topic */
  AT_MQTT_LWT_TOPIC_IS_NULL               = 0x6031, /**< LWT topic can not be set to NULL */
  AT_MQTT_LWT_TOPIC_IS_OVERLENGTH         = 0x6032, /**< LWT topic is to long */
  AT_MQTT_LWT_MSG_READ_FAILED             = 0x6033, /**< Failed to read LWT message */
  AT_MQTT_LWT_MSG_IS_NULL                 = 0x6034, /**< LWT message can not be set to NULL */
  AT_MQTT_LWT_MSG_IS_OVERLENGTH           = 0x6035, /**< LWT message is to long */
  AT_MQTT_LWT_QOS_READ_FAILED             = 0x6036, /**< Failed to read LWT QoS parameter */
  AT_MQTT_LWT_QOS_VALUE_IS_WRONG          = 0x6037, /**< Incorrect LWT QoS value specified */
  AT_MQTT_LWT_RETAIN_READ_FAILED          = 0x6038, /**< Failed to read LWT retain parameter */
  AT_MQTT_LWT_RETAIN_VALUE_IS_WRONG       = 0x6039, /**< Incorrect LWT value specified */
  AT_MQTT_HOST_READ_FAILED                = 0x603A, /**< Failed to read host parameter */
  AT_MQTT_HOST_IS_NULL                    = 0x603B, /**< Failed to set host, value can not be NULL */
  AT_MQTT_HOST_IS_OVERLENGTH              = 0x603C, /**< Host value to long */
  AT_MQTT_PORT_READ_FAILED                = 0x603D, /**< Failed to read port parameter */
  AT_MQTT_PORT_VALUE_IS_WRONG             = 0x603E, /**< Port value is incorrect */
  AT_MQTT_RECONNECT_READ_FAILED           = 0x603F, /**< Failed to read reconnect parameter */
  AT_MQTT_RECONNECT_VALUE_IS_WRONG        = 0x6040, /**< Incorrect reconnect value specified */
  AT_MQTT_TOPIC_READ_FAILED               = 0x6041, /**< Failed to read topic parameter */
  AT_MQTT_TOPIC_IS_NULL                   = 0x6042, /**< Failed to set topic, can't be NULL */
  AT_MQTT_TOPIC_IS_OVERLENGTH             = 0x6043, /**< Failed to set topic, to long */
  AT_MQTT_DATA_READ_FAILED                = 0x6044, /**< Failed to read message */
  AT_MQTT_DATA_IS_NULL                    = 0x6045, /**< Incorrect message specified, can't be NULL */
  AT_MQTT_DATA_IS_OVERLENGTH              = 0x6046, /**< Message to long */
  AT_MQTT_QOS_READ_FAILED                 = 0x6047, /**< Failed to read QoS parameter */
  AT_MQTT_QOS_VALUE_IS_WRONG              = 0x6048, /**< Incorrect QoS value specified */
  AT_MQTT_RETAIN_READ_FAILED              = 0x6049, /**< Failed to read retain parameter */
  AT_MQTT_RETAIN_VALUE_IS_WRONG           = 0x604A, /**< Incorrect retain value specified */
  AT_MQTT_PUBLISH_LENGTH_READ_FAILED      = 0x604B, /**< Failed to read publish length parameter */
  AT_MQTT_PUBLISH_LENGTH_VALUE_IS_WRONG   = 0x604C, /**< Failed to set published length value */
  AT_MQTT_RECV_LENGTH_IS_WRONG            = 0x604D, /**< Incorrect receive length is specified */
  AT_MQTT_CREATE_SEMA_FAILED              = 0x604E, /**< Internal error, failed to create semaphore */
  AT_MQTT_CREATE_EVENT_GROUP_FAILED       = 0x604F, /**< Internal error, failed to create event group */
  AT_MQTT_URI_PARSE_FAILED                = 0x6050, /**< Internal error, failed to parse URI */
  AT_MQTT_IN_DISCONNECTED_STATE           = 0x6051, /**< MQTT is in disconnected state */
  AT_MQTT_HOSTNAME_VERIFY_FAILED          = 0x6052, /**< Hostname lookup failed */
  AT_MQTT_FAILED_TO_PUBLISH_RAW           = 0x6053  /**< Failed to publish raw data */
};

/**
 * A retained message is a normal MQTT message with the retained flag set to
 * true. The broker stores the last retained message and the corresponding QoS
 * for that topic. Each client that subscribes to a topic pattern that matches
 * the topic of the retained message receives the retained message immediately
 * after they subscribe. The broker stores only one retained message per topic.
 */
enum mqtt_retain_e {
  AT_MQTT_DO_NOT_RETAIN = 0,
  AT_MQTT_RETAIN = 1
};

/**
 * Parameter values that represent the keepalive states in function calls where
 * the keepalive function is used.
 */
enum mqtt_keepalive_e {
  AT_MQTT_DO_NOT_KEEPALIVE = 0,
  AT_MQTT_KEEPALIVE = 1
};

/** @typedef mqtt_connectType_t
 * Used to define the different connection types of a MQTT connection.
 */
typedef enum mqtt_connectType_e {
  AT_CONN_UNCONNECTED                     = 0x1000, /**< MQTT server still in its unconnected state */
  AT_CONN_SYNCH                           = 0x1001, /**< MQTT server has connected synchronously */
  AT_CONN_ASYNCH                          = 0x1002  /**< MQTT server has not connected yet and is awaiting a connection callback */
} mqtt_connectType_t;

#define MQTT_ERROR(x)                     (x & 0xffff)

#define DEFAULT_LINK_ID                   0  /**< This is the only supported link ID as of version 2.4.0.0 of the ESP-AT firmware */

/**
 * @typedef subscription_cb_t
 * This is the callback function data type for subscription call backs.
 */
typedef void (*subscription_cb_t)(char *topic, char *mqttdata);
/**
 * @typedef validDateTime_cb_t
 * This is the callback function data type for valid date/time call backs.
 */
typedef void (*validDateTime_cb_t)(char *dateTime);
/**
 * @typedef connected_cb_t
 * This is the callback function data type for connection call backs.
 */
typedef void (*connected_cb_t)(char *connectionString);

/**
 * The return value of an ESP-AT MQTT operation. This value is a combination of
 * the enums mqtt_error_e and mqtt_error_e. The caller should check for both to
 * determine the cause of the error.
 */
typedef uint32_t            mqtt_status_t;

/*******************************************************************************
 * EspAT MQTT EspATMQTT class definition
 ******************************************************************************/
class EspATMQTT {
public:
  EspATMQTT(HardwareSerial* = &ESP_SERIAL_PORT);

  mqtt_status_t begin();
  mqtt_status_t userConfig(uint32_t linkID, mqtt_scheme_t scheme, const char *clientID,
                           const char *userName="", const char *password="",
                           uint32_t certKeyID=0, uint32_t caID=0,
                           const char *path="");
  mqtt_status_t userConfig(uint32_t linkID, mqtt_scheme_t scheme, char *clientID,
                           const char *userName="", const char *password="",
                           uint32_t certKeyID=0, uint32_t caID=0,
                           const char *path="");
  mqtt_status_t userConfig(uint32_t linkID, mqtt_scheme_t scheme, char *clientID,
                           char *userName, char *password, uint32_t certKeyID=0,
                           uint32_t caID=0, const char *path="");
  mqtt_status_t connect(uint32_t linkID, const char *host,
                           uint32_t port=1883, uint32_t reconnect=0,
                           uint32_t timeout = 5000, connected_cb_t cb = NULL);
  mqtt_status_t setClientID(uint32_t linkID, const char *clientID);
  mqtt_status_t setClientID(uint32_t linkID, char *clientID);
  mqtt_status_t setUsername(uint32_t linkID, const char *username);
  mqtt_status_t setUsername(uint32_t linkID, char *username);
  mqtt_status_t setPassword(uint32_t linkID, const char *password);
  mqtt_status_t setPassword(uint32_t linkID, char *password);
  mqtt_status_t connectionConfig(uint32_t linkID, uint32_t keepalive,
                           uint32_t disable_clean_session, const char* lwt_topic,
                           const char* lwt_message, uint32_t lwt_qos = 0,
                           uint32_t lwt_retain = 0);
  mqtt_status_t setALPN(uint32_t linkID, const char *alpn1 = NULL,
                                         const char *alpn2 = NULL,
                                         const char *alpn3 = NULL,
                                         const char *alpn4 = NULL,
                                         const char *alpn5 = NULL);
  mqtt_status_t pubString(uint32_t linkID, const char *topic, const char *data,
                           uint32_t qos=0, uint32_t retain=0);
  mqtt_status_t pubString(uint32_t linkID, const char *topic, char *data,
                           uint32_t qos=0, uint32_t retain=0);
  mqtt_status_t pubRaw(uint32_t linkID, const char *topic, const char *data,
                           uint32_t qos=0, uint32_t retain=0);
  mqtt_status_t pubRaw(uint32_t linkID, const char *topic, char *data,
                           uint32_t qos=0, uint32_t retain=0);
  mqtt_status_t subscribeTopic(subscription_cb_t cb, uint32_t linkID, const char * topic, uint32_t qos=0);
  mqtt_status_t subscribeTopic(subscription_cb_t cb, uint32_t linkID, char * topic, uint32_t qos=0);
  mqtt_status_t unSubscribeTopic(uint32_t linkID, const char * topic);
  mqtt_status_t unSubscribeTopic(uint32_t linkID, char * topic);
  mqtt_status_t close(uint32_t linkID);

  // Non MQTT stuff but needed or usefull to get things going
  mqtt_status_t enableNTPTime(bool enable, validDateTime_cb_t cb, uint32_t timezone,
                           const char *ts1 = NULL, const char *ts2 = NULL,
                           const char *ts3 = NULL);
  mqtt_status_t getNTPTime(char **time);
  bool isConnected();
  void process();
private:
  AT_Class *at;
  subscription_cb_t subscription_cb;
  validDateTime_cb_t validDateTime_cb;
  mqtt_connectType_t connType;
  connected_cb_t connected_cb;

  int topicSubscriptions;
  bool connected;
  bool ntpTimeValid;

  char buff[MQTT_BUFFER_SIZE];
};

#endif

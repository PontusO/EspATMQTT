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
#ifndef _H_ESPATMQTT_
#define _H_ESPATMQTT_

#include <inttypes.h>
#include <AT.h>

/* MQTT configuration schemes */
typedef enum mqtt_scheme_e {
  ESP_MQTT_SCHEME_MQTT_OVER_TCP                   = 1, // MQTT over TCP.
  ESP_MQTT_SCHEME_MQTT_OVER_TLS_NCV               = 2, // MQTT over TLS (no certificate verification).
  ESP_MQTT_SCHEME_MQTT_OVER_TLS_VSC               = 3, // MQTT over TLS (verify server certificate).
  ESP_MQTT_SCHEME_MQTT_OVER_TLS_PCC               = 4, // MQTT over TLS (provide client certificate).
  ESP_MQTT_SCHEME_MQTT_OVER_TLS_VSCPCC            = 5, // MQTT over TLS (verify server certificate and provide client certificate).
  ESP_MQTT_SCHEME_MQTT_OVER_WS_TCP                = 6, // MQTT over WebSocket (based on TCP).
  ESP_MQTT_SCHEME_MQTT_OVER_WS_TCP_SEC_TLS_NCV    = 7, // MQTT over WebSocket Secure (based on TLS, no certificate verification).
  ESP_MQTT_SCHEME_MQTT_OVER_WS_TCP_SEC_TLS_VSC    = 8, // MQTT over WebSocket Secure (based on TLS, verify server certificate).
  ESP_MQTT_SCHEME_MQTT_OVER_WS_TCP_SEC_TLS_PCC    = 9, // MQTT over WebSocket Secure (based on TLS, provide client certificate).
  ESP_MQTT_SCHEME_MQTT_OVER_WS_TCP_SEC_TLS_VSCPCC = 10 // MQTT over WebSocket Secure (based on TLS, verify server certificate and provide client certificate).
} mqtt_scheme_t;

typedef enum mqtt_error_e {
  AT_MQTT_NO_CONFIGURED                   = 0x6001,
  AT_MQTT_NOT_IN_CONFIGURED_STATE         = 0x6002,
  AT_MQTT_UNINITIATED_OR_ALREADY_CLEAN    = 0x6003,
  AT_MQTT_ALREADY_CONNECTED               = 0x6004,
  AT_MQTT_MALLOC_FAILED                   = 0x6005,
  AT_MQTT_NULL_LINK                       = 0x6006,
  AT_MQTT_NULL_PARAMTER                   = 0x6007,
  AT_MQTT_PARAMETER_COUNTS_IS_WRONG       = 0x6008,
  AT_MQTT_TLS_CONFIG_ERROR                = 0x6009,
  AT_MQTT_PARAM_PREPARE_ERROR             = 0x600A,
  AT_MQTT_CLIENT_START_FAILED             = 0x600B,
  AT_MQTT_CLIENT_PUBLISH_FAILED           = 0x600C,
  AT_MQTT_CLIENT_SUBSCRIBE_FAILED         = 0x600D,
  AT_MQTT_CLIENT_UNSUBSCRIBE_FAILED       = 0x600E,
  AT_MQTT_CLIENT_DISCONNECT_FAILED        = 0x600F,
  AT_MQTT_LINK_ID_READ_FAILED             = 0x6010,
  AT_MQTT_LINK_ID_VALUE_IS_WRONG          = 0x6011,
  AT_MQTT_SCHEME_READ_FAILED              = 0x6012,
  AT_MQTT_SCHEME_VALUE_IS_WRONG           = 0x6013,
  AT_MQTT_CLIENT_ID_READ_FAILED           = 0x6014,
  AT_MQTT_CLIENT_ID_IS_NULL               = 0x6015,
  AT_MQTT_CLIENT_ID_IS_OVERLENGTH         = 0x6016,
  AT_MQTT_USERNAME_READ_FAILED            = 0x6017,
  AT_MQTT_USERNAME_IS_NULL                = 0x6018,
  AT_MQTT_USERNAME_IS_OVERLENGTH          = 0x6019,
  AT_MQTT_PASSWORD_READ_FAILED            = 0x601A,
  AT_MQTT_PASSWORD_IS_NULL                = 0x601B,
  AT_MQTT_PASSWORD_IS_OVERLENGTH          = 0x601C,
  AT_MQTT_CERT_KEY_ID_READ_FAILED         = 0x601D,
  AT_MQTT_CERT_KEY_ID_VALUE_IS_WRONG      = 0x601E,
  AT_MQTT_CA_ID_READ_FAILED               = 0x601F,
  AT_MQTT_CA_ID_VALUE_IS_WRONG            = 0x6020,
  AT_MQTT_CA_LENGTH_ERROR                 = 0x6021,
  AT_MQTT_CA_READ_FAILED                  = 0x6022,
  AT_MQTT_CERT_LENGTH_ERROR               = 0x6023,
  AT_MQTT_CERT_READ_FAILED                = 0x6024,
  AT_MQTT_KEY_LENGTH_ERROR                = 0x6025,
  AT_MQTT_KEY_READ_FAILED                 = 0x6026,
  AT_MQTT_PATH_READ_FAILED                = 0x6027,
  AT_MQTT_PATH_IS_NULL                    = 0x6028,
  AT_MQTT_PATH_IS_OVERLENGTH              = 0x6029,
  AT_MQTT_VERSION_READ_FAILED             = 0x602A,
  AT_MQTT_KEEPALIVE_READ_FAILED           = 0x602B,
  AT_MQTT_KEEPALIVE_IS_NULL               = 0x602C,
  AT_MQTT_KEEPALIVE_VALUE_IS_WRONG        = 0x602D,
  AT_MQTT_DISABLE_CLEAN_SESSION_READ_FAILED  = 0x602E,
  AT_MQTT_DISABLE_CLEAN_SESSION_VALUE_IS_WRONG = 0x602F,
  AT_MQTT_LWT_TOPIC_READ_FAILED           = 0x6030,
  AT_MQTT_LWT_TOPIC_IS_NULL               = 0x6031,
  AT_MQTT_LWT_TOPIC_IS_OVERLENGTH         = 0x6032,
  AT_MQTT_LWT_MSG_READ_FAILED             = 0x6033,
  AT_MQTT_LWT_MSG_IS_NULL                 = 0x6034,
  AT_MQTT_LWT_MSG_IS_OVERLENGTH           = 0x6035,
  AT_MQTT_LWT_QOS_READ_FAILED             = 0x6036,
  AT_MQTT_LWT_QOS_VALUE_IS_WRONG          = 0x6037,
  AT_MQTT_LWT_RETAIN_READ_FAILED          = 0x6038,
  AT_MQTT_LWT_RETAIN_VALUE_IS_WRONG       = 0x6039,
  AT_MQTT_HOST_READ_FAILED                = 0x603A,
  AT_MQTT_HOST_IS_NULL                    = 0x603B,
  AT_MQTT_HOST_IS_OVERLENGTH              = 0x603C,
  AT_MQTT_PORT_READ_FAILED                = 0x603D,
  AT_MQTT_PORT_VALUE_IS_WRONG             = 0x603E,
  AT_MQTT_RECONNECT_READ_FAILED           = 0x603F,
  AT_MQTT_RECONNECT_VALUE_IS_WRONG        = 0x6040,
  AT_MQTT_TOPIC_READ_FAILED               = 0x6041,
  AT_MQTT_TOPIC_IS_NULL                   = 0x6042,
  AT_MQTT_TOPIC_IS_OVERLENGTH             = 0x6043,
  AT_MQTT_DATA_READ_FAILED                = 0x6044,
  AT_MQTT_DATA_IS_NULL                    = 0x6045,
  AT_MQTT_DATA_IS_OVERLENGTH              = 0x6046,
  AT_MQTT_QOS_READ_FAILED                 = 0x6047,
  AT_MQTT_QOS_VALUE_IS_WRONG              = 0x6048,
  AT_MQTT_RETAIN_READ_FAILED              = 0x6049,
  AT_MQTT_RETAIN_VALUE_IS_WRONG           = 0x604A,
  AT_MQTT_PUBLISH_LENGTH_READ_FAILED      = 0x604B,
  AT_MQTT_PUBLISH_LENGTH_VALUE_IS_WRONG   = 0x604C,
  AT_MQTT_RECV_LENGTH_IS_WRONG            = 0x604D,
  AT_MQTT_CREATE_SEMA_FAILED              = 0x604E,
  AT_MQTT_CREATE_EVENT_GROUP_FAILED       = 0x604F,
  AT_MQTT_URI_PARSE_FAILED                = 0x6050,
  AT_MQTT_IN_DISCONNECTED_STATE           = 0x6051,
  AT_MQTT_HOSTNAME_VERIFY_FAILED          = 0x6052,
  AT_MQTT_FAILED_TO_PUBLISH_RAW           = 0x6053
} mqtt_error_t;

#define MQTT_ERROR(x)                     (x & 0xffff)

/*******************************************************************************
 * EspAT MQTT class definition
 ******************************************************************************/
class EspATMQTT {
public:
  EspATMQTT(HardwareSerial* = &ESP_SERIAL_PORT);

  status_code_t begin();
  status_code_t UserConfig(uint32_t linkID, mqtt_scheme_t scheme, const char *clientID,
                           const char *userName, const char *password,
                           uint32_t certKeyID, uint32_t caID,
                           const char *path);
  status_code_t UserConfig(uint32_t linkID, mqtt_scheme_t scheme,
                           const char *clientID);
  status_code_t Connect(uint32_t linkID, const char *host,
                           uint32_t port=1883, uint32_t reconnect=1);
  status_code_t pubString(uint32_t linkID, const char *topic, const char *data,
                           uint32_t qos=0, uint32_t retain=0);
  status_code_t pubString(uint32_t linkID, const char *topic, char *data,
                           uint32_t qos=0, uint32_t retain=0);
  status_code_t pubRaw(uint32_t linkID, const char *topic, const char *data,
                           uint32_t qos=0, uint32_t retain=0);
  status_code_t pubRaw(uint32_t linkID, const char *topic, char *data,
                           uint32_t qos=0, uint32_t retain=0);
private:
  AT_Class *at;

  char buff[1024];
};

#endif
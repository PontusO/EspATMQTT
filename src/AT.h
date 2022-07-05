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
#include <inttypes.h>
#include <Arduino.h>

#ifndef _H_AT_COM_
#define _H_AT_COM_

typedef enum status_code_e {
  ESP_AT_SUB_OK                   = 0x00000000,
  ESP_AT_SUB_COMMON_ERROR         = 0x01010000,
  ESP_AT_SUB_NO_TERMINATOR        = 0x01020000,
  ESP_AT_SUB_NO_AT                = 0x01030000,
  ESP_AT_SUB_PARA_LENGTH_MISMATCH = 0x01040000,
  ESP_AT_SUB_PARA_TYPE_MISMATCH   = 0x01050000,
  ESP_AT_SUB_PARA_NUM_MISMATCH    = 0x01060000,
  ESP_AT_SUB_PARA_INVALID         = 0x01070000,
  ESP_AT_SUB_PARA_PARSE_FAIL      = 0x01080000,
  ESP_AT_SUB_UNSUPPORT_CMD        = 0x01090000,
  ESP_AT_SUB_CMD_EXEC_FAIL        = 0x010a0000,
  ESP_AT_SUB_CMD_PROCESSING       = 0x010b0000,
  ESP_AT_SUB_CMD_OP_ERROR         = 0x010c0000,
  ESP_AT_SUB_CMD_ERROR            = 0x010d0000,
  ESP_AT_SUB_CMD_TIMEOUT          = 0x010e0000,
  ESP_AT_SUB_CMD_CONN_SYNCH       = 0x010f0000,
  ESP_AT_SUB_CMD_CONN_ASYNCH      = 0x01100000,
  ESP_AT_SUB_CMD_LAST_COMMAND
} status_code_t;

#define AT_ERROR(x)       (((uint32_t)x >> 16) & 0x0f)
typedef uint32_t          at_status_t;

/*******************************************************************************
 * EspAT MQTT AT_Class definition
 ******************************************************************************/
class AT_Class {
public:
  AT_Class(HardwareSerial* = &ESP_SERIAL_PORT);

  size_t readLine();
  at_status_t waitReply(const char *asynch, uint32_t timeout);
  at_status_t sendCommand(const char *cmd, const char *param, char **result,
                            const char *asynch = NULL, uint32_t timeout=2000);
  at_status_t waitPrompt(uint32_t timeout);
  at_status_t waitString(const char *str, uint32_t timeout);
  at_status_t sendString(const char *str, size_t len);
  at_status_t sendString(char *str, size_t len);
  at_status_t sendString(const char *str);
  size_t readString();
  char read();
  int available();

  char *getBuff();
  uint32_t getErrno();
private:
  HardwareSerial* _serial;
  char buff[1024];        // Serial read buffer
  char cmdBuff[256];      // Command buffer for stuff sent to the ESP
  char resBuff[128];      // Result buffer for parameter data returned from the ESP
  int wx;
  int line;
};

#endif

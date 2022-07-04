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
  ESP_AT_SUB_OK = 0x00,
  ESP_AT_SUB_COMMON_ERROR = 0x01,
  ESP_AT_SUB_NO_TERMINATOR = 0x02,
  ESP_AT_SUB_NO_AT = 0x03,
  ESP_AT_SUB_PARA_LENGTH_MISMATCH = 0x04,
  ESP_AT_SUB_PARA_TYPE_MISMATCH = 0x05,
  ESP_AT_SUB_PARA_NUM_MISMATCH = 0x06,
  ESP_AT_SUB_PARA_INVALID = 0x07,
  ESP_AT_SUB_PARA_PARSE_FAIL = 0x08,
  ESP_AT_SUB_UNSUPPORT_CMD = 0x09,
  ESP_AT_SUB_CMD_EXEC_FAIL = 0x0a,
  ESP_AT_SUB_CMD_PROCESSING = 0x0b,
  ESP_AT_SUB_CMD_OP_ERROR = 0x0c,
  ESP_AT_SUB_CMD_ERROR = 0x0d,
  ESP_AT_SUB_CMD_LAST_COMMAND
} status_code_t;

#define AT_ERROR(x)       ((x >> 16) & 0x0f)

/*******************************************************************************
 * EspAT MQTT class definition
 ******************************************************************************/
class AT_Class {
public:
  AT_Class(HardwareSerial* = &ESP_SERIAL_PORT);

  size_t readLine();
  status_code_t waitReply(uint32_t timeout);
  status_code_t sendCommand(const char *cmd, const char *param, char **result, uint32_t timeout=2000);
  status_code_t waitPrompt(uint32_t timeout);
  status_code_t waitString(const char *str, uint32_t timeout);
  status_code_t sendString(const char *str, size_t len);
  status_code_t sendString(char *str, size_t len);
  size_t readString();
  char read();
  int available();
  void setTimeout(uint32_t to);

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

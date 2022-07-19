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

#include <inttypes.h>
#include <Arduino.h>

#ifndef _H_AT_COM_
#define _H_AT_COM_

#define ESP_AT_CMDBUFF_LENGTH     256
/**
 *
 * Status codes that the AT handler can return. These are basically the same
 * return (error) codes that the ESP-AT module returns when an error is
 * detected but we have added a few to simplify upper layer handling.
 */
enum status_code_e {
  ESP_AT_SUB_OK                   = 0x00000000, /**< All is good in life and no errors have been detected */
  ESP_AT_SUB_COMMON_ERROR         = 0x01010000, /**< A generic error has occured, no more info is available */
  ESP_AT_SUB_NO_TERMINATOR        = 0x01020000, /**< The At interpretor did not detect a terminator */
  ESP_AT_SUB_NO_AT                = 0x01030000, /**< Command was not prepended with "AT" */
  ESP_AT_SUB_PARA_LENGTH_MISMATCH = 0x01040000, /**< Mismatch in the parameter length */
  ESP_AT_SUB_PARA_TYPE_MISMATCH   = 0x01050000, /**< Wrong parameter type */
  ESP_AT_SUB_PARA_NUM_MISMATCH    = 0x01060000, /**< Incorrect number of parameters */
  ESP_AT_SUB_PARA_INVALID         = 0x01070000, /**< Invalid parameter value */
  ESP_AT_SUB_PARA_PARSE_FAIL      = 0x01080000, /**< Failed to parse parameter */
  ESP_AT_SUB_UNSUPPORT_CMD        = 0x01090000, /**< Found unsupported command */
  ESP_AT_SUB_CMD_EXEC_FAIL        = 0x010a0000, /**< Command execution failed */
  ESP_AT_SUB_CMD_PROCESSING       = 0x010b0000, /**< Error in command processing,
                                                     this error always occur before a MQTT error happens */
  ESP_AT_SUB_CMD_OP_ERROR         = 0x010c0000, /**< Error while performing and operation */
  ESP_AT_SUB_CMD_ERROR            = 0x010d0000, /**< Generic command error */
  ESP_AT_SUB_CMD_TIMEOUT          = 0x010e0000, /**< AT_Class timout waiting for a reply from the interpreter */
  ESP_AT_SUB_CMD_CONN_SYNCH       = 0x010f0000, /**< MQTT connection was synchronous */
  ESP_AT_SUB_CMD_CONN_ASYNCH      = 0x01100000, /**< MQTT connection was asynchronous,
                                                     a callback will be issued when the connection is made */
  ESP_AT_SUB_CMD_RETRY            = 0x01110000, /**< The ESP-AT device returned a busy reply */
  ESP_AT_SUB_CMD_INVALID_PKI_PART = 0x01120000, /**< The system found an invalid PKI partition */
  ESP_AT_SUB_CMD_LAST_COMMAND
};

#define AT_ERROR(x)       (x & 0x00ff0000)

/**
 * The return value of an AT operation. This value is represented by a member
 * of the enum #status_code_e. The caller can use the AT_ERROR macro to check
 * for a specifik status code.
 */
typedef uint32_t          at_status_t;

/*******************************************************************************
 * EspAT MQTT AT_Class definition
 *
 * This class provides an AT interface layer that can be used by higher level
 * software to easily send and receive data to and from an ESP-AT module.
 * The module takes care of sending the desired command, waiting for a response
 * as well as storing and parsing the returned data.
 *
 * A few support functions are also included for implementations that require
 * more control over the data sent and received.
 ******************************************************************************/
class AT_Class {
public:
  AT_Class(HardwareSerial* = &ESP_SERIAL_PORT);
  size_t readLine(uint32_t timeout = 2000);
  at_status_t waitReply(const char *asynch, uint32_t timeout);
  at_status_t sendCommand(const char *cmd, const char *param, char **result,
                            const char *asynch = NULL, uint32_t timeout=10000);
  at_status_t waitPrompt(uint32_t timeout=2000);
  at_status_t waitString(const char *str, uint32_t timeout);
  at_status_t sendString(const char *str, size_t len);
  at_status_t sendString(char *str, size_t len);
  at_status_t sendString(const char *str);
  char read(uint32_t timeout = 500);
  void write(char ch);
  int available();

  char *getBuff();
  void setSerial(HardwareSerial* = &ESP_SERIAL_PORT);
  HardwareSerial* getSerial();
private:
  HardwareSerial* _serial;

  char buff[1024];    /**< Serial input buffer */
  char cmdBuff[ESP_AT_CMDBUFF_LENGTH];  /**< Command buffer for stuff sent to the ESP-AT device */
  char resBuff[128];  /**< Result buffer for parameter data returned from the ESP-AT device */
  int wx;             /**< Write pointer to the input buffer while processing an ESP-AT response */
  int line;           /**< Keeps track of how many lines have been received during the processing of an ESP-AT reply */
};

#endif

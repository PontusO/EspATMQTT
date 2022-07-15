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

#include <AT.h>

/** @file */

#define MQTT_MAX_CERTIFICATE_LENGTH     2048
#define MQTT_CERT_BUFFER_LENGTH         MQTT_MAX_CERTIFICATE_LENGTH

/**
 * Partition table from a ESP32C3 version 2.3.0
 *
 * "ble_data",      64, 1,0x1f000, 0x6000
 * "server_cert",   64, 2,0x25000, 0x2000
 * "server_key",    64, 3,0x27000, 0x2000
 * "server_ca",     64, 4,0x29000, 0x2000
 * "client_cert",   64, 5,0x2b000, 0x2000
 * "client_key",    64, 6,0x2d000, 0x2000
 * "client_ca",     64, 7,0x2f000, 0x2000
 * "factory_param", 64, 8,0x31000, 0x1000
 * "wpa2_cert",     64, 9,0x32000, 0x2000
 * "wpa2_key",      64,10,0x34000, 0x2000
 * "wpa2_ca",       64,11,0x36000, 0x2000
 * "mqtt_cert",     64,12,0x38000, 0x2000
 * "mqtt_key",      64,13,0x3a000, 0x2000
 * "mqtt_ca",       64,14,0x3c000, 0x2000
 * "fatfs",         1,129,0x47000,0x19000
 *
 * Here's how a PKI partition is configured in flash memory:
 *
 *   +--------------+--------------+---------------+-------------------+---------------------------+
 *   | <- 1 byte -> | <- 1 byte -> | <- 2 bytes -> | <- content len -> | <- (4-len%4) % 4 bytes -> |
 *   +--------------+--------------+---------------+-------------------+---------------------------+
 *   |     type     |     ID       | content len   |    raw data       | padding for 4 bytes align |
 *   +--------------+--------------+---------------+-------------------+---------------------------+
 *
 *   type: item type
 *       0x01: CA
 *       0x02: certificate
 *       0x03: key
 *   ID: used to match cert and key.
 *   content len: raw data length
 *
 * File Format ( unsigned int is little endian )
 *
 *   +---------------+---------------+---------------+--------------------+
 *   | <- 2 bytes -> | <- 2 bytes -> | <- 4 bytes -> | <- total length -> |
 *   +---------------+---------------+---------------+--------------------+
 *   |   magic code  |  list size    |     length    |    list items      |
 *   +---------------+---------------+---------------+--------------------+
 *
 *   magic code: 0xF1F1
 *   list size: count of total items
 *   length: total length of list items
 *   list items: a list of PKIItems
 *
 **/

/**
 * Definition of the the PKI item header that makes out the first 4 bytes of
 * each PKI item,
 */
typedef struct pki_item_s {
  uint8_t type;
  uint8_t id;
  uint16_t len;
} pki_item_t;

typedef struct file_format_s {
  uint16_t magic;
  uint16_t list_size;
  uint32_t length;
} file_format_t;

#define MQTT_CERT_PART      0
#define MQTT_KEY_PART       1
#define MQTT_CA_PART        2

class MqttCertMgmt {
public:
  MqttCertMgmt(HardwareSerial* = &ESP_SERIAL_PORT);

  at_status_t readPkiItem(uint32_t partition, char *caBuffer, size_t length,
              pki_item_t *pki_item, uint32_t index = 0);
  at_status_t comparePkiItem(uint32_t partition, const char *caBuffer,
              bool *result, size_t length);
  at_status_t comparePkiItem(uint32_t partition, char *caBuffer,
              bool *result, size_t length);
  at_status_t writePkiItem(uint32_t partition, const char *caBuffer,
              bool *result, size_t length);
  at_status_t writePkiItem(uint32_t partition, char *caBuffer,
              bool *result, size_t length);
private:
  at_status_t getPkiHeader(uint32_t partition, pki_item_t *pki_item,
              uint32_t index = 0);
  at_status_t readSysFlash(uint32_t partition, char *buffer, uint32_t offset,
              size_t length);
  at_status_t getFlashData(const char *cmd, char *parms, char *retBuff,
              uint32_t timeout=2000);
  AT_Class *at;
  char buff[MQTT_CERT_BUFFER_LENGTH];   // Output and input buffer
  char pBuff[128];                      // Parameter buffer
  char certBuff[MQTT_MAX_CERTIFICATE_LENGTH];
};

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

#include <MqttCertMgmt.h>
#include <AT.h>
#include <debug.h>

#define DEBUG         1

/** @file */

static const char *mqtt_cert_part   = "mqtt_cert";
static const char *mqtt_key_part    = "mqtt_key";
static const char *mqtt_ca_part     = "mqtt_ca";
static const char *mqtt_parts[3] = {mqtt_cert_part, mqtt_key_part, mqtt_ca_part};

enum mqtt_cert_flash_operation_e {
  MQTT_CERT_OP_ERASE_SECTOR = 0,
  MQTT_CERT_OP_WRITE = 1,
  MQTT_CERT_OP_READ = 2
};

enum mqtt_pki_item_e {
  PKI_TYPE_CA = 0x01,
  PKI_TYPE_CERT = 0x02,
  PKI_TYPE_KEY = 0x03
};

static const char *AT_CMD_SYSFLASH             = "+SYSFLASH";
static const char *AT_CMD_SYSFLASH_RESP        = "+SYSFLASH:";
/*******************************************************************************
 *
 * The constructor simply assigns creates a new instance of the AT class
 * to be used by the certificate management. Maybe it would be better here
 * to share instance with the MQTT manager but this will do for now.
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
MqttCertMgmt::MqttCertMgmt(HardwareSerial* serial) {
  at = new AT_Class(serial);
}

/*******************************************************************************
 *
 * Reads the PKI item from the specified partition and returns it in the provided
 * buffer.
 *
 * @param[out] - pkiBuffer
 *    A caller provided buffer where to store the requested PKI item
 *    data.
 *
 * @param[in] - length
 *    The length of the pkiBuffer the caller has provided. If the buffer is to
 *    small an error will be returned.
 *
 * @param[out] - pki_item
 *    The PKI item associated data such as PKI type, index and length.
 *
 * @param[in] - index
 *    The caller requested index of table of PKI items. Not supported in
 *    version 2.3.0 of the ESP-AT interpreter.
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::readPkiItem(uint32_t partition, char *pkiBuffer,
            size_t length, pki_item_t *pki_item, uint32_t index) {
  at_status_t status;

  status = getPkiHeader(partition, pki_item, index);
  if (status != ESP_AT_SUB_OK)
    return status;
  if (pki_item->len > length)
    return ESP_AT_SUB_PARA_NUM_MISMATCH;

  // Now get certificate data
  status = readSysFlash(partition, pkiBuffer, 12, pki_item->len);

  return status;
}

/*******************************************************************************
 *
 * Compares a stored certifiate to the supplied and returns if it matches or
 * not in out output parameter result.
 *
 * @param[in] - caBuffer
 *    A pointer to the certificate to compare the stored certificate with.
 *
 * @param[in] - length
 *    The length of the supplied certificate in bytes.
 *
 * @param[out] - result
 *    True if the certificates are equal, false if not.
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::comparePkiItem(uint32_t partition,
            const char *caBuffer, bool *result, size_t length) {
  at_status_t status;
  pki_item_t pki_item;

  // Read PKI item descriptor
  status = getPkiHeader(partition, &pki_item);
  if (status != ESP_AT_SUB_OK)
    return status;

  // If length differs the certs most certainly differ as well.
  if (pki_item.len != (length)) {
    dprintf("Certificate length missmatch !\nExisting cert length %d,"
            " compared cert length %d\n", pki_item.len, length);
    *result = false;
    return ESP_AT_SUB_OK;
  }

  // All seems fine so far, lets get the certificate from FLASH to compare.
  status = readSysFlash(partition, &certBuff[0], 12, pki_item.len);
  if (status != ESP_AT_SUB_OK)
    return status;

  for (int i=0;i<pki_item.len;i++) {
    if (caBuffer[i] != certBuff[i]) {
      *result = false;
      return ESP_AT_SUB_OK;
    }
  }
  *result = true;
  return ESP_AT_SUB_OK;
}

/*******************************************************************************
 *
 * Compares a stored certifiate to the supplied and returns if it matches or
 * not in out output parameter result.
 *
 * @param[in] - caBuffer
 *    A pointer to the certificate to compare the stored certificate with.
 *
 * @param[in] - length
 *    The length of the supplied certificate in bytes.
 *
 * @param[out] - result
 *    True if the certificates are equal, false if not.
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::comparePkiItem(uint32_t partition,
            char *caBuffer, bool *result, size_t length) {
  at_status_t status;
  pki_item_t pki_item;

  // Read PKI item descriptor
  status = getPkiHeader(partition, &pki_item);
  if (status != ESP_AT_SUB_OK)
    return status;

  // If length differs the certs most certainly differ as well.
  if (pki_item.len != (length)) {
    dprintf("Certificate length missmatch !\nExisting cert length %d,"
            " compared cert length %d\n", pki_item.len, length);
    *result = false;
    return ESP_AT_SUB_OK;
  }

  // All seems fine so far, lets get the certificate from FLASH to compare.
  status = readSysFlash(partition, &certBuff[0], 12, pki_item.len);
  if (status != ESP_AT_SUB_OK)
    return status;

  for (int i=0;i<pki_item.len;i++) {
    if (caBuffer[i] != certBuff[i]) {
      *result = false;
      return ESP_AT_SUB_OK;
    }
  }
  *result = true;
  return ESP_AT_SUB_OK;
}

/*******************************************************************************
 *
 * Reads the PKI header, parses it and returns the data to the caller.
 *
 * @param[in] - partition
 *   An integer specifying the partition to read from.
 *
 * @param[in] - index
 *   The index of the pki item that shall be retrieved. With the current
 *   version of the ESP-AT interpreter (V2.3.0) only one certificate/key
 *   per partition is supported.
 *
 * @param[out] - pki_item
 *   Caller provided buffer for the resulting pki_item.
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::getPkiHeader(uint32_t partition, pki_item_t *pki_item,
            uint32_t index) {
  char rData[32];
  at_status_t status;
  file_format_t ff;

  // When more than one certificate is supported by ESP-AT firmware this
  // need to change.
  if (index != 0)
    return ESP_AT_SUB_PARA_INVALID;

  // Read file format, not used at the moment
  status = readSysFlash(partition, (char *)&ff, 0, sizeof(file_format_t));
  if (status != ESP_AT_SUB_OK)
    return status;

  dprintf("File format magic 0x%04x\n", ff.magic);
  dprintf("File format number of items 0x%04x\n", ff.list_size);
  dprintf("File format total length 0x%04x (%d) bytes\n", ff.length, ff.length);

  // Read the PKI item
  status = readSysFlash(partition, (char *)pki_item, sizeof(file_format_t),
           sizeof(pki_item_t));

  return status;
}

/*******************************************************************************
 *
 * Read certificate/key data from flash using AT command +SYSFLASH.
 *
 * @param[in] - partition
 *   An integer specifying the partition to read from.
 *
 * @param[in] - buffer
 *   Pointer to a buffer where the data should be stored.
 *
 * @param[in] - offset
 *   Offset in the flash partition to start reading data from.
 *
 * @param[in] - length
 *   This is the number of bytes that should be read into the buffer. The caller
 *   must ensure that the buffer is large enough to hold all the data.
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::readSysFlash(uint32_t partition, char *buffer,
            uint32_t offset, size_t length) {

  snprintf(pBuff, 128, "=2,\"%s\",%d,%d",
           mqtt_parts[partition], offset, length);
  return getFlashData(AT_CMD_SYSFLASH, pBuff, buffer);
}

/*******************************************************************************
 *
 * Special handling of the SYSFLASH command is needed as it returns data
 * differently from normal text based commands.
 *
 * @param[in] - cmd
 *    This is the command section of the hole AT command.
 *
 * @param[in] - param
 *    This is the parameter part of the AT command. The caller can fill a
 *    buffer using snprintf for instance with all the correct AT parameters.
 *
 * @param[out] - retBuff
 *    This buffer will hold the result returned by the ESP-AT module.
 *
 * @param[in] - timeout
 *    A timeout value can be specified if it is expected to take a very long
 *    time for the reply to arrive. This is 2 seconds default which probably
 *    covers most situations.
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::getFlashData(const char *cmd, char *param,
                         char *retBuff, uint32_t timeout) {

  snprintf(buff, MQTT_CERT_BUFFER_LENGTH, "AT%s%s\r\n", cmd, param);
  at->sendString(buff);
  dprintf("S:\'%s\'\n", buff);

  // Ensure that we have received at least a start of the reply within the
  // given timeout.
  uint32_t to = millis();
  while (!at->available() && (millis() - to < timeout))
    yield();
  if (millis() - to >= timeout)
    return ESP_AT_SUB_CMD_TIMEOUT;

  // Make sure we read in the response identifier (+SYSFLASH:)
  int ix = 0;
  do {
    buff[ix++] = at->read();
    buff[ix] = 0;   // Always terminate the incoming string
  } while (!strstr(&buff[0], AT_CMD_SYSFLASH_RESP) && (millis() - to < timeout));
  // Check for timeout
  if (millis() - to >= timeout)
    return ESP_AT_SUB_CMD_TIMEOUT;

  // Now get the number of bytes in the response
  ix = 0;
  char ch;
  do {
    ch = at->read();
    buff[ix++] = ch;
  } while (isdigit(ch) && (millis() - to < timeout));
  buff[ix] = '\0';
  // Check for timeout
  if (millis() - to >= timeout)
    return ESP_AT_SUB_CMD_TIMEOUT;

  // Read and store the received data in the return buffer.
  int numBytes = strtol(&buff[0], NULL, 10);
  dprintf("Getting %d bytes from the ESP32\n", numBytes);
  ix = 0;
  while (numBytes--) {
    ch = at->read();
    retBuff[ix++] = ch;
  }
  return ESP_AT_SUB_OK;
}

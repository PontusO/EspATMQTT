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

/**
 * List of partition names that holds mqtt certificates.
 */
static const char *mqtt_cert_part   = "mqtt_cert";
static const char *mqtt_key_part    = "mqtt_key";
static const char *mqtt_ca_part     = "mqtt_ca";
static const char *mqtt_parts[3] = {mqtt_cert_part, mqtt_key_part, mqtt_ca_part};
/**
 * Array holding the different ESP-AT native PKI types found in the pki item header.
 */
uint32_t pkiTypes[] = { 2, 3, 1 };

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
 * The constructor simply creates a new instance of the AT class to be used
 * by the certificate management.
 *
 ******************************************************************************/
MqttCertMgmt::MqttCertMgmt(HardwareSerial* serial) {
  _at = new AT_Class(serial);
}

/*******************************************************************************
 *
 * Uses an already constructed AT_Class.
 *
 ******************************************************************************/
MqttCertMgmt::MqttCertMgmt(AT_Class* at) {
  _at = at;
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
 * @param[in] - pkiBuffer
 *    A pointer to the pki item data to compare the stored certificate with.
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
            const char *pkiBuffer, bool *result, size_t length, uint32_t index) {
  at_status_t status;
  pki_item_t pki_item;

  // Read PKI item descriptor
  status = getPkiHeader(partition, &pki_item, index);
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
    if (pkiBuffer[i] != certBuff[i]) {
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
 * @param[in] - pkiBuffer
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
            char *pkiBuffer, bool *result, size_t length, uint32_t index) {

  return comparePkiItem(partition, (const char *)pkiBuffer, result, length, index);
}

/*******************************************************************************
 *
 * Here we are going to write a PKI item to the specified partitions.
 * Currently we don't really care what is in there already, that should have
 * been checked and dealt with by the caller before calling this function.
 *
 * @param[in] - partition
 *    Specifies to what partition we intend to write to.

 * @param[in] - pkiBuffer
 *    A pointer to the PKI item that we want to write to the partition.
 *
 * @param[in] - length
 *    The length of the supplied PKI item
 *
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::writePkiItem(uint32_t partition, const char *pkiBuffer,
            size_t length) {
  at_status_t status;
  file_format_t ff;
  pki_item_t pki_item;

  dprintf("Writing new PKI data and structures to FLASH\n", NULL);
  // First erase the partition that we are writing the data to.
  status = erasePartition(partition);
  if (status != ESP_AT_SUB_OK) {
    dprintf("Failed to erase partition, error 0x%08x occured.\n", status);
    return status;
  }

  /* Now, create the file format header of the partition.
   *   +---------------+---------------+---------------+--------------------+
   *   | <- 2 bytes -> | <- 2 bytes -> | <- 4 bytes -> | <- total length -> |
   *   +---------------+---------------+---------------+--------------------+
   *   |   magic code  |  list size    |     length    |    list items      |
   *   +---------------+---------------+---------------+--------------------+
   */
  ff.magic = FF_MAGIC;
  ff.list_size = 1;   /* Always 1 as long as ESP-AT only supports one item */
  ff.length = length + sizeof(pki_item_t);
  status = writeSysFlash(partition, (char *)&ff, 0, sizeof(ff));
  if (status != ESP_AT_SUB_OK) {
    dprintf("Failed to write file format header, error 0x%08x occured.\n", status);
    return status;
  }

  /* And now we update the PKI item header.
   *   +--------------+--------------+---------------+-------------------+---------------------------+
   *   | <- 1 byte -> | <- 1 byte -> | <- 2 bytes -> | <- content len -> | <- (4-len%4) % 4 bytes -> |
   *   +--------------+--------------+---------------+-------------------+---------------------------+
   *   |     type     |     ID       | content len   |    raw data       | padding for 4 bytes align |
   *   +--------------+--------------+---------------+-------------------+---------------------------+
   */
  pki_item.type = pkiTypes[partition];
  pki_item.id = 1;
  pki_item.len = length;
  status = writeSysFlash(partition, (char *)&pki_item, sizeof(file_format_t),
           sizeof(pki_item_t));
  if (status != ESP_AT_SUB_OK) {
    dprintf("Failed to write PKI item header, error 0x%08x occured.\n", status);
    return status;
  }

  // Now write the actual PKI data.
  status = writeSysFlash(partition, pkiBuffer,
           sizeof(file_format_t) + sizeof(pki_item_t), length);
  if (status != ESP_AT_SUB_OK) {
    dprintf("Failed to write PKI fata, error 0x%08x occured.\n", status);
    return status;
  }

  return status;
}

/*******************************************************************************
 *
 * Here we are going to write a PKI item to the specified partitions.
 * Currently we don't really care what is in there already, that should have
 * been checked and dealt with by the caller before calling this function.
 *
 * @param[in] - partition
 *    Specifies to what partition we intend to write to.

 * @param[in] - pkiBuffer
 *    A pointer to the PKI item that we want to write to the partition.
 *
 * @param[in] - length
 *    The length of the supplied PKI item
 *
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::writePkiItem(uint32_t partition, char *pkiBuffer,
            size_t length) {

  return writePkiItem(partition, (const char *)pkiBuffer, length);
}

/*******************************************************************************
 *
 * Update the PKI data of in a specified partition. Here we first compare the
 *    already existing PKI data with the new data and only write new data
 *    if they differ.
 *
 * @param[in] - partition
 *    Specifies to what partition we intend to write to.

 * @param[in] - pkiBuffer
 *    A pointer to the PKI item that we want to write to the partition.
 *
 * @param[in] - length
 *    The length of the supplied PKI item
 *
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::updatePkiItem(uint32_t partition,
            const char *pkiBuffer, size_t length) {
  at_status_t status;

  bool result;
  status = comparePkiItem(partition, pkiBuffer, &result, length);

  switch (status) {
    case ESP_AT_SUB_OK:
      if (result) {
        dprintf("PKI Items equal, no need to write a new item.\n", NULL);
        status = ESP_AT_SUB_OK;
      } else {
        dprintf("PKI Items not equal, need to write a new item.\n", NULL);
        status = writePkiItem(partition, pkiBuffer, length);
        dprintf("OKI write status is 0x%08x\n", NULL);
      }
    break;

    case ESP_AT_SUB_CMD_INVALID_PKI_PART:
      dprintf("Invalid PKI partition, need to write a new partition data.\n", NULL);
      status = writePkiItem(partition, pkiBuffer, length);
    break;

    default:
    break;
  }

  return status;
}

/*******************************************************************************
 *
 * Update the PKI data of in a specified partition. Here we first compare the
 *    already existing PKI data with the new data and only write new data
 *    if they differ.
 *
 * @param[in] - partition
 *    Specifies to what partition we intend to write to.

 * @param[in] - pkiBuffer
 *    A pointer to the PKI item that we want to write to the partition.
 *
 * @param[in] - length
 *    The length of the supplied PKI item
 *
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::updatePkiItem(uint32_t partition,
            char *pkiBuffer, size_t length) {

  return updatePkiItem(partition, (const char *)pkiBuffer, length);
}
/*******************************************************************************
 *
 * Returns the textual name of a specific partition. If the partition is is
 * incorrect, NULL is returned.
 *
 * @param[in] - partition
 *   An integer specifying the partition to read from.
 *
 * @return - A pointer to the textual information of the given partition.
 *           NULL is returned if the partition id is ioncorrect.
 *
 ******************************************************************************/
const char *MqttCertMgmt::getPartitionName(uint32_t partition) {
  if (partition >= sizeof(pkiTypes))
    return NULL;
  return mqtt_parts[partition];
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
  at_status_t status;
  file_format_t ff;
  bool validPartition;

  // First make sure it is a valid item, otherwise its no use trying.
  status = checkIfValid(partition, &validPartition);
  if (status != ESP_AT_SUB_OK)
    return status;
  if (!validPartition)
    return ESP_AT_SUB_CMD_INVALID_PKI_PART;

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
 * Erase the specified partition.
 *
 * @param[in] - partition
 *   An integer specifying the partition to erase.
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::erasePartition(uint32_t partition) {
  at_status_t status;

  snprintf(pBuff, 128, "=0,\"%s\"", mqtt_parts[partition]);
  status = _at->sendCommand(AT_CMD_SYSFLASH, pBuff, NULL);

  // We need to avoid interupting the ESP-AT device while it is erasing
  // partitions, otherwise we end up with corrupt data in the partition and
  // an unresponsive device.
  delay(1000);

  return status;
}

/*******************************************************************************
 *
 * Write certificate/key data to the flash using AT command +SYSFLASH.
 *
 * @param[in] - partition
 *   An integer specifying the partition to read from.
 *
 * @param[in] - buffer
 *   Pointer to a buffer with the certificate data to write to FLASH.
 *
 * @param[in] - offset
 *   Offset in the flash partition to start writing to.
 *
 * @param[in] - length
 *   This is the number of bytes that should be written into FLASH.
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::writeSysFlash(uint32_t partition, const char *buffer,
            uint32_t offset, size_t length) {
  at_status_t status;
  size_t len = length;
  const char *buf = buffer;

  snprintf(pBuff, 128, "=1,\"%s\",%d,%d",
           mqtt_parts[partition], offset, length);
  status = _at->sendCommand(AT_CMD_SYSFLASH, pBuff, NULL);
  if (status != ESP_AT_SUB_OK)
    return status;

  status = _at->waitPrompt();
  if (status != ESP_AT_SUB_OK)
    return status;

  while (len--) {
    _at->write(*buf++);
  }
  return ESP_AT_SUB_OK;
}

/*******************************************************************************
 *
 * Write certificate/key data to the flash using AT command +SYSFLASH.
 *
 * @param[in] - partition
 *   An integer specifying the partition to read from.
 *
 * @param[in] - buffer
 *   Pointer to a buffer with the certificate data to write to FLASH.
 *
 * @param[in] - offset
 *   Offset in the flash partition to start writing to.
 *
 * @param[in] - length
 *   This is the number of bytes that should be written into FLASH.
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::writeSysFlash(uint32_t partition, char *buffer,
            uint32_t offset, size_t length) {
  return writeSysFlash(partition, (const char *)buffer, offset, length);
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
  dprintf("Reading sys flash partition %s, offset %d, length %d\n",
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
  _at->sendString(buff);

  snprintf(buff, MQTT_CERT_BUFFER_LENGTH, "AT%s%s", cmd, param);
  dprintf("S:\'%s\'\n", buff);

  // Ensure that we have received at least a start of the reply within the
  // given timeout.
  uint32_t to = millis();
  while (!_at->available() && (millis() - to < timeout))
    yield();
  if (millis() - to >= timeout)
    return ESP_AT_SUB_CMD_TIMEOUT;

  // Make sure we read in the response identifier (+SYSFLASH:)
  int ix = 0;
  do {
    buff[ix++] = _at->read();
    buff[ix] = 0;   // Always terminate the incoming string
  } while (!strstr(&buff[0], AT_CMD_SYSFLASH_RESP) && (millis() - to < timeout));
  // Check for timeout
  if (millis() - to >= timeout)
    return ESP_AT_SUB_CMD_TIMEOUT;

  // Now get the number of bytes in the response
  ix = 0;
  char ch;
  do {
    ch = _at->read();
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
    ch = _at->read();
    retBuff[ix++] = ch;
  }
  return ESP_AT_SUB_OK;
}

/*******************************************************************************
 *
 * Reads in the first 2 bytes of the partition and makes sure it contains
 * a valid magic word (f1f1). If not the result output parameter is set to
 * false.
 *
 * @param[in] - cmd
 *    This is the command section of the hole AT command.
 *
 * @param[out] - result
 *    The result of the operation. True if a valid magic word is found and
 *    false if not.
 *
 * @return - The status of the operation, See #status_code_e for more information.
 *
 ******************************************************************************/
at_status_t MqttCertMgmt::checkIfValid(uint32_t partition, bool *result) {
  at_status_t status;
  uint16_t check;

  status = readSysFlash(partition, (char *)&check, 0, 2);
  if (status != ESP_AT_SUB_OK) {
    return status;
    *result = false;
  }

  dprintf("Partition magic number is 0x%04x\n", check);
  if (check == FF_MAGIC) {
    *result = true;
  } else {
    *result = false;
  }

  return status;
}

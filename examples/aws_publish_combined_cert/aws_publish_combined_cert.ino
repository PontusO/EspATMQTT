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

/*
 * This example shows how to connect the ESP-AT device to AWS using a TLS
 * transport scheme
 *
 * To test this app you need an aws account and to have a device created in
 * the IoT backend. The generated CA, client cert and key must be copied into
 * the secrets.h file. Make sure the certificate is copied properly and that
 * each line has a \n character appended to it.
 */
#include <EspATMQTT.h>
#include <MqttCertMgmt.h>
#include <AT.h>
#include <WiFiEspAT.h>
#include <ChallengerWiFi.h>
#include <ArduinoJson.h>
#include "secrets.h"

AT_Class atMan;             /**< Create AT manager */
EspATMQTT mqtt(&atMan);     /**< Creates mqtt manager */
MqttCertMgmt cm(&atMan);    /**< And finally the certificate manager */

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  while (!Serial)
    delay(10);

  Serial.begin(115200);
  Serial.println("MQTT test application VX.Y !");

  WiFi.init(&ESP_SERIAL_PORT);

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println();
    Serial.println("Communication with WiFi module failed!");
    Serial.println("Please disconnect board and try again.");
    // don't continue
    while (true);
  }

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

  int status = WiFi.begin(ssid, pass);

  Serial.print("Status: ");
  Serial.println(status);

  // Initialize the MQTT library.
  mqtt.begin();

  // Make sure we have the proper certificates and keys installed.
  // First check the root CA
  status = cm.updatePkiItem(MQTT_CA_PART, amazonRootCA, sizeof(amazonRootCA)-1);
  if (status == ESP_AT_SUB_OK) {
    Serial.printf("PKI Item update in %s successfull !\n", cm.getPartitionName(MQTT_CA_PART));
  } else {
    Serial.printf("PKI Item update in %s FAILED !\n", cm.getPartitionName(MQTT_CA_PART));
  }

  // Now check the client certificate
  cm.updatePkiItem(MQTT_CERT_PART, clientCert, sizeof(clientCert)-1);
  if (status == ESP_AT_SUB_OK) {
    Serial.printf("PKI Item update in %s successfull !\n", cm.getPartitionName(MQTT_CERT_PART));
  } else {
    Serial.printf("PKI Item update in %s FAILED !\n", cm.getPartitionName(MQTT_CERT_PART));
  }

  // And finally check the client key
  cm.updatePkiItem(MQTT_KEY_PART, clientKey, sizeof(clientKey)-1);
  if (status == ESP_AT_SUB_OK) {
    Serial.printf("PKI Item update in %s successfull !\n", cm.getPartitionName(MQTT_KEY_PART));
  } else {
    Serial.printf("PKI Item update in %s FAILED !\n", cm.getPartitionName(MQTT_KEY_PART));
  }

  // The certificate management requires that the correct time has been set
  // So here we start the ESP-AT NTP client.
  mqtt.enableNTPTime(true, got_ntp_time, 2, "us.pool.ntp.org");
}

/**
 * This callback function fires when the NTP client has acquired a valid date/time
 * from a NTP time server.
 */
void got_ntp_time(char *dateTime) {
  mqtt_status_t sc;

  // Amazon requires that we verify the server cert and provide the client cert.
  mqtt.userConfig(DEFAULT_LINK_ID, ESP_MQTT_SCHEME_MQTT_OVER_TLS_VSCPCC, clientID);
  delay(1000);

  // Start a connection sequence. It will most likely be asynchronous due to the time it
  // takes to connect. The callback function "connected" will be called when a connection
  // has been established.
  sc = mqtt.connect(DEFAULT_LINK_ID, mqtt_server, 8883, 1, 50, connected);
  if (sc == ESP_AT_SUB_CMD_CONN_SYNCH) {
    // In case we make a direct connection we end up here.
    Serial.printf("Synchronously connected\n");
    mqtt.subscribeTopic(sub_cb, DEFAULT_LINK_ID, "messages/news");
  } else if (sc == ESP_AT_SUB_CMD_CONN_ASYNCH) {
    Serial.printf("Asynchronously connected, waiting for callback\n");
  } else {
    Serial.printf("An error occured (%08x) during connection!\n", sc);
  }
}

void connected(char *connectionString) {

  Serial.print("Received connection string: ");
  Serial.println(connectionString);

  mqtt.subscribeTopic(sub_cb, DEFAULT_LINK_ID, "messages/news");
}


void sub_cb(char *topic, char *mqttdata) {
  Serial.print(topic);
  Serial.print(" - ");
  Serial.println(mqttdata);
}

void loop() {
  static uint32_t timer1 = millis();
  char mydata[256];
  StaticJsonDocument<1024> doc;

  // The application must call the process function repetively.
  mqtt.process();

  // Just send a short message repetively
  if (millis() - timer1 > 10000 && mqtt.isConnected()) {
    timer1 = millis();

    digitalWrite(LED_BUILTIN, 1);
    char *time;
    mqtt.getNTPTime(&time);

    doc["time"] = time;
    doc["temp"] = String(random(2200, 2300) / 100.0);
    doc["humidity"] = String(random(780, 790) / 10.0) + "%";

    char output[1024];
    serializeJson(doc, output);
    mqtt.pubRaw(DEFAULT_LINK_ID, "messages/log", output);

    digitalWrite(LED_BUILTIN, 0);
  }
}

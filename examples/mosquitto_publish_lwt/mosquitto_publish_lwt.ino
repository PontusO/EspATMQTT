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
 * This app demonstrates how to use the EspATMQTT library by setting up an
 * MQTT client that sends a simple text message to a mosquitto MQTT server
 * but also configures a LWT (Last Will and Testament) message to be sent out
 * by the broker/server when the device can no longer be detected.
 *
 * To test this app out I have used mosquitto on a Ubuntu machine the
 * following way.
 *
 * 1 - Make sure the mosquitto server is running on your machine.
 * 2 - Make a note of the IP address of my Ubuntu machine and enter
 *     this information into the mqtt_server variable below.
 * 3 - Flash your device with this sketch, if you're using the
 *     Challenger RP2040 WiFi/BLE board it should work out of the box.
 *
 * To see what's going on you can use the mosquitto_sub command in an Ubuntu
 * terminal window. Simply type:
 *     mosquitto_sub -t messages/log
 * and you will start receiving json formatted data from your test node.
 *
 *   {"temp":"22.27","humidity":"78.60%"}
 *   {"temp":"22.43","humidity":"78.40%"}
 *   {"temp":"22.04","humidity":"78.00%"}
 *   {"temp":"22.49","humidity":"78.00%"}
 *
 * To receive the LWT message configure another terminal window to receive
 * it:
 *    mosquitto_sub -t messages/lwt
 * With the current keepalive timout it takes up to 2 minutes before the
 * LWT message is received.
 *
 * This test app was written using our Challenger RP2040 WiFi/BLE boards
 * and as such there are some specific code here that only works for that
 * board. But the library is generic and can be used with any ESP-AT based
 * ESP32C3 module.
 */
#include <ChallengerWiFi.h>
#include <AT.h>
#include <EspATMQTT.h>
#include <WiFiEspAT.h>
#include <ArduinoJson.h>

EspATMQTT mqtt;

const char *mqtt_server = "192.168.0.151";

const char ssid[] = "<ssid>";      // Access point credentials
const char pass[] = "<password>";

void setup() {
  // We're gonna do some blinky stuff here, so prepare.....
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 0);

  // Just wait for the USB to connect to the PC
  while (!Serial)
    delay(10);

  Serial.begin(115200);
  Serial.println("Mosquitto MQTT test sketch !");

  // Reset ESP32 and wait for ESP-AT interpreter to start.
  if (Challenger2040WiFi.reset()) {
    Serial.println(F("WiFi Chip reset OK !"));
  } else {
    Serial.println(F("Could not reset WiFi chip !"));
    while(1);
  }

  // Initialize the WiFi stack
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
  mqtt.userConfig(DEFAULT_LINK_ID, ESP_MQTT_SCHEME_MQTT_OVER_TCP, "Challenger_RP2040_WiFi_BLE");
  delay(1000);
  // LWT Messages must be configured before the connection is made.
  mqtt.connectionConfig(DEFAULT_LINK_ID, 120, AT_MQTT_DO_NOT_RETAIN, "messages/lwt", "BYE:0x87EA453F");
  mqtt.connect(DEFAULT_LINK_ID, mqtt_server);
}

void loop() {
  static uint32_t timer1 = millis();
  char mydata[256];
  StaticJsonDocument<1024> doc;

  // When subscribing to messages from the mqtt server the application must call
  // the process function repetively, so in this example this isn't really necessary.
  mqtt.process();

  // Just send a short message repetively
  if (millis() - timer1 > 10000 && mqtt.isConnected()) {
    timer1 = millis();

    digitalWrite(LED_BUILTIN, 1);

    doc["temp"] = String(random(2200, 2300) / 100.0);
    doc["humidity"] = String(random(780, 790) / 10.0) + "%";

    char output[1024];
    serializeJson(doc, output);
    mqtt.pubRaw(DEFAULT_LINK_ID, "messages/log", output);

    digitalWrite(LED_BUILTIN, 0);
  }
}

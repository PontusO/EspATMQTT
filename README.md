# A simple wrapper library for the MQTT functionality in the ESP-AT stack.

This library was developed to support the MQTT stack found in the ESP-AT interpreter. It forms a simple wrapper around the AT commands in order to make it easier to use, instead of sending AT commands over the serial port you can now use a simple C++ class with a well defined API to do the same. All the handling of the serial port is taken care of for you.

## The AT_Class

A simple AT handler class supports all the low level stuff towards the ESP-AT device. It will handle everything from the simple sending of data up to waiting for asynchronous responses as well as dealing with serial timeouts.

## The EspATMQTT Class

This is the cruncher of the library. It forms a well defined API and lets the user focus on developing his/hers application rather than having to deal with serial timeouts and other hardware releated bits and bobs.

### Connections

In its most simple form only three lines is necessary to connect to a MQTT server/broker.

```
  mqtt.begin();
  mqtt.userConfig(DEFAULT_LINK_ID, ESP_MQTT_SCHEME_MQTT_OVER_TCP, "Challenger_RP2040_WiFi_BLE");
  mqtt.connect(DEFAULT_LINK_ID, "192.168.0.151");
```

That is it. you can now make your subscriptions or start sending data as easy as 1-2-3.

### Subscriptions

Subscriptions are easily handled by subscribing to a topic and for every message that you receive you will receive a callback that can be used to handle the incoming data. A perfect way to handle control parameters and other run time relevant data.

```
void sub_cb(char *topic, char *data) {
  Serial.printf("Data incoming:. topic: '%s', data '%s'\n", topic, data);
}

  mqtt.subscribeTopic(sub_cb, DEFAULT_LINK_ID, "messages/news");
  mqtt.subscribeTopic(sub_cb, DEFAULT_LINK_ID, "messages/bulletin");
```

### Publish data

And it is of course just as easy to send data. Two different methods can be used. If you only have small strings that need to be sent use the pubString() method. This method is however not so convenient if you have a little more data to send. In this case you can use the pubRaw() method. This method makes it much easier to publish larger json string or binary data.

```
  // Publish a short string to the server/broker
  mqtt.pubString(DEFAULT_LINK_ID, "messages/log", "Error 101 - Could not find the file !");
  mqtt.pubString(DEFAULT_LINK_ID, "messages/data", "{"time":"Thu Jul  7 09:30:37 2022"\\,"temp":"22.33","humidity":"78.30%"}");
```

Notice the double escaped comma sign in the json formated string above. This is required when using the pubString method but not when using the pubRaw() method.

Here's a simple example on how to create and send a json formated response using the pubRaw() method.

```
  StaticJsonDocument<1024> doc;

  char *time;
  mqtt.getNTPTime(&time);

  doc["time"] = time;
  doc["temp"] = String(random(2200, 2300) / 100.0);
  doc["humidity"] = String(random(780, 790) / 10.0) + "%";

  char output[1024];
  serializeJson(doc, output);
  mqtt.pubRaw(DEFAULT_LINK_ID, "messages/data", output);
```

## Security

MQTT relies on the TCP transport protocol. By default, TCP connections do not use an encrypted communication. To encrypt the whole MQTT communication, many MQTT brokers (such as HiveMQ and Mosquitto) allow use of TLS instead of plain TCP. If you use the username and password fields of the MQTT CONNECT packet for authentication and authorization mechanisms, you should strongly consider using TLS.

Port 8883 is standardized for a secured MQTT connection. The standardized name at IANA is “secure-mqtt”. Port 8883 is exclusively reserved for MQTT over TLS.

The ESP-AT MQTT client also supports the use of TLS to protect your data and this wrapper of course also supports this. A number of conenction methods and certificate validation schemes can be used (See #mqtt_scheme_e) with this implementation.

Selecting a connection scheme is a simple oneliner:
```
    mqtt.userConfig(DEFAULT_LINK_ID, ESP_MQTT_SCHEME_MQTT_OVER_TLS_VSCPCC, "Challenger_RP2040_WiFi_BLE");
```

Validation of certificates requires that the system can tell the data and time and the library implements methods for enabling an ESP-AT internal NTP client that automatically connects and get the current date and time.


Enabling the NTP client is also extremely simple, simply enter:
```
    mqtt.enableNTPTime(true, ntp_time_received, 2, "us.pool.ntp.org");
```
When the client has acquired a valid date and time the ```ntp_time_received``` function will be called in which you can go ahead and connect to your secure MQTT server/broker.

Certificate, key and CA can be uploaded to the device to support any IoT cloud vendor. We've tested the library and ESP-AT fw with Amazon AWS, Microsoft Azure and a bunch of local test servers using different security schemes.

## License

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

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
const char *mqtt_server = "<hash>-ats.iot.us-west-2.amazonaws.com";
const char *clientID = "Challenger_RP2040_WiFi_BLE";

const char ssid[] = "<Your SSID>";              // Our local test network
const char pass[] = "<Your Password>";

/* These are not complete certificates and keys, you need to obtain your
 * own from the Amazon IoT backend. */
const char amazonRootCA[] = \
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
  "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
  "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
  "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
  "-----END CERTIFICATE-----";

const char clientCert[] = \
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDWTCCAkGgAwIBAgIUPQBqzp051XVGgrb8MPPhakJ9AxowDQYJKoZIhvcNAQEL\n"
  "BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n"
  "SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTIyMDcwNDEzNTIz\n"
  "71p+fB3kmOZLGxQJSy9mhTCuOla6S9yKup4jUOBzZfPvUfE9OYpaRiKLCqlj\n"
  "-----END CERTIFICATE-----\n";

 const char clientKey[] = \
  "-----BEGIN RSA PRIVATE KEY-----\n"
  "MIIEowIBAAKCAQEAuka7uMYofLZkbBzL6u+3KFL6pY/YBYe2oZH6TzDye8BXh/+1\n"
  "gu408QFS83g7gk1hIcNNwEQLT106o+kYyxrkz0zviDcU7+LBCOhY2/pY5G5sfEpA\n"
  "zKIADFCUx5AdhF3hU/JWwJXjkqOYTsmrDh/NclpJDNag59iAPHyjhKV6Hs+rRY2l\n"
  "/H2XN0QArmQcsFngRVgFU1IMXDV06L6C7rmrk0JshqHCaeBC054v\n"
  "-----END RSA PRIVATE KEY-----\n";

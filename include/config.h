#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

#define MAX_QUEUE_LENGTH 10

// A7670 pins Modem pins
#define MODEM_RST 5
#define MODEM_PWRKEY 4
#define MODEM_TX 26
#define MODEM_RX 27
#define DEBUG 1

#if DEBUG == 1
#define debug(x) Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

// SIM card PIN (leave empty, if not defined)
const char simPIN[] = "";

// Your GPRS credentials, if any

const char apn[] = "web";
// const char apn[] = "jawalnet.com.sa";
const char gprsUser[] = "";
const char gprsPass[] = "";

// Server details
const char server[] = "172.206.216.191";
const char resource[] = "/sms";
const int port = 3000;

#endif
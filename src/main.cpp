
#include "main.h"
#include <ArduinoHttpClient.h>

bool httpRequest(String &httpRequestData);
bool networkSetup(void);
void readIncomingMessage(void);

void procSmsMsgTask(void *parameter)
{
  SmsData receivedData;
  for (;;)
  {
    if (xQueueReceive(smsDataQueue, &receivedData, pdMS_TO_TICKS(100)) == pdTRUE)
    {
      // debugln("\nReceived message::");
      // debugln(receivedData.msgId);
      // debugln(receivedData.sender);
      // debugln(receivedData.timestamp);
      // debugln(receivedData.message);

      JsonDocument doc;
      doc["msgId"] = receivedData.msgId;
      doc["sender"] = receivedData.sender;
      doc["timestamp"] = receivedData.timestamp;
      doc["message"] = receivedData.message;

      String httpRequestData;
      httpRequestData.reserve(2048);
      serializeJson(doc, httpRequestData);
      if (modem.isGprsConnected())
        httpRequest(httpRequestData);
      else
        debugln(F("[ERROR] GPRS Disconnected, Cannot make request!"));
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup()
{
#if DEBUG == 1
  Serial.begin(115200);
#endif

  // Set modem reset, enable, power pins
  pinMode(MODEM_RST, OUTPUT);
  digitalWrite(MODEM_RST, LOW);
  delay(100);
  digitalWrite(MODEM_RST, HIGH);
  delay(2600);
  digitalWrite(MODEM_RST, LOW);
  pinMode(MODEM_PWRKEY, OUTPUT);
  digitalWrite(MODEM_PWRKEY, LOW);
  delay(100);
  digitalWrite(MODEM_PWRKEY, HIGH);
  delay(100);
  digitalWrite(MODEM_PWRKEY, LOW);

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(2000);

  // Create Queue
  smsDataQueue = xQueueCreate(MAX_QUEUE_LENGTH, sizeof(SmsData));

  xTaskCreate(procSmsMsgTask,
              "HTTPSMsg",
              4096 * 4,
              nullptr,
              1,
              nullptr);

  // Restart SIMA7670 module, it takes quite some time
  // To skip it, call init() instead of restart()
  debugln(F("\n\n[STEP::1]Initializing modem..."));
  modem.restart();

  // Unlock your SIM card with a PIN if needed
  if (strlen(simPIN) && modem.getSimStatus() != 3)
  {
    modem.simUnlock(simPIN);
  }
  SerialAT.println("AT+CMGF=1"); // Configuring TEXT mode
  delay(200);
  SerialAT.println("AT+CNMI=2,2,0,0,0"); // Decides how newly arrived SMS messages should be handled
  delay(200);
  SerialAT.println("AT+CMGD=,4"); // Clearing all the existing messages
  delay(200);
  networkSetup();
  debugln(F("[STEP::5]Waiting for SMS message..."));
}

void loop()
{
  readIncomingMessage();
  vTaskDelay(pdMS_TO_TICKS(50));
}

void readIncomingMessage()
{
  if (!SerialAT.available())
    return;

  String response = SerialAT.readStringUntil('\n');
  // debugln(response);

  if (!response.startsWith("+CMT:"))
    return;

  // for the first quote message
  int quoteStart = response.indexOf('"');
  int quoteEnd = response.indexOf('"', quoteStart + 1);

  if (quoteStart == -1 && quoteEnd == -1)
    return;

  SmsData data;
  strcpy(data.sender, response.substring(quoteStart + 1, quoteEnd).c_str());

  // Skip this sequence
  quoteStart = response.indexOf('"', quoteEnd + 1);
  quoteEnd = response.indexOf('"', quoteStart + 1);

  // Extract timestamp
  quoteStart = response.indexOf('"', quoteEnd + 1);
  quoteEnd = response.indexOf('"', quoteStart + 1);

  if (quoteStart == -1 && quoteEnd == -1)
    return;

  msgCount++;

  data.msgId = msgCount;
  strcpy(data.timestamp, response.substring(quoteStart + 1, quoteEnd).c_str());

  String text = SerialAT.readStringUntil('\n');
  text.trim();
  debugln(text);

  strcpy(data.message, text.c_str());
  if (xQueueSend(smsDataQueue, &data, portMAX_DELAY) != pdTRUE)
  {
    debugln("Failed to send to queue");
  }
}

bool networkSetup()
{
  bool ret;
  ret = modem.setNetworkMode(MODEM_NETWORK_AUTO);
  if (modem.waitResponse(10000L) != 1)
  {
    debugln("setNetworkMode faill");
    return false;
  }

  debug("[STEP::2]Waiting for network...");
  if (!modem.waitForNetwork())
  {
    debugln(" fail");
    delay(10000);
    return false;
  }
  debug(" success");

  if (modem.isNetworkConnected())
  {
    debugln(F(" Network connected"));
  }
  debug(F("[STEP::3]Connecting to "));
  debug(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass))
  {
    debugln(" fail");
    delay(10000);
    return false;
  }
  debugln(F(" success"));

  if (!modem.isGprsConnected())
  {
    delay(10000);
    return false;
  }

  debugln(F("[STEP::4]GPRS connected"));
  return true;
}
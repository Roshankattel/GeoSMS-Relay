
#include "main.h"

void readIncomingMessage();

void setup()
{
  Serial.begin(115200); // Set console baud rate

  // Create Queue
  smsDataQueue = xQueueCreate(MAX_QUEUE_LENGTH, sizeof(SmsData));

  Serial.println("Start Sketch");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

#ifdef BOARD_POWERON_PIN
  pinMode(BOARD_POWERON_PIN, OUTPUT);
  digitalWrite(BOARD_POWERON_PIN, HIGH);
#endif

  // Set modem reset pin ,reset modem
  pinMode(MODEM_RESET_PIN, OUTPUT);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);
  delay(100);
  digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL);
  delay(2600);
  digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);

  pinMode(BOARD_PWRKEY_PIN, OUTPUT);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);
  delay(100);
  digitalWrite(BOARD_PWRKEY_PIN, HIGH);
  delay(100);
  digitalWrite(BOARD_PWRKEY_PIN, LOW);

  // Check if the modem is online
  Serial.println("Start modem...");

  int retry = 0;
  while (!modem.testAT(1000))
  {
    Serial.println(".");
    if (retry++ > 10)
    {
      digitalWrite(BOARD_PWRKEY_PIN, LOW);
      delay(100);
      digitalWrite(BOARD_PWRKEY_PIN, HIGH);
      delay(1000);
      digitalWrite(BOARD_PWRKEY_PIN, LOW);
      retry = 0;
    }
  }
  Serial.println();

  // Check if SIM card is online
  SimStatus sim = SIM_ERROR;
  while (sim != SIM_READY)
  {
    sim = modem.getSimStatus();
    switch (sim)
    {
    case SIM_READY:
      Serial.println("SIM card online");
      break;
    case SIM_LOCKED:
      Serial.println("The SIM card is locked. Please unlock the SIM card first.");
      // const char *SIMCARD_PIN_CODE = "123456";
      // modem.simUnlock(SIMCARD_PIN_CODE);
      break;
    default:
      break;
    }
    delay(1000);
  }

  if (!modem.setNetworkMode(MODEM_NETWORK_AUTO))
  {
    Serial.println("Set network mode failed!");
  }
  String mode = modem.getNetworkModes();
  Serial.print("Current network mode : ");
  Serial.println(mode);

  // Check network registration status and network signal status
  int16_t sq;
  Serial.println("Wait for the modem to register with the network.");
  RegStatus status = REG_NO_RESULT;
  while (status == REG_NO_RESULT || status == REG_SEARCHING || status == REG_UNREGISTERED)
  {
    status = modem.getRegistrationStatus();
    switch (status)
    {
    case REG_UNREGISTERED:
    case REG_SEARCHING:
      sq = modem.getSignalQuality();
      Serial.printf("[%lu] Signal Quality:%d ", millis() / 1000, sq);
      delay(1000);
      break;
    case REG_DENIED:
      Serial.println("Network registration was rejected, please check if the APN is correct");
      return;
    case REG_OK_HOME:
      Serial.println("Online registration successful");
      break;
    case REG_OK_ROAMING:
      Serial.println("Network registration successful, currently in roaming mode");
      break;
    default:
      Serial.printf("Registration Status:%d\n", status);
      delay(1000);
      break;
    }
  }
  Serial.println();

  Serial.printf("Registration Status:%d\n", status);
  delay(1000);

  String ueInfo;
  if (modem.getSystemInformation(ueInfo))
  {
    Serial.print("Inquiring UE system information:");
    Serial.println(ueInfo);
  }

  if (!modem.enableNetwork())
  {
    Serial.println("Enable network failed!");
  }

  delay(5000);

  String ipAddress = modem.getLocalIP();
  Serial.print("Network IP:");
  Serial.println(ipAddress);

  SerialAT.println("AT+CMGF=1"); // Configuring TEXT mode
  delay(200);
  SerialAT.println("AT+CNMI=2,2,0,0,0"); // Decides how newly arrived SMS messages should be handled
  delay(200);
  SerialAT.println("AT+CMGD=,4"); // Clearing all the existing messages
  delay(200);
  Serial.println("Waiting for SMS ...");
}

void loop()
{
  // Serial.print AT
  readIncomingMessage();
  if (millis() - lastReceivedTime > 1000)
    sendData = true;

  if (sendData)
  {
    if ((xQueueReceive(smsDataQueue, &receivedData, pdMS_TO_TICKS(200)) == pdTRUE))
    {
      // Serial.println("\n[Received::]");
      // Serial.print("Message Id:");
      // Serial.println(receivedData.msgId);
      // Serial.print("Sender:");
      // Serial.println(receivedData.sender);
      // Serial.print("Timestamp:");
      // Serial.println(receivedData.timestamp);
      // Serial.print("Message:");
      // Serial.println(receivedData.message);
      String httpRequestData =
          "{\"msgId\":" + String(receivedData.msgId) +
          ",\"sender\":\"" + String(receivedData.sender) +
          "\",\"timestamp\":\"" + String(receivedData.timestamp) +
          "\",\"message\":\"" + String(receivedData.message) +
          "\"}";
      if (!httpRequest(httpRequestData))
        Serial.println("[ERROR] GPRS Disconnected, Cannot make request!");
    }
    else
      sendData = false;
  }
}

void readIncomingMessage()
{
  if (!SerialAT.available())
    return;

  String response = SerialAT.readStringUntil('\n');
  // Serial.println(response);

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
  // Serial.println(text);

  strcpy(data.message, text.c_str());
  lastReceivedTime = millis();
  if (xQueueSend(smsDataQueue, &data, portMAX_DELAY) != pdTRUE)
  {
    Serial.println("Failed to send to queue");
  }
}
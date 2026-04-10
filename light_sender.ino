#include <BLEDevice.h>
#include <BLEClient.h>
#include "mbedtls/aes.h"

// ================= SENSOR =================
#define LIGHT_PIN 13

// ================= BLE =================
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-ab12-ab12-ab12-abcdef123456"
#define RECEIVER_MAC "3c:8a:1f:77:7e:16"

BLEClient *pClient;
BLERemoteCharacteristic *pRemoteChar;

// ================= RTOS =================
QueueHandle_t dataQueue;

// ================= AES =================
unsigned char key[17] = "1234567890ABCDEF";

// ================= TASK 1 =================
// 🌞 Light Sensor Task
void Light_Task(void *pvParameters) {

  uint8_t state;

  while (1) {

    state = digitalRead(LIGHT_PIN);

    Serial.print("[LIGHT] State: ");
    Serial.println(state); // 0 or 1

    xQueueSend(dataQueue, &state, 0);

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// ================= TASK 2 =================
// 🔐 AES + BLE Task
void AES_BLE_Task(void *pvParameters) {

  uint8_t receivedState;
  unsigned char input[16] = {0};
  unsigned char encrypted[16];

  mbedtls_aes_context aes;
  mbedtls_aes_init(&aes);

  while (1) {

    if (xQueueReceive(dataQueue, &receivedState, portMAX_DELAY)) {

      if (!pClient->isConnected()) {
        Serial.println("[BLE] Reconnecting...");
        pClient->connect(BLEAddress(RECEIVER_MAC));
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }

      memset(input, 0, 16);
      input[0] = receivedState;  // only 1 byte needed

      // Encrypt
      mbedtls_aes_setkey_enc(&aes, key, 128);
      mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, input, encrypted);

      if (pRemoteChar != nullptr) {
        pRemoteChar->writeValue(encrypted, 16);
        Serial.println("[BLE] Sent encrypted light state");
      }
    }
  }

  mbedtls_aes_free(&aes);
}

// ================= BLE CONNECT =================
void connectBLE() {

  pClient = BLEDevice::createClient();

  Serial.println("[BLE] Connecting...");
  pClient->connect(BLEAddress(RECEIVER_MAC));

  BLERemoteService *pService = pClient->getService(SERVICE_UUID);
  pRemoteChar = pService->getCharacteristic(CHARACTERISTIC_UUID);

  Serial.println("[BLE] Connected!");
}

// ================= SETUP =================
void setup() {

  Serial.begin(115200);
  pinMode(LIGHT_PIN, INPUT);

  BLEDevice::init("ESP32-Light-Sender");
  connectBLE();

  dataQueue = xQueueCreate(5, sizeof(uint8_t));

  xTaskCreatePinnedToCore(Light_Task, "Light Task", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(AES_BLE_Task, "AES BLE", 4096, NULL, 1, NULL, 1);
}

void loop() {}
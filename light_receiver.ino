#include <BLEDevice.h>
#include <BLEServer.h>
#include "mbedtls/aes.h"

#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcd1234-ab12-ab12-ab12-abcdef123456"

unsigned char key[17] = "1234567890ABCDEF";

class Callbacks : public BLECharacteristicCallbacks {

  void onWrite(BLECharacteristic *pCharacteristic) {

    String value = pCharacteristic->getValue();

    if (value.length() == 16) {

      unsigned char buffer[16];
      uint8_t state;

      memcpy(buffer, value.c_str(), 16);

      mbedtls_aes_context aes;
      mbedtls_aes_init(&aes);

      // Decrypt
      mbedtls_aes_setkey_dec(&aes, key, 128);
      mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT,
                            buffer, buffer);

      state = buffer[0];

      Serial.print("[RECEIVED LIGHT]: ");
      Serial.println(state);

      mbedtls_aes_free(&aes);
    }
  }
};

void setup() {
  Serial.begin(115200);

  BLEDevice::init("ESP32-Light-Receiver");

  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pChar = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );

  pChar->setCallbacks(new Callbacks());

  pService->start();
  BLEDevice::getAdvertising()->start();

  Serial.println("[BLE] Receiver Ready...");
}

void loop() {}
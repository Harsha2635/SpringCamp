#include <BLEDevice.h>

void setup() {
  Serial.begin(115200);
  BLEDevice::init("");
  Serial.println(BLEDevice::getAddress().toString().c_str());
}

void loop() {}
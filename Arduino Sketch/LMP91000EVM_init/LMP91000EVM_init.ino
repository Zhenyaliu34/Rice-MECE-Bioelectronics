#include <Wire.h>
#define LMP91000_ADDR 0x48

void setup() {
  Wire.begin();                // Initialize I²C
  Serial.begin(115200);

  // Quick comms check – read STATUS (address 0x00)
  Wire.beginTransmission(LMP91000_ADDR);
  Wire.write(0x00);
  if (Wire.endTransmission() == 0) {
    Wire.requestFrom(LMP91000_ADDR, 1);
    if (Wire.available()) {
      byte status = Wire.read();
      Serial.print("LMP91000 status: 0x");
      Serial.println(status, HEX);
    }
  }
  
  
}

void loop() {
  // Required, even if empty.
  // Put periodic reads or other tasks here.
}

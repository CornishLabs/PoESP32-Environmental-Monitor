#include <Wire.h>

void setup() {
  Wire.begin(16, 17);
  Serial.begin(115200);
  delay(1000);
  Serial.println("I2C Scanner");
  
  for(byte addr=1; addr<127; addr++){
    Wire.beginTransmission(addr);
    if(Wire.endTransmission()==0){
      Serial.print("Found device at 0x");
      Serial.println(addr, HEX);
    }
  }
}

void loop() {}
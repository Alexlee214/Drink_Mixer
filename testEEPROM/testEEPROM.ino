#include <Wire.h>

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Serial.begin(9600);
  for(int i = 0; i < 100; i++){
    Serial.print("writing to location: ");
    Serial.print(i);
    saveEEPROM(0x50, i, 12);
    Serial.print(", reading value: ");
    Serial.println(readEEPROM(0x50, i));
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}

void saveEEPROM( int deviceaddress, unsigned int eeaddress, byte data ){
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.write(data);
    Wire.endTransmission();
    delay(10);
}

byte readEEPROM( int deviceaddress, unsigned int eeaddress ) {
    byte rdata = 0xFF;
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
    Wire.requestFrom(deviceaddress,1);
    if (Wire.available()) rdata = Wire.read();
    return rdata;
}

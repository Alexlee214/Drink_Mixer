#include <Wire.h>

//drink eeprom has i2c address 80(1010000)
const byte drinkDisk = 0x50;

const byte motor1 = 4;
const byte motor2 = 5;
const byte motor3 = 6;
const byte motor4 = 7;
const byte motor5 = 8;

//0 means either no drink received or drink done dispensing
byte drinkAddr = 0;

void setup() {
  //join slave as address 8
  Serial.begin(9600);
  Wire.begin(8);
  
  //register events
  Wire.onReceive(receiveDrink);
  Wire.onRequest(sendFlag);
  initializeEEPROM();
  pinMode(motor1, OUTPUT);
  pinMode(motor2, OUTPUT);
  pinMode(motor3, OUTPUT);
  pinMode(motor4, OUTPUT);
  pinMode(motor5, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(drinkAddr != 0){
    makeDrink();
  }

  Serial.println(drinkAddr);
}



void receiveDrink(int howMany){
  if(drinkAddr == 0){
    while(Wire.available()){
      //extract the drink number(address in eeprom) from the master device
      drinkAddr = Wire.read();
    }
  }
}



void driveMotor(int motorNum, long volume){
  
  long delayTime = 0;

  //pump dispenses 15 ml in 1 second, accurate down to 6 ml
  //also dispenses 5 ml in 300 ms
  if(volume > 5){
    delayTime = (1000 * volume) / 15;
  }
  else{
    delayTime = (300 * volume) / 5;
  }

  Serial.print(drinkAddr);
  Serial.print("driving motor :");
  Serial.print(motorNum);
  Serial.print(", dispensing ");
  Serial.print(volume);
  Serial.print(" ml, delay time is: ");
  Serial.println(delayTime);

  switch(motorNum){
    case 1: digitalWrite(motor1, HIGH);
            delay(delayTime);
            digitalWrite(motor1, LOW);
            break;
    case 2: digitalWrite(motor2, HIGH);
            delay(delayTime);
            digitalWrite(motor2, LOW);
            break;
    case 3: digitalWrite(motor3, HIGH);
            delay(delayTime);
            digitalWrite(motor3, LOW);
            break;
    case 4: digitalWrite(motor4, HIGH);
            delay(delayTime);
            digitalWrite(motor4, LOW);
            break;
    case 5: digitalWrite(motor5, HIGH);
            delay(delayTime);
            digitalWrite(motor5, LOW);
            break;
    default: break;
                      
  }
}


void makeDrink(){

  //if address is multiple of 5, auto mode, make drink
  if(drinkAddr % 5 == 0){
    //stores the amount that each pump should dispense
    long drinkRatio[5] = {0, 0, 0, 0, 0};
    //extract values from external eeprom
    getDrinkRatio(drinkDisk, drinkAddr, drinkRatio);
    //drive motors to dispense drink
    dispenseDrink(drinkRatio);
    
  //if address notr multiple of 5, in another mode, manual mode
  }else{
    Serial.println("MANUAL");
    //addr = 6: motor 5, addr = 4: motor 4, addr = 3: motor 3, addr = 2: motor 2, addr = 1: motor 1,
    if(drinkAddr == 6){
      driveMotor(5, 5);
    }
    else{
      driveMotor(drinkAddr, 5);
    }
    drinkAddr = 0;
  }
  
}

//get the drink ratio of the drink from the EEPROM and sets the drinkRatio array
void getDrinkRatio(byte diskAddr, byte drinkAddr, long* drinkRatio){
  //begin transmission with drink disk
  Wire.beginTransmission(diskAddr);
  //the location to read from
  Wire.write(drinkAddr - 1);
  Wire.endTransmission();

  //request 5 bytes from the eeprom, to extract the ratio for 1 particular drink
  Wire.requestFrom(diskAddr,6);

  byte countMem = 0;
  while(Wire.available()){
    drinkRatio[countMem] = Wire.read();
    countMem++;
  }
  delay(50);
}


void dispenseDrink(long* drinkRatio){
  
  //iterate through all drinks
  for(int countDrink = 1; countDrink < 6; countDrink++){
    //dispense if not 0 ml
      Serial.print("driving motor: ");
      Serial.print(countDrink);
      Serial.print(" Amount: ");
      Serial.println(drinkRatio[countDrink]);
      
      driveMotor(countDrink, drinkRatio[countDrink]);
      delay(50);
  }

  //drink is done, set flag to done
  delay(50);
  drinkAddr = 0;
}


//sends "t" to master if drink is done, and resets the done flag
//otherwise sends "f"
void sendFlag(){
  if(drinkAddr == 0){
    Wire.write("t");
  }else{
    Wire.write("f");
  }
}


//drives the motor to dispense a certain amonunt of a particular drink



void initializeEEPROM(){
  // 5 drinks in EEPROM
  saveEEPROM(0, 2, drinkDisk);
  
  saveEEPROM(5, 77, drinkDisk);
  saveEEPROM(6, 80, drinkDisk);
  saveEEPROM(7, 19, drinkDisk);
  saveEEPROM(8, 10, drinkDisk);
  saveEEPROM(9, 11, drinkDisk);
  
  saveEEPROM(10, 12, drinkDisk);
  saveEEPROM(11, 13, drinkDisk);
  saveEEPROM(12, 14, drinkDisk);
  saveEEPROM(13, 15, drinkDisk);
  saveEEPROM(14, 16, drinkDisk);

  saveEEPROM(15, 17, drinkDisk);
  saveEEPROM(16, 18, drinkDisk);
  saveEEPROM(17, 19, drinkDisk);
  saveEEPROM(18, 20, drinkDisk);
  saveEEPROM(19, 21, drinkDisk);
}


void saveEEPROM(byte data, uint8_t eeaddress, byte device) {
  Wire.beginTransmission(device);
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.write(data);
    Wire.endTransmission();
    delay(10);
}


byte readEEPROM(uint8_t eeaddress, byte device) {
  byte rdata = 0xFF;
    Wire.beginTransmission(device);
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
    Wire.requestFrom(device,1);
    if (Wire.available()) rdata = Wire.read();
    return rdata;
}

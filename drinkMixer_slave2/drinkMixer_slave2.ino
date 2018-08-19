#include <Wire.h>

typedef struct{
  byte rVal;
  byte gVal;
  byte bVal;
} color;



//drink eeprom has i2c address 80(1010000)
const byte drinkDisk = 0x50;

const byte ledG = 11;
const byte ledR = 10;
const byte ledB = 9;
const byte motor1 = 4;
const byte motor2 = 5;
const byte motor3 = 6;
const byte motor4 = 7;
const byte motor5 = 8;

const byte randomPin = A0;

//0 means either no drink received or drink done dispensing
uint8_t drinkAddr = 0;

unsigned  long ledTimer = millis();

color myColor = {0, 0, 0};


void setup() {
  //join slave as address 8
  Wire.begin(8);
  
  //register events
  Wire.onReceive(receiveDrink);
  Wire.onRequest(sendFlag);
  //initializeEEPROM();
  pinMode(motor1, OUTPUT);
  pinMode(motor2, OUTPUT);
  pinMode(motor3, OUTPUT);
  pinMode(motor4, OUTPUT);
  pinMode(motor5, OUTPUT);
  randomSeed(analogRead(randomPin));

  pinMode(ledR, OUTPUT);
  analogWrite(ledR, 0);
  pinMode(ledG, OUTPUT);
  analogWrite(ledG, 0);
  pinMode(ledB, OUTPUT);
  analogWrite(ledB, 0);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(drinkAddr != 0){
    makeDrink();
  } else if(millis() - ledTimer > 2500 && millis() - ledTimer < 2700){
    fadeOutColor(&myColor);
  }

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
  /*
  //pump dispenses 15 ml in 1 second, accurate down to 6 ml
  //also dispenses 5 ml in 300 ms
  if(volume > 5){
    delayTime = (950 * volume) / 10;
  }
  else{
    delayTime = 90 * volume;
  }
*/
  

  //dispensing 10ml - pump1:950ms pump2:950ms pump3:900ms pump4:1150ms pump5:950ms
  //dispensing 5ml - pump1:450ms pump2:450ms pump3:400ms pump4:550ms: pump5:450ms
  if(volume > 5){
    switch(motorNum){
      case 1:
      case 2:
      case 5: delayTime = (950 * volume) / 10;
              break;
      case 3: delayTime = (900 * volume) / 10;
              break;       
      case 4: delayTime = (1150 * volume) / 10;
              break;
      default: break;       
    }
  }else{
    switch(motorNum){
      case 1:
      case 2:
      case 5: delayTime = (450 * volume) / 5;
              break;
      case 3: delayTime = (400 * volume) / 5;
              break;       
      case 4: delayTime = (550 * volume) / 5;
              break;
      default: break;       
    }
  }

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
    showRandomColor();
    //stores the amount that each pump should dispense
    byte drinkRatio[5] = {0, 0, 0, 0, 0};
    //extract values from external eeprom
    getDrinkRatio(drinkDisk, drinkAddr, drinkRatio);
    //drive motors to dispense drink
    dispenseDrink(drinkRatio);
  //if address notr multiple of 5, in another mode, manual mode
  }else{
    showManualColor();
    //addr = 6: motor 5, addr = 4: motor 4, addr = 3: motor 3, addr = 2: motor 2, addr = 1: motor 1,
    if(drinkAddr == 6){
      driveMotor(5, 5);
    }
    else{
      driveMotor(drinkAddr, 5);
    }
    ledTimer =  millis();
    drinkAddr = 0;
  }
  
}

//get the drink ratio of the drink from the EEPROM and sets the drinkRatio array
void getDrinkRatio(byte diskAddr, unsigned int drinkAddr, byte* drinkRatio){
  
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
  
  
  for(byte countPump = 0; countPump < 5; countPump++){
    drinkRatio[countPump] = readEEPROM(drinkAddr + countPump, diskAddr);
  }
  
}


void dispenseDrink(byte* drinkRatio){
  
  //iterate through all drinks
  for(int countDrink = 0; countDrink < 5; countDrink++){
    //dispense if not 0 ml
      driveMotor(countDrink + 1, drinkRatio[countDrink]);
      delay(50);
  }

  //drink is done, set flag to done
  ledTimer = millis();
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
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void saveEEPROM(byte data, byte device, uint8_t eeaddress) {
  Wire.beginTransmission(device);
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.write(data);
    Wire.endTransmission();
    delay(10);
}



void showManualColor(){
    //warm white
    myColor.rVal = 240;
    myColor.gVal = 45;
    myColor.bVal = 5;
    //led buffer
    analogWrite(ledR, myColor.rVal);
    analogWrite(ledG, myColor.gVal);
    analogWrite(ledB, myColor.bVal);
}

void colorOff(){
    myColor.rVal = 0;
    myColor.gVal = 0;
    myColor.bVal = 0;
   analogWrite(ledR, 0);
   analogWrite(ledG, 0);
   analogWrite(ledB, 0);
}


void showRandomColor(){
  byte colorVal = random(0, 7);

  switch(colorVal){
    //orange
    case 0: myColor.rVal = 255;
            myColor.gVal = 25;
            myColor.bVal = 0;
            break;
    //light blue        
    case 1: myColor.rVal = 0;
            myColor.gVal = 162;
            myColor.bVal = 250;
            break;
    //pink        
    case 2: myColor.rVal = 255;
            myColor.gVal = 23;
            myColor.bVal = 21; 
            break;
    //light purple                          
    case 3: myColor.rVal = 255;
            myColor.gVal = 15;
            myColor.bVal = 145;
            break;
    //warm white        
    case 4: myColor.rVal = 240;
            myColor.gVal = 45;
            myColor.bVal = 5;
            break;
    //yellow        
    case 5: myColor.rVal = 255;
            myColor.gVal = 90;
            myColor.bVal = 0;
            break;
    //light green        
    case 6: myColor.rVal = 115;
            myColor.gVal = 235;
            myColor.bVal = 0; 
            break; 
    //white         
    default: myColor.rVal = 255;
            myColor.gVal = 210;
            myColor.bVal = 110;  
            break;                            
  }

  fadeInColor(&myColor);
}


void fadeInColor(color *thisColor){
  color showThis = {0, 0, 0};

  while((showThis.rVal < thisColor -> rVal) || (showThis.gVal < thisColor -> gVal) || (showThis.bVal < thisColor -> bVal)){
    if(showThis.rVal < thisColor -> rVal) showThis.rVal  = showThis.rVal + 1;
    if(showThis.gVal < thisColor -> gVal) showThis.gVal  = showThis.gVal + 1;
    if(showThis.bVal < thisColor -> bVal) showThis.bVal  = showThis.bVal + 1;

    ledShow(&showThis);
    delay(5);
  }
}

void ledShow(color *thisColor){
  analogWrite(ledR, thisColor -> rVal);
  analogWrite(ledG, thisColor -> gVal);
  analogWrite(ledB, thisColor -> bVal);
}

void fadeOutColor(color *thisColor){
  //mask when fading out to prevent i2c from crashing
  int prevAddr = drinkAddr;
  drinkAddr = 1;
  while((thisColor -> rVal > 0) || (thisColor -> gVal > 0) || (thisColor -> bVal > 0)){
    if(thisColor -> rVal > 0) thisColor -> rVal = thisColor -> rVal - 1;
    if(thisColor -> gVal > 0) thisColor -> gVal = thisColor -> gVal - 1;
    if(thisColor -> bVal > 0) thisColor -> bVal = thisColor -> bVal - 1;

    ledShow(thisColor);
    delay(1);
  }

  
  drinkAddr = prevAddr;
}



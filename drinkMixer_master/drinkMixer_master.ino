#include <Wire.h>
#include <U8glib.h>
#include <LiquidCrystal_I2C.h>

U8GLIB_ST7920_128X64 u8g(13, 12, 11, U8G_PIN_NONE);                  // SPI Com: SCK = en = 13, MOSI = rw = 11, CS = di = 0
LiquidCrystal_I2C lcd(0x27, 16, 2);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//defines a point on the lcd display
typedef struct{
  byte xCoor;
  byte yCoor;
} point;

//defines a type of drink
typedef struct{
  byte drinkLocation;
  unsigned int volume;
  //max 10 characters
  char drinkName[11];
} drink;
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


const uint8_t leftArrow[] U8G_PROGMEM = {
0x0,  //00000000
0x4,  //00000100
0xc,  //00001100
0x1c,  //00011100
0x3c,  //00111100
0x1c,  //00011100
0xc,  //00001100
0x4  //00000100
};

const uint8_t rightArrow[] U8G_PROGMEM = {
0x0,  //00000000
0x20,  //00100000
0x30,  //00110000
0x38,  //00111000
0x3c, //00111100
0x38,  //00111000
0x30,  //00011000
0x20  //00100000
};


const uint8_t deleteButton[] U8G_PROGMEM = {
0x0, //00000000
0xf, //00001111
0x1f, //00011111
0x35, //00110101
0x7b, //01111011
0x35, //00110101
0x1f, //00011111
0xf //00001111
};

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//I2C Devices
const byte slaveDevice = 8;
const byte drinkDisk = 0x50;

//buttons for controlling machine
const byte upBtn = 9;
const byte leftBtn = 8;
const byte downBtn = 7;
const byte rightBtn = 6;
const byte confirmBtn = 5;
const byte cancelBtn = 4;


//0 = none, 1 = up, 2 = left, 3 = down, 4 = right, 5 = confirm, 6 = cancel
byte btnVal = 0;

byte cursorPos = 1;

byte modeNum = 0;

//only redraw the lcd if u8gRedraw is set to true
bool u8gRedraw = true;
//
bool lcdRedraw = true;
//to prevent the lcd from flickering due to the picture loop
//bool lcdDrawn = false;
//slave device tells the master device when the drink is done
bool doneFlag = true;

drink drinks[12];
byte numDrinks;
drink *curDrink;

//the 5 pumps in the machine
drink pumps[5];

char nameRegister[11];
byte nameRegLength = 0;

byte volRegister[5] = {0, 0, 0, 0, 0};

byte setPumpNum = 0;
//in drinkDisk, loc300: 1 means pump is set, 0 otherwise, loc301: stores length of first pump
byte pumpFreeLocation =301;

unsigned long drinkProgressTimer;
unsigned long btnTimer;
unsigned long confirmTimer;
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void testInitializeDrinks();
void mainAction();
void selectDrinkAction();
void manualAction();
void editDrinkAction();
void ratioAdjustAction();
void keypadAction();
void resetAction();
void aboutAction();
void setPumpAction();
void drawMain();
void drawSelectDrink();
void drawManual();
void drawEditDrink();
void drawRatioAdjust();
void drawKeypad();
void drawReset();
void drawAbout();
void drawSetPump();
void lcdMain();
void lcdSelectDrink();
void lcdManual();
void lcdEditDrink();
void lcdRatioAdjust();
void lcdKeypad();
void lcdReset();
void lcdAbout();
void lcdSetPump();


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

















void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  lcdStartup();
  Wire.begin();  //join I2C as master device
  u8g.begin();
  u8g.setFont(u8g_font_6x10);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  
  pinMode(upBtn, INPUT_PULLUP);
  pinMode(leftBtn, INPUT_PULLUP);
  pinMode(downBtn, INPUT_PULLUP);
  pinMode(rightBtn, INPUT_PULLUP);
  pinMode(confirmBtn, INPUT_PULLUP);
  pinMode(cancelBtn, INPUT_PULLUP);
  //initializeEEPROM();
  initializeMachine();
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//set the EEPROM to its initial state where nothing is set
void initializeEEPROM(){
  saveEEPROM(0, drinkDisk, 0);
  saveEEPROM(102, drinkDisk, 100);
  saveEEPROM(0, drinkDisk, 300);
  
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void lcdStartup(){
  lcd.init();
  lcd.backlight();
  lcd.setCursor(4, 0);
  lcd.print(F("WELCOME!"));
  lcd.setCursor(0, 1);
  lcd.print(F("PLEASE WAIT....."));
}


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void loop() {
  Serial.println(numDrinks);
  //redraw the 128x64 display only if necessary
  u8g.firstPage();
  if(u8gRedraw == true){
    do{
      modeDraw(modeNum);
    }while(u8g.nextPage());
  }
  //redraw the 1602 lcd only if necessary
  if(lcdRedraw == true){
    drawLcd(modeNum);
  }
  
  //perform the necessary action according to the current mode we are in
  buttonCheck();
  modeAction(modeNum);

  if(doneFlag == false){ 
    doneFlagCheck();
    //doneFlagTest();
  }else if(millis() - drinkProgressTimer > 2000 && millis() - drinkProgressTimer < 2080){
    lcdRedraw = true;
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//checks if any of the buttons are pressed, and sets the btnVal to the corresponding values if no buttons were pressed previously in the same cycle
void buttonCheck(){
  //only check button presses if the previous button press has been processed
  if(btnVal == 0 && millis() - btnTimer >= 60){
    if(digitalRead(upBtn) == LOW) btnVal = 1;
    else if(digitalRead(leftBtn) == LOW) btnVal = 2;
    else if(digitalRead(downBtn) == LOW) btnVal = 3;
    else if(digitalRead(rightBtn) == LOW) btnVal = 4;
    else if(digitalRead(confirmBtn) == LOW && millis() - confirmTimer >= 350){
      btnVal = 5;
      confirmTimer = millis();
    }
    else if(digitalRead(cancelBtn) == LOW) btnVal = 6;
    btnTimer = millis();
  }

  //sets the u8gRedraw if button pressed so display will be updated
  if(btnVal != 0){
    u8gRedraw = true;
    lcdRedraw = true;
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//request 1 byte from slave device via I2C
void doneFlagCheck(){
  //only check every 3 seconds if machine is making drink
  if(millis() - drinkProgressTimer >= 1000){
    //request 1 byte from the slave device
    Wire.requestFrom(slaveDevice, 1);
    while(Wire.available()){
      if(Wire.read() == 't'){
        doneFlag = true;
        lcdRedraw = true;
      }
    }
    drinkProgressTimer = millis();
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//Perform the necessary actions based on the current mode we are in
void modeAction(byte curMode){
  switch(curMode){
    case 0: mainAction();
            break;
    case 1: selectDrinkAction();
            break;
    case 2: manualAction();
            break;
    case 3: editDrinkAction();
            break;
    case 4: ratioAdjustAction();
            break;
    case 5: keypadAction();
            break;
    case 6: resetAction();
            break;
    case 7: aboutAction();
            break;
    case 8: setPumpAction();
            break;
    default: break;                                                                      
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//draw menu according to the current menu mode we are in, to be updated in picture loop
void modeDraw(byte curMode){
  switch(modeNum){
    case 0: drawMain();
            break;
    case 1: drawSelectDrink();
            break;
    case 2: drawManual();
            break;
    case 3: drawEditDrink();
            break;
    case 4: drawRatioAdjust();
            break;
    case 5: drawKeypad();
            break;
    case 6: drawReset();
            break;
    case 7: drawAbout();
            break;
    case 8: drawSetPump();
            break;
    default: break;
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawLcd(byte curMode){
  switch(modeNum){
    case 0: lcdMain();
            break;
    case 1: lcdSelectDrink();
            break;
    case 2: lcdManual();
            break;
    case 3: lcdEditDrink();
            break;
    case 4: lcdRatioAdjust();
            break;
    case 5: lcdKeypad();
            break;
    case 6: lcdReset();
            break;
    case 7: lcdAbout();
            break;
    case 8: lcdSetPump();
            break;
    default: break;
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//return 1 byte from the EEPROM
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

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void sendDrink(drink *myDrink){
  Wire.beginTransmission(slaveDevice);
  Wire.write(myDrink -> drinkLocation);
  Wire.endTransmission();
  delay(200);
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------




//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawMain(){

  char *MainMenu[6] PROGMEM = {"SELECT DRINK", "MANUAL MODE", "EDIT DRINK", "ADD DRINK", "RESET MACHINE", "ABOUT"};
  uint8_t i, h;
  u8g_uint_t w, d;
  h = u8g.getFontAscent() - u8g.getFontDescent();
  w = u8g.getWidth();

  for(byte countItem = 0; countItem < 6; countItem++){
    d = (w-u8g.getStrWidth(MainMenu[countItem])) / 2;
    u8g.setDefaultForegroundColor();
    if ( (countItem + 1) == cursorPos ) {
      u8g.drawBox(0, countItem * h + 1 + countItem, w, h);
      u8g.setDefaultBackgroundColor();
    }
    u8g.drawStr(d, countItem*h + 1 + countItem, MainMenu[countItem]);
  }
  
  u8gRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawSelectDrink(){
    u8g_uint_t w, d;
    
    if(numDrinks > 0){
      uint8_t i, h;
    
      h = u8g.getFontAscent() - u8g.getFontDescent();
      w = u8g.getWidth()/2;
    
      for(byte countItem = 0; countItem < numDrinks; countItem++){
        if(countItem % 2 != 0) d = ((w - u8g.getStrWidth(drinks[countItem].drinkName)) / 2) + w;
        else d = (w - u8g.getStrWidth(drinks[countItem].drinkName)) / 2;
    
        u8g.setDefaultForegroundColor();
        if ( (countItem + 1) == cursorPos ) {
          u8g.drawBox((countItem % 2) * w, ((countItem / 2) * h + 1) + (countItem / 2), w, h);
          u8g.setDefaultBackgroundColor();
        }
        u8g.drawStr(d, (countItem / 2 ) * h + 1 + (countItem / 2), drinks[countItem].drinkName);
      }
    }else{
    
      w = u8g.getWidth();
      d = (w - u8g.getStrWidth(F("ADD DRINKS"))) / 2;
      u8g.setColorIndex(1);
      u8g.drawStr(d, 20, F("ADD DRINKS"));
      d = (w - u8g.getStrWidth(F("BEFORE PROCEEDING"))) / 2;
      u8g.drawStr(d, 35, F("BEFORE PROCEEDING"));
  }  
  
  u8gRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawManual(){
  uint8_t i, h;
  u8g_uint_t w;
  h = u8g.getFontAscent() - u8g.getFontDescent();
  w = u8g.getWidth();
  for(byte countItem = 0; countItem < 5; countItem++){
    u8g.setColorIndex(1);
    if ( (countItem + 1) == cursorPos ) {
      u8g.drawBox(0, countItem * h + 1 + countItem, w, h);
      u8g.setColorIndex(0);
      u8g.drawStr(80, countItem * h + 1 + countItem, F("dispense"));
    }
    u8g.drawStr(0, countItem * h + 1 + countItem, pumps[countItem].drinkName);
  }
  
  u8gRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawEditDrink(){
  u8g_uint_t w, d;
  
  if(doneFlag == true){
      if(numDrinks > 0){
      uint8_t i, h;
    
      h = u8g.getFontAscent() - u8g.getFontDescent();
      w = u8g.getWidth()/2;
    
      for(byte countItem = 0; countItem < numDrinks; countItem++){
          if(countItem % 2 != 0) d = ((w - u8g.getStrWidth(drinks[countItem].drinkName)) / 2) + w;
          else d = (w - u8g.getStrWidth(drinks[countItem].drinkName)) / 2;
      
          u8g.setDefaultForegroundColor();
          if ( (countItem + 1) == cursorPos ) {
              u8g.drawBox((countItem % 2) * w, ((countItem / 2) * h + 1) + (countItem / 2), w, h);
              u8g.setDefaultBackgroundColor();
          }
          u8g.drawStr(d, (countItem / 2 ) * h + 1 + (countItem / 2), drinks[countItem].drinkName);
      }
  
      }else{
      
          w = u8g.getWidth();
          d = (w - u8g.getStrWidth(F("ADD DRINKS"))) / 2;
          u8g.setColorIndex(1);
          u8g.drawStr(d, 20, F("ADD DRINKS"));
          d = (w - u8g.getStrWidth(F("BEFORE PROCEEDING"))) / 2;
          u8g.drawStr(d, 35, F("BEFORE PROCEEDING"));
      }  
  }else{
    
      w = u8g.getWidth();
      d = (w - u8g.getStrWidth(F("PLEASE WAIT"))) / 2;
      u8g.setColorIndex(1);
      u8g.drawStr(d, 15, F("PLEASE WAIT"));
      d = (w - u8g.getStrWidth(F("MAKING DRINK..."))) / 2;
      u8g.drawStr(d, 35, F("MAKING DRINK..."));
  }  
  
  u8gRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawRatioAdjust(){
  uint8_t i, h;
  u8g_uint_t w;
  h = u8g.getFontAscent() - u8g.getFontDescent();
  w = u8g.getWidth();
  for(byte countItem = 0; countItem < 5; countItem++){
    u8g.setColorIndex(1);
    if ( (countItem + 1) == cursorPos ) {
      u8g.drawBox(0, countItem * h + 1 + countItem, w, h);
      u8g.setColorIndex(0);
    }
    u8g.drawStr(0, countItem * h + 1 + countItem, pumps[countItem].drinkName);
    u8g.drawBitmapP(80, countItem * h + 1 + countItem, 1, 8, leftArrow);
    u8g.drawBitmapP(110, countItem * h + 1 + countItem, 1, 8, rightArrow);
    if(volRegister[countItem] >= 100) u8g.setPrintPos(91, countItem * h + 1 + countItem);
    else if(volRegister[countItem] >= 10) u8g.setPrintPos(94, countItem * h + 1 + countItem);
    else u8g.setPrintPos(97, countItem * h + 1 + countItem);
    u8g.print(volRegister[countItem]);
    u8g.drawStr(118, countItem * h + 1 + countItem, "ml");
  }
  
  u8gRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawKeypad(){
   u8g_uint_t w, d;
  
  if(numDrinks < 12){
    uint8_t i, h;
  
    h = u8g.getFontAscent() - u8g.getFontDescent();
    w = u8g.getWidth() / 6;
    for(byte countItem = 0; countItem < 27; countItem++){
      switch(countItem % 6){
        case 0: d = ((w - u8g.getStrWidth("A")) / 2);
                break;
        case 1: d = ((w - u8g.getStrWidth("A")) / 2) + w;
                break;
        case 2: d = ((w - u8g.getStrWidth("A")) / 2) + 2 * w;
                break;
        case 3: d = ((w - u8g.getStrWidth("A")) / 2) + 3 * w;
                break;
        case 4: d = ((w - u8g.getStrWidth("A")) / 2) + 4 * w;
                break;
        case 5: d = ((w - u8g.getStrWidth("A")) / 2) + 5 * w;
                break;
      }
  
      u8g.setColorIndex(1);
      
      if ( (countItem + 1) == cursorPos ) {
        u8g.drawBox((countItem % 6) * w, ((countItem / 6) * h + 1) + (countItem / 6), w, h);
        u8g.setColorIndex(0);
      }
  
      //draw the characters and the delete button
      if(countItem == 26) u8g.drawBitmapP(d - 2, ((countItem / 6) * h + 1) + (countItem / 6), 1, 8, deleteButton);
      else{
        char myChar[2] = {(char)countItem + 65, '\0'};
        //u8g.setCursorPos(d, (countItem / 6 ) * h + 1 + (countItem / 6));
        //u8g.print(myChar);
        u8g.drawStr(d, (countItem / 6 ) * h + 1 + (countItem / 6), myChar);
      }
    }
   
    //draw the done button
    d = (u8g.getWidth() - u8g.getStrWidth(F("DONE"))) / 2;
    if(cursorPos == 28){
      u8g.setColorIndex(1);
      u8g.drawBox(0, 5 * h + 6, u8g.getWidth(), h);
      u8g.setColorIndex(0);
    }else{
      u8g.setColorIndex(1);
    }
      u8g.drawStr(d, 5 * h + 6, F("DONE"));
  }else{
      w = u8g.getWidth();
      d = (w - u8g.getStrWidth(F("ERROR:"))) / 2;
      u8g.setColorIndex(1);
      u8g.drawStr(d, 12, F("ERROR:"));
      d = (w - u8g.getStrWidth(F("EXCEED MAXIMUM DRINKS"))) / 2;
      u8g.drawStr(d, 45, F("EXCEED MAXIMUM DRINKS"));   
  }
  
  u8gRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawReset(){
  u8g_uint_t w, d;
  
  if(doneFlag == true){
    w = u8g.getWidth();
    d = (w - u8g.getStrWidth(F("WARNING!!!"))) / 2;
    u8g.setColorIndex(1);
    u8g.drawStr(d, 12, F("WARNING!!!"));
    d = (w - u8g.getStrWidth(F("RESET WILL"))) / 2;
    u8g.drawStr(d, 35, F("RESET WILL"));
    d = (w - u8g.getStrWidth(F("CLEAR ALL DRINKS"))) / 2;
    u8g.drawStr(d, 47, F("CLEAR ALL DRINKS"));
  }else{
    w = u8g.getWidth();
    d = (w - u8g.getStrWidth(F("PLEASE WAIT"))) / 2;
    u8g.setColorIndex(1);
    u8g.drawStr(d, 12, F("PLEASE WAIT"));
    d = (w - u8g.getStrWidth(F("MAKING DRINK..."))) / 2;
    u8g.drawStr(d, 45, F("MAKING DRINK..."));
  }
  u8gRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawAbout(){
    u8g.setColorIndex(1);
    u8g.drawStr(0, 1, F("DESIGNER- Alex Lee"));
    u8g.drawStr(0, 13, F("HARDWARE- Alex Lee"));
    u8g.drawStr(0, 25, F("SOFTWARE- Alex Lee"));
    u8g.drawStr(0, 37, F("VERSION- v1.0.0"));
    u8g.drawStr(0, 49, F("DATE- JULY 13, 2018"));
    u8gRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void drawSetPump(){
  uint8_t i, h;
  u8g_uint_t w, d;

  h = u8g.getFontAscent() - u8g.getFontDescent();
  w = u8g.getWidth() / 6;
  
  for(byte countItem = 0; countItem < 27; countItem++){
    switch(countItem % 6){
      case 0: d = ((w - u8g.getStrWidth("A")) / 2);
              break;
      case 1: d = ((w - u8g.getStrWidth("A")) / 2) + w;
              break;
      case 2: d = ((w - u8g.getStrWidth("A")) / 2) + 2 * w;
              break;
      case 3: d = ((w - u8g.getStrWidth("A")) / 2) + 3 * w;
              break;
      case 4: d = ((w - u8g.getStrWidth("A")) / 2) + 4 * w;
              break;
      case 5: d = ((w - u8g.getStrWidth("A")) / 2) + 5 * w;
              break;
    }

    u8g.setColorIndex(1);
    
    if ( (countItem + 1) == cursorPos ) {
      u8g.drawBox((countItem % 6) * w, ((countItem / 6) * h + 1) + (countItem / 6), w, h);
      u8g.setColorIndex(0);
    }

    //draw the characters and the delete button
    if(countItem == 26) u8g.drawBitmapP(d - 2, ((countItem / 6) * h + 1) + (countItem / 6), 1, 8, deleteButton);
    else{
      char myChar[2] = {(char)countItem + 65, '\0'};
      //u8g.setCursorPos(d, (countItem / 6 ) * h + 1 + (countItem / 6));
      //u8g.print(myChar);
      u8g.drawStr(d, (countItem / 6 ) * h + 1 + (countItem / 6), myChar);
    }
  }
 
  //draw the done button
  d = (u8g.getWidth() - u8g.getStrWidth(F("DONE"))) / 2;
  if(cursorPos == 28){
    u8g.setColorIndex(1);
    u8g.drawBox(0, 5 * h + 6, u8g.getWidth(), h);
    u8g.setColorIndex(0);
  }else{
    u8g.setColorIndex(1);
  }
  u8g.drawStr(d, 5 * h + 6, F("DONE"));
  
  u8gRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//while in the main menu, given the current mode, returns the next mode
byte nextConfirmMode(byte myCursor){
  byte mode = 0;
  switch(myCursor){
    case 1: mode = 1;
            if(numDrinks != 0) curDrink = &drinks[0];
            break;
    case 2: mode = 2;
            curDrink = &pumps[0];
            break;
    case 3: mode = 3;
            if(numDrinks != 0) curDrink = &drinks[0];
            break;
    case 4: mode = 5;
            break;
    case 5: mode = 6;
            break;
    case 6: mode = 7;
            break;        
    default: break;
  }
  //lcdDrawn = false;
  return mode;
}


void returnMain(){
  modeNum = 0;
  cursorPos = 1;
  //lcdDrawn = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void mainAction(){
  switch(btnVal){
    //up button
    case 1: if(cursorPos > 1) cursorPos = cursorPos - 1;
            break;
    //down button        
    case 3: if(cursorPos < 6) cursorPos = cursorPos + 1;
            break;
    //confirm button        
    case 5: modeNum = nextConfirmMode(cursorPos); 
            cursorPos = 1;
            break;
    default: break;
  }
  //reset the btnVal
  btnVal = 0;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void selectDrinkAction(){
    switch(btnVal){
      //up button
      case 1: if(cursorPos > 2) cursorPos = cursorPos - 2;
              break;
      //left button        
      case 2: if(cursorPos % 2 == 0 && cursorPos > 1) cursorPos = cursorPos - 1;
              break;
      //down button        
      case 3: if(cursorPos + 2 <= numDrinks) cursorPos = cursorPos + 2;
              break;
      //right button        
      case 4: if(cursorPos % 2 != 0 && cursorPos + 1 <= numDrinks) cursorPos = cursorPos + 1;
              break;   
      //select button                             
      case 5: if(numDrinks != 0){ 
                sendDrink(curDrink);
                doneFlag = false;
              }
              break;
      case 6: returnMain();
              break;
      default: break;
    }
    //reassign pointer
    if(numDrinks != 0){
      curDrink = &drinks[cursorPos - 1];
    }
    //reset the btnVal
    btnVal = 0;      
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void manualAction(){
    switch(btnVal){
        case 1: if(cursorPos > 1) cursorPos = cursorPos - 1;
            break;
      //down button        
      case 3: if(cursorPos < 5) cursorPos = cursorPos + 1;
            break;
      case 5: sendDrink(curDrink);
              break;      
      case 6: returnMain();
              break;
      default: break;
  }
  curDrink = &pumps[cursorPos - 1];
  //reset the btnVal
  btnVal = 0;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void editDrinkAction(){
    //reassign pointer
    curDrink = &drinks[cursorPos - 1];
    if(doneFlag == true){
      switch(btnVal){
        //up button
        case 1: if(cursorPos > 2) cursorPos = cursorPos - 2;
                break;
        //left button        
        case 2: if(cursorPos % 2 == 0 && cursorPos > 1) cursorPos = cursorPos - 1;
                break;
        //down button        
        case 3: if(cursorPos + 2 <= numDrinks) cursorPos = cursorPos + 2;
                break;
        //right button        
        case 4: if(cursorPos % 2 != 0 && cursorPos + 1 <= numDrinks) cursorPos = cursorPos + 1;
                break;   
        //select button                             
        case 5: if(numDrinks > 0){
                  modeNum = 4;
                  cursorPos = 1;
                  initializeVolRegister(curDrink);
                }
                break;
        case 6: returnMain();
                break;
        default: break;
      }
    }else{
        if(btnVal == 6) returnMain();
    }
    //reset the btnVal
    btnVal = 0;      
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void initializeVolRegister(drink *myDrink){
  byte location = myDrink -> drinkLocation;
  for(byte countPump = 0; countPump < 5; countPump++){
    volRegister[countPump] = readEEPROM(location + countPump, drinkDisk);
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void saveVolRegister(drink *myDrink){
  byte location = myDrink -> drinkLocation;
  unsigned int totalVol = 0;
  for(byte countPump = 0; countPump < 5; countPump++){
    saveEEPROM(volRegister[countPump], drinkDisk, location + countPump);
    totalVol = totalVol + volRegister[countPump];
  }
  //update the total volume
  myDrink -> volume = totalVol;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void ratioAdjustAction(){
    switch(btnVal){
      case 1: if(cursorPos > 1) cursorPos = cursorPos - 1;
              break;
      case 2: if(volRegister[cursorPos - 1] > 0) volRegister[cursorPos - 1] = volRegister[cursorPos - 1] - 1; 
              break;
      case 3: if(cursorPos < 5) cursorPos = cursorPos + 1;
              break;
      //maximum is 200 ml        
      case 4: if(volRegister[cursorPos - 1] < 200) volRegister[cursorPos - 1] = volRegister[cursorPos - 1] + 1; 
              break;  
      case 5: saveVolRegister(curDrink);
              returnMain();
              break;              
      case 6: returnMain();
              break;
      default: break;
  }
  //reset the btnVal
  btnVal = 0;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void keypadAction(){
  if(numDrinks < 12){
    if(nameRegLength == 0) lcd.cursor();
    switch(btnVal){
        case 1: if(cursorPos == 28) cursorPos = 27;
                else if(cursorPos > 6) cursorPos = cursorPos - 6;
                break;
        case 2: //if not in the left column already
                if(cursorPos % 6 != 1 && cursorPos > 1) cursorPos = cursorPos - 1; 
                break;
        case 3: if(cursorPos + 6 <= 27) cursorPos = cursorPos + 6; 
                else if(cursorPos >= 22) cursorPos = 28;
                break;      
        case 4: //if not in the right column already
                if(cursorPos % 6 != 0 && cursorPos + 1 <= 28) cursorPos = cursorPos + 1; 
                break; 
        case 5: //done typing
                if(cursorPos == 28 && nameRegLength != 0){
                  lcd.noCursor();
                  newDrink();
                  modeNum = 4;
                  cursorPos = 1;
                //delete button
                }else if(cursorPos == 27 && nameRegLength > 0){
                  lcd.cursor();
                  nameRegLength = nameRegLength - 1;
                //typed new character, maximum is 10 characters  
                }else if(nameRegLength < 10 && cursorPos <= 26){
                  nameRegister[nameRegLength] = (char) (cursorPos + 64);
                  nameRegLength = nameRegLength + 1;
                  if(nameRegLength == 10) lcd.noCursor();
                }
                break;              
        case 6: returnMain();
                lcd.noCursor();
                nameRegLength = 0;
                break;
        default: break;
    }
  }else{
      if(btnVal == 6) returnMain();
  }
  //reset the btnVal
  btnVal = 0;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//creates a new drink and set the curDrink pointer
void newDrink(){
  modeNum = 4;
  cursorPos = 1;

  drink * myDrink = &drinks[numDrinks];
  curDrink = &drinks[numDrinks];
  numDrinks = numDrinks + 1;

  //update EEPROM
  saveEEPROM(numDrinks, drinkDisk, 0);

  //100 stores first free name
  byte firstFreeName = readEEPROM(100, drinkDisk);

  //save the entered name
  for(byte countChar = 0; countChar < nameRegLength; countChar++){
    (myDrink -> drinkName)[countChar] = nameRegister[countChar];
    saveEEPROM(nameRegister[countChar], drinkDisk, firstFreeName + 2 + countChar);
  }
  //add the terminating character
  (myDrink -> drinkName)[nameRegLength] = '\0';
  
  int myLocation = 5 * numDrinks;
  saveEEPROM(myLocation, drinkDisk, firstFreeName + 1);

  myDrink -> drinkLocation = myLocation;
  myDrink -> volume = 0;

  //initialize the ratio of the drink in the EEPROM
  for(byte countPump = 0; countPump < 5; countPump++){
    saveEEPROM(0, drinkDisk, myLocation + countPump);
  }

  //update the length of the new drink name
  saveEEPROM(nameRegLength, drinkDisk, firstFreeName);
  //update the first free position in the EEPROM
  saveEEPROM(firstFreeName + 2 + nameRegLength, drinkDisk, 100);

  nameRegLength = 0;
  initializeVolRegister(myDrink);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void resetAction(){
  if(doneFlag == true){
    switch(btnVal){
      case 5: resetMemory();
              goToSetPump();
              break;
      case 6: returnMain();        
      default: break;
    }
  }else{
    if(btnVal == 6) returnMain();
  }
  //reset the btnVal
  btnVal = 0;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void resetMemory(){
   numDrinks = 0;
   nameRegLength = 0;
   //stores numDrinks
   saveEEPROM(0, drinkDisk, 0);
   //stores length of first drink name
   saveEEPROM(102, drinkDisk, 100);
   //stores pump is set
   saveEEPROM(0,drinkDisk, 300);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void goToSetPump(){
  nameRegLength = 0;
  modeNum = 8;
  cursorPos = 1;
  setPumpNum = 0;
  pumpFreeLocation = 301;
  numDrinks = 0;
  //reset the drinks if pump needs to be set
  saveEEPROM(numDrinks, drinkDisk, 0);
  //name hasnt been declared yet
  saveEEPROM(102, drinkDisk, 100);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void savePump(byte pumpNum){
  drink *myPump = &pumps[pumpNum];
  saveEEPROM(nameRegLength, drinkDisk, pumpFreeLocation);
  for(byte countChar = 0; countChar < nameRegLength; countChar++){
    saveEEPROM(nameRegister[countChar], drinkDisk, pumpFreeLocation + 1 + countChar);
    (myPump -> drinkName)[countChar] = nameRegister[countChar];
  }
  //add the terminating character
  (myPump -> drinkName)[nameRegLength] = '\0';
  pumpFreeLocation = pumpFreeLocation + 1 + nameRegLength;
  
  setPumpNum = setPumpNum + 1;
  nameRegLength = 0;
  cursorPos = 1;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void aboutAction(){
  //reset the btnVal
    switch(btnVal){
      case 6: returnMain();
              break;
      default: break;
  }
  //reset the btnVal
  btnVal = 0;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void setPumpAction(){
  Serial.println(cursorPos);
  if(nameRegLength == 0) lcd.cursor();
  //reset the btnVal
    switch(btnVal){
      case 1: if(cursorPos == 28) cursorPos = 27;
              else if(cursorPos > 6) cursorPos = cursorPos - 6;
              break;
      case 2: //if not in the left column already
              if(cursorPos % 6 != 1 && cursorPos > 1) cursorPos = cursorPos - 1; 
              break;
      case 3: if(cursorPos + 6 <= 27) cursorPos = cursorPos + 6; 
              else if(cursorPos >= 22) cursorPos = 28;
              break;      
      case 4: //if not in the right column already
              if(cursorPos % 6 != 0 && cursorPos + 1 <= 28) cursorPos = cursorPos + 1; 
              break; 
      case 5: //done typing
              if(cursorPos == 28 && nameRegLength != 0){
                lcd.noCursor();
                savePump(setPumpNum);
                if(setPumpNum > 4){
                  //done setting pump, indicate in EEPROM
                  saveEEPROM(1, drinkDisk, 300);
                  returnMain();
                }  
              //delete button
              }else if(cursorPos == 27 && nameRegLength > 0){
                lcd.cursor();
                nameRegLength = nameRegLength - 1;
              //typed new character, maximum is 10 characters  
              }else if(nameRegLength < 10 && cursorPos <= 26){
                nameRegister[nameRegLength] = (char) (cursorPos + 64);
                nameRegLength = nameRegLength + 1;
                if(nameRegLength == 10) lcd.noCursor();
              break;              
      default: break;
      }
   }  
  //reset the btnVal
  btnVal = 0;
}  
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void lcdMain(){
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print(F("HELLO!"));
  lcd.setCursor(0, 1);
  lcd.print(F("HOW MAY I HELP?"));
  lcdRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void lcdManual(){
    lcd.clear();
    String curPump = curDrink -> drinkName;
    lcd.setCursor(2, 0);
    lcd.print(F("MANUAL  MODE"));
    lcd.setCursor(0, 1);
    lcd.print(F("PUMP: "));
    lcd.print(curPump);
  lcdRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void lcdSelectDrink(){
  //still making drink
  if(numDrinks == 0){
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print(F("NO DRINKS"));
    lcd.setCursor(0, 1);
    lcd.print(F("PLEASE ADD DRINK"));
  }else if(doneFlag == false){
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print(F("MAKING DRINK"));
    lcd.setCursor(0, 1);
    lcd.print(F("PLEASE WAIT....."));
  }//doneFlag is true but timer is not over yet
  else if(millis() - drinkProgressTimer <= 2000){
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print(F("DRINK DONE"));
    lcd.setCursor(5, 1);
    lcd.print(F("ENJOY!"));
  }else{
    unsigned int drinkVol = curDrink -> volume;
    String curName = curDrink -> drinkName;
    byte numSpace = (16 - curName.length()) / 2;
    lcd.clear();
    lcd.setCursor(numSpace, 0);
    lcd.print(curName);
    lcd.setCursor(0, 1);
    lcd.print(F("Volume: "));
    lcd.print(drinkVol);
    lcd.setCursor(13, 1);
    lcd.print(F("ml"));
  }  
  lcdRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void lcdEditDrink(){
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print(F("SELECT DRINK"));
  lcd.setCursor(4, 1);
  lcd.print(F("TO EDIT"));
  lcdRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void lcdRatioAdjust(){
  lcd.clear();
  String drinkName = curDrink -> drinkName;
  byte numSpace = (16 - drinkName.length()) / 2;
  lcd.home();
  lcd.print(F("ADJUSTING RATIO:"));
  lcd.setCursor(numSpace, 1);
  lcd.print(drinkName);
  lcdRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void lcdKeypad(){
  lcd.clear();
  if(numDrinks < 12){
    lcd.home();
    lcd.print(F("ENTER DRINK NAME"));
    lcd.setCursor(0, 1);
    for(byte countChar = 0; countChar < nameRegLength; countChar++){
      lcd.print(nameRegister[countChar]);
    }
  }else{ 
    lcd.setCursor(5,0);
    lcd.print(F("ERROR"));
    lcd.setCursor(0,1);
    lcd.print(F("CANNOT ADD DRINK"));
  }
  lcdRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void lcdReset(){
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print(F("RESET"));
  lcd.setCursor(1, 1);
  lcd.print(F("ARE YOU SURE?"));
  lcdRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void lcdAbout(){
  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print(F("ABOUT"));
  lcd.setCursor(0, 1);
  lcd.print(F("DRINK DISPENSER"));
  lcdRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void lcdSetPump(){
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print(F("SET PUMP "));
  lcd.print(setPumpNum + 1);
  lcd.setCursor(0, 1);
  for(byte countChar = 0; countChar < nameRegLength; countChar++){
    lcd.print(nameRegister[countChar]);
  }
  lcdRedraw = false;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

/*drinkDisk:
* loc0: num drinks, loc5: start of drink1, loc10: start of drink2
*drinkDisk:
* loc0: first free location, loc2: length of first name, loc3: location in drinkDisk
*
*/
void initializeDrinks(){
  //get thew number of drinks
  numDrinks = readEEPROM(0, drinkDisk);
  //keep track of the EEPROM location, 102 stores the length of first
  byte drinkNameCounter = 102;

  //get all the drinks from the EEPROM, including name and location in memory
  for(byte drinkCount = 0; drinkCount < numDrinks; drinkCount++){
    byte lengthChar = readEEPROM(drinkNameCounter, drinkDisk);
    byte drinkLocation = readEEPROM(drinkNameCounter + 1, drinkDisk);

    //get the name
    for(byte charCount = 0; charCount < lengthChar; charCount++){
      (drinks[drinkCount].drinkName)[charCount] = readEEPROM(drinkNameCounter + 2 + charCount, drinkDisk);
    }
    //add the terminating character
    (drinks[drinkCount].drinkName)[lengthChar] ='\0';

    //get nameLength, drinkLocation
    drinks[drinkCount].drinkLocation = drinkLocation;
    drinkNameCounter = drinkNameCounter + 2 + lengthChar;

    //get volume
    unsigned int totalVol = 0;
    for(byte countPump = 0; countPump < 5; countPump++){
      totalVol = totalVol + readEEPROM(drinkLocation + countPump, drinkDisk);
    }
    drinks[drinkCount].volume = totalVol;
  }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------
void initializeMachine(){
  bool pumpSet = readEEPROM(300, drinkDisk);
  if(pumpSet == true){
    //get the drinks
    initializeDrinks();
    //initialize each pump one by one
    for(byte countPump = 0; countPump < 5; countPump++){
      byte charCount = readEEPROM(pumpFreeLocation, drinkDisk);

      //initialize the name of one specific pump
      for(byte numChar = 0; numChar < charCount; numChar++){
        pumps[countPump].drinkName[numChar] = readEEPROM(pumpFreeLocation + 1 + numChar, drinkDisk);
      }
      //add the terminating character
      pumps[countPump].drinkName[charCount] = '\0';
      pumpFreeLocation = pumpFreeLocation + 1 + charCount;
    }
    pumpFreeLocation = 301;
  }else{
    goToSetPump();
  }
}


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <Wire.h> 
#include "RCSwitch.h"
#include "LiquidCrystal_I2C.h"
#include "menu.h"

// LED lamp
const int ledPin = 13; // Pin 13 for the led lamp

// Starting strap
const int strapPin = 4; // Pin 4 for the starting strap

// Proximity sensor
const int ProxSensorPin = 3; // Pin 3 for the proximity sensor

// LCD
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

// RC pilot
RCSwitch mySwitch = RCSwitch();

// Menu
#define BTN_BACK  21763
#define BTN_NEXT  21772
#define BTN_PREV  21952
#define BTN_OK    21808

STRUCT_MENUPOS menu[5];

int currentMenuPos = 0;
int menuSize;
bool isInLowerLevel = false;
int tempVal;

void setup() {
  // LED lamp
  pinMode(ledPin, OUTPUT);
  
  // Starting strap
  pinMode(strapPin, OUTPUT);
  
  // Proximity sensor
  pinMode(ProxSensorPin, INPUT);
  
  // LCD
  lcd.init(); // initialize the lcd 
  lcd.backlight();
  lcd.clear();
  lcd.print("Start programu..."); // Print a message to the LCD.
  
  // RC pilot
  mySwitch.enableReceive(0);  // Receiver on interrupt 0 => that is pin #2
 
  // Menu
  menu[0] = {"Start", 0, 9, 5, initAlgorithm};
  menu[1] = {"Sensor", 10, 19, 15, proximityCalibration};
  menu[2] = {"LED", 20, 29, 25, ledTest};
  menu[3] = {"Tasma", 30, 39, 35, strapTest};
  menu[4] = {"Autor", 40, 49, 45, authorShow};

  menuSize = sizeof(menu)/sizeof(STRUCT_MENUPOS);
}

void loop() {
  drawMenu();
}

ENUM_BUTTON getButton() {
  if (mySwitch.available()) {
    int value = mySwitch.getReceivedValue();
   
    if(value == BTN_BACK)  {
      mySwitch.resetAvailable();
      return BACK;
    }
    else if(value == BTN_NEXT) {
      mySwitch.resetAvailable();
      return NEXT;
    }
    else if(value == BTN_PREV) {
      mySwitch.resetAvailable();
      return PREV;
    }
    else if(value == BTN_OK) {
      mySwitch.resetAvailable();
      return OK;
    } else {
      mySwitch.resetAvailable();
      lcd.print("Nie ten pilot! ");
      lcd.setCursor(0, 1);
      lcd.print(value);
    }
  }
  return NONE;
}

void drawMenu() {
  static unsigned long lastRead = 0;
  static ENUM_BUTTON lastPressedButton = OK;
  static unsigned int isPressedSince = 0;
  int autoSwitchTime = 500;

  ENUM_BUTTON pressedButton = getButton();

  if(pressedButton == NONE && lastRead != 0) {
    isPressedSince = 0;
    return;
  }
  if(pressedButton != lastPressedButton) {
    isPressedSince = 0;
  }

  if(isPressedSince > 3) autoSwitchTime = 70;
  if(lastRead != 0 && millis() - lastRead < autoSwitchTime && pressedButton == lastPressedButton) return;

  isPressedSince++;
  lastRead = millis();
  lastPressedButton = pressedButton;
  
  switch(pressedButton) {
    case NEXT: handleNext(); break;
    case PREV: handlePrev(); break;
    case BACK: handleBack(); break;
    case OK: handleOk(); break;
  }

  lcd.home();
  lcd.clear();
  if(isInLowerLevel) {
    lcd.print(menu[currentMenuPos].label);
    lcd.setCursor(0, 1);
    lcd.print(F("> "));

    if(menu[currentMenuPos].handler != NULL) {
      (*(menu[currentMenuPos].handler))();
    } else {
      lcd.print(tempVal);
    }
  } else {
    lcd.print(F("Menu:"));
    lcd.setCursor(0, 1);
    lcd.print(F("> "));

    lcd.print(menu[currentMenuPos].label);
  }
}

void handleNext() {
  if(isInLowerLevel) {
    tempVal++;
    if(tempVal > menu[currentMenuPos].maxVal) tempVal = menu[currentMenuPos].maxVal;
  } else {
    currentMenuPos = (currentMenuPos + 1) % menuSize;
  }
}

void handlePrev() {
  if(isInLowerLevel) {
    tempVal--;
    if(tempVal < menu[currentMenuPos].minVal) tempVal = menu[currentMenuPos].minVal;
  } else {
    currentMenuPos--;
    if(currentMenuPos < 0) currentMenuPos = menuSize - 1;
  }
}

void handleBack() {
  if(isInLowerLevel) {
    isInLowerLevel = false;
  }
}

void handleOk() {
  if(menu[currentMenuPos].handler != NULL && menu[currentMenuPos].maxVal <= menu[currentMenuPos].minVal) {
    (*(menu[currentMenuPos].handler))();
    return;
  }
  if(isInLowerLevel) {
    menu[currentMenuPos].currentVal = tempVal;
    isInLowerLevel = false;
  } else {
    tempVal = menu[currentMenuPos].currentVal;
    isInLowerLevel = true;
  }
}

/* User function for menu */
void initAlgorithm() {
  lcd.clear();
  long randNumber;
  // 1) "Start" information
  lcd.print("Inicjalizacja...");
  // 2) Turn on LED lamp - ledPin = HIGH
  digitalWrite(ledPin, HIGH);
  
  // 3) Generate 1-5 s random number and wait that time
  randNumber = random(1000, 5000);
  delay(randNumber);
  lcd.clear();
  // 3.5) If proximity sensor is OFF then show falstart information
  if (digitalRead(ProxSensorPin) == HIGH) // falstart or something is wrong
  {
    lcd.clear();
    lcd.print("Falstart");
    digitalWrite(ledPin, LOW);
    return;
  }
  
  // 4) Release the strap - strapPin = HIGH
  digitalWrite(strapPin, HIGH);
  lcd.clear();
  
  // 5) Start counting time in real values
  long i = 0;
  while(1) 
  {
    delay(1); // 1ms resolution
    // 6) Check proximity sensor pin.
    //    if ON then continue counting
    //    if OFF then stop counting
    if (digitalRead(ProxSensorPin) == HIGH) break; // If sensor is OFF      
    if (getButton() == BACK) break; // STOP buttom for finishing loop in any moment
    
    
    i++;
  } 
  
  // 7) Show result on LCD
  lcd.print("Czas: ");
  lcd.setCursor(0,1);
  lcd.print(i);
  lcd.print(" ms");
  // Finish all OFF
  digitalWrite(ledPin, LOW);
  digitalWrite(strapPin, LOW);
}

void proximityCalibration() {
  while(!(getButton() == BACK))
  {
    delay(200);
    lcd.clear();
    if (digitalRead(ProxSensorPin) == HIGH)
    {
      lcd.print("Sensor:OFF");
    } else  {
      lcd.print("Sensor:ON"); 
    } 
  }
}

void ledTest() {
  lcd.clear();
  lcd.print("LED: ON/OFF");
  digitalWrite(ledPin, HIGH);
  delay(1000);
  digitalWrite(ledPin, LOW);
}

void strapTest() {
  lcd.clear();
  lcd.print("STRETCH: ON/OFF");
  digitalWrite(strapPin, HIGH);
  delay(1000);
  digitalWrite(strapPin, LOW);
}

void authorShow() {
  lcd.clear();
  lcd.print("Damian Glowienka");
}

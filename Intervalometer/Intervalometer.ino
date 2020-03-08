#include <U8g2lib.h>
#include <OneWire.h>
#include <TimerOne.h>

#define Trigger 8

const int PinA = 3;
const int PinB = 2;
const int EncoderButton = 4;
int lastCount = 50;
volatile int virtualPosition = 50;
short menuCurrent = 1;
short interval = 15; //Default
short exposure = 0; //Default
int countdown = 0;
long countdownTimer1, countdownTimer2;
boolean awake = true;
boolean buttonActive = false;
boolean longPressActive = false;
long buttonTimer = 0;
long buttonTimer2 = 0;
long longPressTime = 250;
String output = "";
String lastFrame = "";
boolean startSleep = false;

U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 6);

/*
 * Adjusts selected menu item based on increment/decrement parameter
 */
void adjustValue(boolean increment) {
  short *pointer;
  switch (menuCurrent) {
    case 1:
      pointer = &interval;
      break;
    case 2:
      pointer = &exposure;
      break;
  }
  if (increment) {
    *pointer += 1;
  } else {
    if (menuCurrent == 1 && *pointer != 1) {
      *pointer -= 1;
    } else if (menuCurrent == 2 && *pointer != 0) {
      *pointer -= 1;
    }
  }
}

void physticalTrigger() {
  digitalWrite(Trigger, HIGH);
  delay(200);
  digitalWrite(Trigger, LOW);
}

void trigger() {
  output = "Triggering";
  updateScreen();
  if (exposure == 0) {
  //trigger 0.01 seconds
  physticalTrigger();
  } else {
  //trigger 0.01 seconds
  physticalTrigger();
  delay(exposure * 1000);
  //trigger 0.01 seconds
  physticalTrigger();
  }
}

void updateScreen() {
  if (!output.equals(lastFrame)) {
    u8g2.clearDisplay();
    lastFrame = output;
  }

  do {
    //u8g2.setFont(u8g2_font_7x14_mf); //u8g2.setFont(u8g2_font_diodesemimono_tr);
    u8g2.setCursor(10,24);
    u8g2.print(output);
  } while (u8g2.nextPage());
}

void sleepCycle() {
  countdownTimer2 = millis();
  while (countdownTimer2 - countdownTimer1 < 1000) {
    countdownTimer2 = millis();
  }
  Serial.println(countdownTimer2 - countdownTimer1);
}

void isr ()  {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 5) {
    if (digitalRead(PinB) == LOW)
    {
      virtualPosition-- ; // Could be -5 or -10
    }
    else {
      virtualPosition++ ; // Could be +5 or +10
    }
  }
  lastInterruptTime = interruptTime;
}

void setup() {
  u8g2.begin();
  u8g2.setDisplayRotation(U8G2_R2);
  pinMode(Trigger, OUTPUT);
  
  pinMode(PinA, INPUT_PULLUP);
  pinMode(PinB, INPUT_PULLUP);
  pinMode(EncoderButton, INPUT_PULLUP);
  pinMode (6, OUTPUT); //temp
  attachInterrupt(digitalPinToInterrupt(PinA), isr, LOW);

  u8g2.clearDisplay();
  u8g2.setFont(u8g2_font_7x14_mf);

  Serial.begin(9600);
  physticalTrigger();
}

void loop() {
  if (awake) {
    if ((!digitalRead(EncoderButton))) {
      virtualPosition = 50;
      buttonTimer = millis();
      while (!digitalRead(EncoderButton))
        buttonTimer2 = millis();
        if ((buttonTimer2 - buttonTimer) > longPressTime) {
          awake = !awake;
          startSleep = true;
        } else {
          if (menuCurrent == 1) {
            menuCurrent = 2;
            output = "Interval: ";
            output += interval;
            updateScreen();
          } else {
            menuCurrent = 1;
            output = "Exposure: ";
            output += exposure;
            updateScreen();
          }
        }
    }
    if (virtualPosition != lastCount) {
      virtualPosition > lastCount ? adjustValue(true) : adjustValue(false);
      lastCount = virtualPosition ;
    }
    switch (menuCurrent) {
      case 1:
        output = "Interval: ";
        output += interval;
        break;
      case 2:
        output = "Exposure: ";
        output += exposure;
        break;
    }
    updateScreen();
  } else {
    countdownTimer1 = millis();
    if ((!digitalRead(EncoderButton))) {
      buttonTimer = millis();
      while (!digitalRead(EncoderButton))
        buttonTimer2 = millis();
        if ((buttonTimer2 - buttonTimer) > longPressTime) {
          awake = !awake;
        }
    } else {
      if (startSleep) {
      countdown = interval;
      startSleep = false;
      }
    
      if (countdown > 0) {
        if (countdown < 10) {
          output = "0";
          output += countdown;
        } else {
          output = countdown;
        }
        countdown -= 1;
        updateScreen();
        sleepCycle();
      } else {
        output = "00";
        countdown = interval;
        updateScreen();
        sleepCycle();
        trigger();
      }
    }
  }
}

//#pragma once
#include <U8g2lib.h>
#include <OneWire.h>
#include <TimerOne.h>
#include <avdweb_Switch.h>

#define SerialOutput 0
#define Trigger 8
#define MenuSize 5
Switch cycleButton = Switch(5);
Switch selectButton = Switch(4);
Switch downButton = Switch(3);
Switch upButton = Switch(2);

short menuCurrent = 1;
float interval = 5; //Default
float exposure = 0.00; //Default
int countdown = 0;
long countdownTimer1, countdownTimer2; //timer 1 is used as triggering timer, 2 is used for interval timer
String output = "";
String lastFrame = "";
bool awake = true;
short refreshRate = 50;
short refreshRateMS = 1000/refreshRate; //floor division, gets rid of decimal point
long lastDisplayed = 0;
U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 7);

/*
Menu Items:
1: Interval
2: Exposure (seconds)
3: Exposure (decisecond)
4: Exposure (centisecond)
5: Start
*/

/*
 * Adjusts current menu item based on increment/decrement parameter
 */
void adjustValue(boolean increment) {
    float *pointer;
    switch (menuCurrent) {
        case 1:
            pointer = &interval;
            break;
        case 2:
        case 3:
        case 4:
            pointer = &exposure;
            break;
    }
    if (increment) {
        switch(menuCurrent) {
            case 1:
                *pointer += 1;
                break;
            case 2:
                *pointer += 1;
                break;
            case 3:
                *pointer += 0.1;
                break;
            case 4:
                *pointer += 0.01;
                break;
        }
    } else {
        switch(menuCurrent) {
            case 1:
                if (*pointer != 1) {
                    *pointer -= 1;
                }
                break;
            case 2:
                if (*pointer - 1 >= 0.00) {
                    *pointer -= 1;
                } else {
                    *pointer = 0;
                }
                break;
            case 3:
                if (*pointer - 0.1 >= 0.00) {
                    *pointer -= 0.1;
                } else {
                    *pointer = 0;
                }
                break;
            case 4:
                if (*pointer - 0.01 >= 0.00) {
                    *pointer -= 0.01;
                } else {
                    *pointer = 0;
                }
                break;
        }
    }
}



/*
 * Triggers the wireless remote
 * ~0.2 seconds runtime
 */
void physicalTrigger() {
    digitalWrite(Trigger, HIGH);
    delay(200); //200ms
    digitalWrite(Trigger, LOW);
}
//bool trigger/release
//Function overloading doesn't work with .ino files, only .cpp
void physicalTriggerBulb(bool trigger) {
    if (trigger) {
        digitalWrite(Trigger, HIGH);
    } else {
        digitalWrite(Trigger, LOW);
    }
}
/*
 * Triggers remote with necessary delays
 */
void trigger() {
    if (!exposure) {
        physicalTrigger(); //trigger 0.2 seconds
    } else {
        physicalTriggerBulb(true);
        countdownTimer1 = millis();
        long triggerCountdown = exposure * 1000;
        long differenceMS = 0;
        long lastRun = millis();
        while (triggerCountdown > 0) {
            long localMS = millis();
            differenceMS = localMS - lastRun;
            triggerCountdown -= differenceMS;
            lastRun = localMS;
            
            output = "T-";
            output += triggerCountdown / 1000;
            output += ".";
            output += (triggerCountdown % 1000) / 100;
            output += "s";
            updateScreen(false);
        }
        output = "T-0s";
        updateScreen(true);
        
        physicalTriggerBulb(false);     //trigger release 0.01 seconds
    }
}

/*
 * Updates screen if output data isn't the same as previous frame
 */
void updateScreen(bool force) {
    if (millis() - lastDisplayed >= refreshRateMS) {
        if (!output.equals(lastFrame)) {
            u8g2.clearDisplay();
            lastFrame = output;
            do {
                //u8g2.setFont(u8g2_font_7x14_mf); //u8g2.setFont(u8g2_font_diodesemimono_tr);
                u8g2.setCursor(10,24);
                u8g2.print(output);
            } while (u8g2.nextPage());
        }
        lastDisplayed = millis();
    } else if (force) {
        u8g2.clearDisplay();
        lastFrame = output;
        do {
            //u8g2.setFont(u8g2_font_7x14_mf); //u8g2.setFont(u8g2_font_diodesemimono_tr);
            u8g2.setCursor(10,24);
            u8g2.print(output);
        } while (u8g2.nextPage());
    }
}

void setup() {
    u8g2.begin();
    u8g2.setDisplayRotation(U8G2_R2);
    pinMode(Trigger, OUTPUT);
    pinMode(2, INPUT_PULLUP);
    pinMode(3, INPUT_PULLUP);
    pinMode(4, INPUT_PULLUP);
    pinMode(5, INPUT_PULLUP);
    //pinMode(6, INPUT_PULLUP);
  
    u8g2.clearDisplay();
    u8g2.setFont(u8g2_font_7x14_mf);
#ifndef SerialOutput == 1
    Serial.begin(9600);
#endif
    output = "Starting...";
    updateScreen(true);
    awakeOutput(true);
}

void awakeOutput(bool force) {
    if (menuCurrent == 1) {
        output = "Interval: ";
        output += (int)interval;
        output += 's';
    } else if (menuCurrent == 5) {
        output = "Start";
    } else {
        output = "Exposure: ";
        output += exposure;
        switch(menuCurrent) {
            case 2:
                output += 's';
                break;
            case 3:
                output += "ds";
                break;
            case 4:
                output += "cs";
                break;
        }
    }
#ifndef SerialOutput == 1
    Serial.println(output);
#endif
    updateScreen(force);
}

void cycleAction() {
    if(cycleButton.pushed()) {
        if(menuCurrent == MenuSize) {
            menuCurrent = 1;
        } else {
          menuCurrent++;
        }
        awakeOutput(false);
    }
}

void upAction() {
    if(upButton.pushed()) {
        adjustValue(true);
        
        awakeOutput(false);
        updateScreen(false);
    }
}

void downAction() {
    if(downButton.pushed()) {
        adjustValue(false);
        awakeOutput(false);
    }
}

void resetCountdown() {
    countdown = interval - 1;
    output = "Countdown: ";
    output += countdown;
    updateScreen(true);
    countdownTimer2 = millis();
}

void selectAction() {
    if(selectButton.pushed()) {
        if (menuCurrent == MenuSize) {
            awake = !awake;
            if(!awake) {
                resetCountdown();
            } else {
                awakeOutput(false);
            }
        }
    }
}

void loop() {
    if (awake) {
        downButton.poll();
        upButton.poll();
        cycleButton.poll();
        selectButton.poll();
        cycleAction();
        upAction();
        downAction();
        selectAction();
    } else {
        selectButton.poll();
        selectAction();
        if (millis() - countdownTimer2 >= 1000) {
            countdown--;
            if (countdown > -1) {
                output = "Countdown: ";
                output += countdown;
                updateScreen(false);
                countdownTimer2 = millis();
            } else if (countdown == -1) {
                output = "Triggering...";
                updateScreen(false);
                trigger();
                resetCountdown();
            }
        }
    }
}

/*
 *  SimpleReceiver.cpp
 *
 *  Receives IR protocol data of 15 main protocols.
 *
 *  *****************************************************************************************************************************
 *  To access the library files from your sketch, you have to first use `Sketch > Show Sketch Folder (Ctrl+K)` in the Arduino IDE.
 *  Then navigate to the parallel `libraries` folder and select the library you want to access.
 *  The library files itself are located in the `src` sub-directory.
 *  If you did not yet store the example as your own sketch, then with Ctrl+K you are instantly in the right library folder.
 *  *****************************************************************************************************************************
 *
 *
 *  The following IR protocols are enabled by default:
 *      Sony SIRCS
 *      NEC + APPLE
 *      Samsung + Samsg32
 *      Kaseikyo
 *
 *      Plus 11 other main protocols by including irmpMain15.h instead of irmp.h
 *      JVC, NEC16, NEC42, Matsushita, DENON, Sharp, RC5, RC6 & RC6A, IR60 (SDA2008) Grundig, Siemens Gigaset, Nokia
 *
 *  To disable one of them or to enable other protocols, specify this before the "#include <irmp.h>" line.
 *  If you get warnings of redefining symbols, just ignore them or undefine them first (see Interrupt example).
 *  The exact names can be found in the library file irmpSelectAllProtocols.h (see Callback example).
 *
 *  Copyright (C) 2019  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of IRMP https://github.com/IRMP-org/IRMP.
 *
 *  IRMP is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>
#include "pins.h"

int pins[] = {4, 5, 6, 7, 8, 9, 10, 11};
int gnds[] = {2, 3};

int activeNumber;
int tmpNum;

int states[][8] = {
    {0, 1, 1, 1, 1, 1, 1, 0}, // 0
    {0, 0, 0, 0, 1, 1, 0, 0}, // 1
    {1, 0, 1, 1, 0, 1, 1, 0}, // 2
    {1, 0, 0, 1, 1, 1, 1, 0}, // 3
    {1, 1, 0, 0, 1, 1, 0, 0}, // 4
    {1, 1, 0, 1, 1, 0, 1, 0}, // 5
    {1, 1, 1, 1, 1, 0, 1, 0}, // 6
    {0, 0, 0, 0, 1, 1, 1, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1, 0}, // 8
    {1, 1, 0, 1, 1, 1, 1, 0}, // 9
    {1, 1, 1, 0, 1, 1, 1, 0}, // A
    {1, 1, 1, 1, 1, 0, 0, 0}, // b
    {0, 1, 1, 1, 0, 0, 1, 0}, // C
    {1, 0, 1, 1, 1, 1, 0, 0}, // d
    {1, 1, 1, 1, 0, 0, 1, 0}, // E
    {1, 1, 1, 0, 0, 0, 1, 0} // F
};

void setDigit(int num, int gnd_act, int gnd_f)
{
    if (num > 15 || num < 0)
    {
        return;
    }
    digitalWrite(gnd_act, 0);
    digitalWrite(gnd_f, 1);
    for (int i = 0; i < 8; i++)
    {
        digitalWrite(pins[i], states[num][i]);
    }
}

void setNum(int num)
{
    setDigit(num / 100, gnds[0], gnds[1]);
    delay(10);
    //  Serial.println(num / 10);
    setDigit(num % 100, gnds[1], gnds[0]);
    delay(10);
    // Serial.println(num % 10);
}

int getLongFormat(int num) {
    return ((num / 16) * 100) + num % 16;
}

/*
 * Set input pin and output pin definitions etc.
 */
#include "PinDefinitionsAndMore.h"
#define IRMP_INPUT_PIN 12

#define IRMP_PROTOCOL_NAMES 1 // Enable protocol number mapping to protocol strings - requires some FLASH. Must before #include <irmp*>

#include <irmpSelectMain15Protocols.h> // This enables 15 main protocols
// #define IRMP_SUPPORT_NEC_PROTOCOL        1 // this enables only one protocol
// #define IRMP_SUPPORT_SIRCS_PROTOCOL      1 // this enables only one protocol

/*
 * We use LED_BUILTIN as feedback for commands 0x40 and 0x48 and cannot use it as feedback LED for receiving
 */
#if defined(ALTERNATIVE_IR_FEEDBACK_LED_PIN)
#define IRMP_FEEDBACK_LED_PIN ALTERNATIVE_IR_FEEDBACK_LED_PIN
#endif
/*
 * After setting the definitions we can include the code and compile it.
 */
#include <irmp.hpp>

IRMP_DATA irmp_data;

void setup()
{
    digitalWrite(RESET_PIN, 1);
    pinMode(RESET_PIN, OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(9600);

    // Just to know which program is running on my Arduino
    irmp_init();
    for (int i = 0; i < 2; i++)
    {
        pinMode(gnds[i], OUTPUT);
    }
    for (int i = 0; i < 8; i++)
    {
        pinMode(pins[i], OUTPUT);
    }
    activeNumber = 0;
    tmpNum = 0;

    /*
     * We use LED_BUILTIN as feedback for commands 0x40 and 0x48 and cannot use it as feedback LED for receiving
     */
    /*#if defined(ALTERNATIVE_IR_FEEDBACK_LED_PIN)
        irmp_irsnd_LEDFeedback(true); // Enable receive signal feedback at ALTERNATIVE_IR_FEEDBACK_LED_PIN
        Serial.println(F("IR feedback pin is " STR(ALTERNATIVE_IR_FEEDBACK_LED_PIN)));
    #endif
    */
   Serial.println("Started");
}

void loop()
{
    /*
     * Check if new data available and get them
     */
    if (irmp_get_data(&irmp_data))
    {
        /*
         * Skip repetitions of command
         */
        if (!(irmp_data.flags & IRMP_FLAG_REPETITION))
        {
            /*
             * Here data is available and is no repetition -> evaluate IR command
             */
            Serial.print("Data: ");
            Serial.println(irmp_data.command);
            if (irmp_data.command >= LOW_NUM && irmp_data.command <= HIGH_NUM)
            {
                if(tmpNum < MAX) {
                tmpNum *= 10;
                if(irmp_data.command != HIGH_NUM) {
                    tmpNum += irmp_data.command - LOW_NUM + 1;
                }
                Serial.print("Input: ");
                Serial.println(tmpNum);
                }
                
            }
            else
            {
                switch (irmp_data.command)
                {
                case ENTER:
                    if(tmpNum < MAX) {
                        activeNumber = getLongFormat(tmpNum);
                        Serial.println(activeNumber);
                    } 
                    tmpNum = 0;
                    break;
                case DEL:
                    tmpNum /= 10;
                    break;
                case OFF:
                    digitalWrite(RESET_PIN, LOW);
                    break;
                case STOP:
                    tmpNum = 0;
                    break;
                default:
                    break;
                }
            }
        }
        (&irmp_data);
    }
    setNum(activeNumber);
}

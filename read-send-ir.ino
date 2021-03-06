/*
 *  IR Remote Module
 *
 *  Copyright (C) Libelium Comunicaciones Distribuidas S.L.
 *  http://www.libelium.com
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  a
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see http://www.gnu.org/licenses/.
 *
 *  Version:           1.0
 *  Design:            David Gascón
 *  Implementation:    Luis Martin
 */

// We need to use the 'raw' pin reading methods
// because timing is very important here and the digitalRead()
// procedure is slower!
#include "Arduino.h"
#include <stdint.h>

#define IRpin_PIN PIND
#define IRpin 2

// The maximum pulse we'll listen for - 65 milliseconds is a long time
#define MAXPULSE 65000

// Timing resolution
#define RESOLUTION 20

int IRledPin =  3;      // IR emitter LED connected to digital pin 3
int orangeLedPin =  7;  // Orange LED connected to digital pin 7
int sendButton = 4;     // "Send" push-button connected to digital pin 4
int blueLedPin =  6;    // Blue LED connected to digital pin 6
int receiveButton = 5;     // "Receive" push-button connected to digital pin 5

int sendButtonState = 0;
int receiveButtonState = 0;

// Store up to 100 pulse pairs
uint16_t pulses[100][2]; // Pair is high and low pulse
uint8_t currentpulse = 0; // Index for pulses we're storing
uint8_t sendpulse = 0; // Index for send pulses

// This procedure sends a 38KHz pulse to the IRledPin for a certain # of microseconds.
void pulseIR(long microsecs) {

  cli();  // Turn off any background interrupts

  while (microsecs > 0) {
   // 38 kHz is about 13 microseconds high and 13 microseconds low
   digitalWrite(IRledPin, HIGH);  // 3 microseconds
   delayMicroseconds(9);         /* hang out for 10 microseconds,
                                     you can also change this to 9 if its not working */
   digitalWrite(IRledPin, LOW);   // 3 microseconds
   delayMicroseconds(9);         /* hang out for 10 microseconds,
                                     you can also change this to 9 if its not working */

   // So 26 microseconds altogether
   microsecs -= 24;
  }

  sei();  // Turn them back on
}

void SendIRCode() {

  for (uint8_t i = 0; i < sendpulse; i++) {

    if (i != 0){
    delayMicroseconds(pulses[i][0] * RESOLUTION);
    }

    pulseIR(pulses[i][1] * RESOLUTION);
  }
}

void setup() {
  // Initialize the Orange LED pin as an output
  pinMode(orangeLedPin, OUTPUT);
  // Initialize the "Send" push-button pin as an input
  pinMode(sendButton, INPUT);
  // Initialize the Orange LED pin as an output
  pinMode(blueLedPin, OUTPUT);
  // Initialize the "Send" push-button pin as an input
  pinMode(receiveButton, INPUT);
  // Set uart baudrate
  Serial.begin(9600);
}

void printpulses(void) {
  Serial.println("\n\r\n\rReceived:");
  for (uint8_t i = 0; i < currentpulse; i++) {

    if (i != 0){
    Serial.print("delayMicroseconds(");
    Serial.print(pulses[i][0] * RESOLUTION);
    Serial.println(");");
    }
    Serial.print("pulseIR(");
    Serial.print(pulses[i][1] * RESOLUTION);
    Serial.println(");");
  }
  delay(1000);
  digitalWrite(orangeLedPin, HIGH);
  delay(200);
  digitalWrite(orangeLedPin, LOW);
  delay(200);

/*  // Print it in a 'array' format
  Serial.println("int IRsignal[] = {");
  Serial.println("// ON, OFF (in 10's of microseconds)");
  for (uint8_t i = 0; i < currentpulse-1; i++) {
    Serial.print("\t"); // tab
    Serial.print(pulses[i][1] * RESOLUTION / 10, DEC);
    Serial.print(", ");
    Serial.print(pulses[i+1][0] * RESOLUTION / 10, DEC);
    Serial.println(",");
  }
  Serial.print("\t"); // tab
  Serial.print(pulses[currentpulse-1][1] * RESOLUTION / 10, DEC);
  Serial.print(", 0};");
*/
}

void loop() {
   // Read the state of the "Send" push-button value
  sendButtonState = digitalRead(sendButton);
  // Read the state of the "Receive" push-button value
  receiveButtonState = digitalRead(receiveButton);

if (sendButtonState == HIGH) {
    Serial.println("Sending IR signal");
    digitalWrite(orangeLedPin, HIGH);
    delay(200);
    digitalWrite(orangeLedPin, LOW);
    delay(200);

    printpulses();
    SendIRCode();
    //delay(15);  // wait 15 milliseconds before sending it again
    //SendIRCode();  // repeat IR code if it is neccesary

    delay(5000);  // wait 5 seconds to resend the code
    }

 if ((receiveButtonState==HIGH) || (currentpulse != 0)){

   if (receiveButtonState==HIGH){
    Serial.println("Ready to decode IR!");
    digitalWrite(blueLedPin, HIGH);
    delay(200);
    digitalWrite(blueLedPin, LOW);
    delay(200);
   }

  uint16_t highpulse, lowpulse; // Temporary storage timing
  highpulse = lowpulse = 0; // Start out with no pulse length

    while (IRpin_PIN & (1 << IRpin)) {
     // count off another few microseconds
     highpulse++;
     delayMicroseconds(RESOLUTION);

     // If the pulse is too long, we 'timed out' - either nothing
     // was received or the code is finished, so print what
     // we've grabbed so far, and then reset
     if ((highpulse >= MAXPULSE) && (currentpulse != 0)) {
       printpulses();
       sendpulse=currentpulse;
       currentpulse=0;
       return;
     }
  }
  // we didn't time out so lets stash the reading
  pulses[currentpulse][0] = highpulse;

  // same as above
  while (! (IRpin_PIN & _BV(IRpin))) {
     // pin is still LOW
     lowpulse++;
     delayMicroseconds(RESOLUTION);
     if ((lowpulse >= MAXPULSE) && (currentpulse != 0)) {
       printpulses();
       sendpulse=currentpulse;
       currentpulse=0;
       return;
     }
  }
  pulses[currentpulse][1] = lowpulse;

  // we read one high-low pulse successfully, continue!
  currentpulse++;

 }
}


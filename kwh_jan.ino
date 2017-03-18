#include <ESP8266WiFi.h>
#include <inttypes.h>

#define READINGS       250
#define MS_PER_HOUR    3.6e6

int ledPin = 2; // GPIO2 of ESP8266

// Power measurement settings
unsigned short cycles_per_kwh = 400;
unsigned char  loThresholdP = 101;
unsigned char  hiThresholdP = 110;
unsigned long debounceTimeP = 600;
boolean markerState = LOW;


void setup () {
  
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
}

unsigned long cycle = 0;
unsigned long previous = 0; // timestamp
unsigned short readings[READINGS];
unsigned short cursor = 0;
boolean gotenough = false;
unsigned short hits = 0;
  
void loop () {

  //  Calulate the sum of 40 samples
  unsigned short sum = 0;
  for (byte i = 0; i < 40; i++) {
    sum += analogRead(A0);
  }

  // Calculate average over 250 sum samples 
  unsigned long bigsum = 0;
  for (unsigned short i = 0; i < READINGS; i++){
    bigsum += readings[i];
  }
  unsigned short average = bigsum / READINGS;

//  Calculate the ratio of the sum samples and the 250 sum samples multipied by 100
  unsigned short ratio = (double) sum / (average+1) * 100;
  
//   Keep 250 sum samples in the reading array

   readings[cursor++] = sum;
   if (cursor >= READINGS) {
    cursor = 0;
   }

// If markerState has not changed, make newmarkerState high if ration > lo. If the markerState HAS changed make newledstae low if ratio >= hi
  boolean newmarkerState = markerState 
    ? (ratio >  loThresholdP)
    : (ratio >= hiThresholdP);

  if (newmarkerState) hits++;

// Return if the markerstate has not changed
  if (newmarkerState == markerState) return;

  markerState = newmarkerState;

//  LED is ON (low) if the marker is detected
  digitalWrite(ledPin, !markerState);

  if (!markerState) {
    Serial.print("Marker: ");
    Serial.print(millis() - previous);
    Serial.print(" ms (");
    Serial.print(hits, DEC);
    Serial.println(" readings)");
    hits = 0;
    return;
  }
  
  unsigned long now = millis();
  unsigned long time = now - previous;

  Serial.println(time);

  if (time < debounceTimeP) return;

  previous = now;  

// If cycle=0 discard the cycle. Post increase the cycle
  if (!cycle++) {
    Serial.println("Discarding incomplete cycle.");
    return;
  }
  
  double W = 1000 * ((double) MS_PER_HOUR / time) / cycles_per_kwh;
  Serial.print("Cycle ");
  Serial.print(cycle, DEC);
  Serial.print(": ");
  Serial.print(time, DEC);
  Serial.print(" ms, ");
  Serial.print(W, 2);
  Serial.println(" W");
}


#include <ESP8266WiFi.h>
//#include "EEPROM.h"
#include "AnythingEEPROM.h"
#include <inttypes.h>

#define READINGS       250
#define EEPROM_OFFSET  512
#define MS_PER_HOUR    3.6e6

struct SettingsStruct {
  unsigned short cycles_per_kwh = 400;
  unsigned char  lower_threshold = 101;
  unsigned char  upper_threshold = 105;
} settings;

boolean ledstate = LOW;
unsigned long debounce_time = 1600;

void setup () {
  
  Serial.begin(115200);
}

unsigned long cycle = 0;
unsigned long previous = 0; // timestamp

unsigned short readings[READINGS];
unsigned short cursor = 0;
boolean gotenough = false;

unsigned short hits = 0;
  
void loop () {
//  delay(10);


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
  
//   Read 250 sum samples in the reading array
   readings[cursor++] = sum;
   if (cursor >= READINGS) {
    cursor = 0;
   }


  unsigned short lo = settings.lower_threshold;
  unsigned short hi = settings.upper_threshold;

// If ledstate has not changed ledstate becomes ???
  boolean newledstate = ledstate 
    ? (ratio >  lo)
    : (ratio >= hi);

  int numleds = ratio - lo;
  if (numleds < 0) numleds = 0;
  if (numleds > 8) numleds = 8;
  unsigned long ledmask = 0xff >> 8 - numleds;
  if (newledstate) ledmask <<= 8;
   
  if ((!gotenough) || (!newledstate)) {
    readings[cursor++] = sum;
    if (cursor >= READINGS) {
      cursor = 0;
      if (!gotenough) {
        gotenough = true;
        Serial.println("Done averaging");
      }
    }
  }

  
  if (newledstate) hits++;
 
  if (newledstate == ledstate) return;
  
  digitalWrite(13, ledstate = newledstate);

  if (!ledstate) {
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

  if (time < debounce_time) return;

  previous = now;  
 
  if (!cycle++) {
    Serial.println("Discarding incomplete cycle.");
    return;
  }
  
  double W = 1000 * ((double) MS_PER_HOUR / time) / settings.cycles_per_kwh;
  Serial.print("Cycle ");
  Serial.print(cycle, DEC);
  Serial.print(": ");
  Serial.print(time, DEC);
  Serial.print(" ms, ");
  Serial.print(W, 2);
  Serial.println(" W");
}


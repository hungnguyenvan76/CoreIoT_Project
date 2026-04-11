#ifndef __NEO_BLINKY__
#define __NEO_BLINKY__
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
//#include "freertos/semphr.h"
//#include "freertos/queue.h"


#define NEO_PIN 45
#define LED_COUNT 1 

void neo_blinky(void *pvParameters);


#endif

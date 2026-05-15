
#ifndef __TASK_HANDLER_H__
#define __TASK_HANDLER_H__

#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Ticker.h>
#include <task_check_info.h>
#define NEO_PIN 45
#define LED_GPIO 48
#define LED_COUNT 1
extern void handleWebSocketMessage(String message);
#endif

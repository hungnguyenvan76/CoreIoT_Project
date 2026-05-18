#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

typedef struct {
    float temperature;
    float humidity;
} SensorData_t;

typedef struct {
    bool isManual; //false = follow AI/Sensor, true = Web control
    bool isOn;
    int delayMs;
} WsLedConfig_t;

typedef struct {
    bool isManual;
    bool isOn;
    int delayMs;
    uint8_t r, g, b;
} WsNeoConfig_t;

extern QueueHandle_t wsLedQueue;
extern QueueHandle_t wsNeoQueue;


extern String WIFI_SSID;
extern String WIFI_PASS;
extern String CORE_IOT_TOKEN;
extern String CORE_IOT_SERVER;
extern String CORE_IOT_PORT;

extern boolean isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern QueueHandle_t sensorQueue;
extern QueueHandle_t aiQueue;

#endif

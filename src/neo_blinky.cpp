#include "neo_blinky.h"

void neo_blinky(void *pvParameters){
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    SensorData_t receivedData;
    int ai_state = -1;

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.show();

    uint32_t current_color = strip.Color(0, 255, 0); // Default is green
    TickType_t current_delay = pdMS_TO_TICKS(1000);
    bool led_is_on = false;
    float current_humidity = 0.0;

    WsNeoConfig_t wsState = {false, false, 1000, 0, 0, 0};

    while(1) {                      
        if (aiQueue != NULL) {
            xQueuePeek(aiQueue, &ai_state, 0);
        }

        if (wsNeoQueue != NULL) {
            xQueuePeek(wsNeoQueue, &wsState, 0);
        }

        #if is_board_1 // BOARD 1 RUNS NORMALLY
        // PRIORITY 1: WEBSOCKET OVERRIDE (Manual Control)
        if (wsState.isManual) {
            if (wsState.isOn) {
                if (led_is_on) strip.setPixelColor(0, strip.Color(wsState.r, wsState.g, wsState.b));
                else strip.clear();
                strip.show();
                led_is_on = !led_is_on;
                vTaskDelay(pdMS_TO_TICKS(wsState.delayMs));
            } else {
                strip.clear();
                strip.show();
                vTaskDelay(pdMS_TO_TICKS(100)); // AI
            }
        }
        // PRIORITY 2: AI CRITICAL WARNINGS
        else if (ai_state == 2) { // MOLD_RISK
            // Double Blink (Blue) for MOLD_RISK
            strip.setPixelColor(0, strip.Color(0, 0, 255)); strip.show(); vTaskDelay(pdMS_TO_TICKS(150));
            strip.setPixelColor(0, strip.Color(0, 0, 0));   strip.show(); vTaskDelay(pdMS_TO_TICKS(150));
            strip.setPixelColor(0, strip.Color(0, 0, 255)); strip.show(); vTaskDelay(pdMS_TO_TICKS(150));
            strip.setPixelColor(0, strip.Color(0, 0, 0));   strip.show(); vTaskDelay(pdMS_TO_TICKS(800));
        }
        else if (ai_state == 3) { // SENSOR_ERROR
            // Ultra-fast Red blinking for SENSOR_ERROR
            strip.setPixelColor(0, strip.Color(255, 0, 0)); strip.show(); vTaskDelay(pdMS_TO_TICKS(80));
            strip.setPixelColor(0, strip.Color(0, 0, 0));   strip.show(); vTaskDelay(pdMS_TO_TICKS(80));
        }
        else { // PRIORITY 3: DEFAULT BEHAVIOR (Green blinking based on humidity)
            if (xQueuePeek(queue, &receivedData, 0) == pdTRUE) {
                current_humidity = receivedData.humidity;
                current_color = strip.Color(100, 255, 100);
                
                if (current_humidity < 20.0) {
                    current_delay = pdMS_TO_TICKS(250);
                }
                else if (current_humidity >= 20.0 && current_humidity < 40.0) {
                    current_delay = pdMS_TO_TICKS(500);
                }            
                else if (current_humidity >= 40.0 && current_humidity < 70.0) {
                    current_delay = pdMS_TO_TICKS(750);
                }
                else {
                    current_delay = pdMS_TO_TICKS(1000);
                }
            }

            if (led_is_on) {
                strip.setPixelColor(0, current_color); 
            } else {
                strip.setPixelColor(0, strip.Color(0, 0, 0));
            }
            strip.show(); 
            led_is_on = !led_is_on;
            vTaskDelay(current_delay);
        }

        #else // BOARD 2 LOGIC
        if (ai_state == 1) { // FIRE_RISK
            strip.setPixelColor(0, strip.Color(255, 0, 0)); strip.show(); vTaskDelay(pdMS_TO_TICKS(200));
            strip.setPixelColor(0, strip.Color(0, 0, 0));   strip.show(); vTaskDelay(pdMS_TO_TICKS(200));
        } 
        else if (ai_state == 2) { // MOLD_RISK
            strip.setPixelColor(0, strip.Color(0, 0, 255)); strip.show(); vTaskDelay(pdMS_TO_TICKS(200));
            strip.setPixelColor(0, strip.Color(0, 0, 0));   strip.show(); vTaskDelay(pdMS_TO_TICKS(200));
        }
        else if (ai_state == 3) { // SENSOR_ERROR
            strip.setPixelColor(0, strip.Color(255, 255, 0)); strip.show(); vTaskDelay(pdMS_TO_TICKS(200));
            strip.setPixelColor(0, strip.Color(0, 0, 0));     strip.show(); vTaskDelay(pdMS_TO_TICKS(200));
        }
        else {
            strip.setPixelColor(0, strip.Color(10, 10, 10)); strip.show(); vTaskDelay(pdMS_TO_TICKS(500));
            strip.setPixelColor(0, strip.Color(0, 0, 0));    strip.show(); vTaskDelay(pdMS_TO_TICKS(500));
        }

        #endif
    }
}

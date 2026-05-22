#include "led_blinky.h"

void led_blinky(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);

  QueueHandle_t queue = (QueueHandle_t)pvParameters;
  SensorData_t receivedData;
  int ai_state = -1;

  TickType_t current_delay = pdMS_TO_TICKS(1000);
  bool led_is_on = false;
  float current_temperature = 0.0;

  WsLedConfig_t wsState = {false, false, 1000};

  while(1) {                        
    if (aiQueue != NULL) {
        xQueuePeek(aiQueue, &ai_state, 0);
    }
    
    if (wsLedQueue != NULL) {
            xQueuePeek(wsLedQueue, &wsState, 0);
    }
        
    // PRIORITY 1: WEBSOCKET OVERRIDE (Manual Control)
    if (wsState.isManual) {
        if (wsState.isOn) {
            if (led_is_on) digitalWrite(LED_GPIO, HIGH);
            else digitalWrite(LED_GPIO, LOW);
            led_is_on = !led_is_on;
            vTaskDelay(pdMS_TO_TICKS(wsState.delayMs));
        } else {
            digitalWrite(LED_GPIO, LOW);
            vTaskDelay(pdMS_TO_TICKS(100)); 
        }

    }
    // PRIORITY 2: AI CRITICAL WARNINGS
    else if (ai_state == 1) { // FIRE_RISK
        // Continuous rapid blinking for FIRE_RISK
        digitalWrite(LED_GPIO, HIGH); vTaskDelay(pdMS_TO_TICKS(50));
        digitalWrite(LED_GPIO, LOW);  vTaskDelay(pdMS_TO_TICKS(50));
    }
    else if (ai_state == 3) { // SENSOR_ERROR
        // SOS blinking pattern for SENSOR_ERROR (3 rapid blinks, long pause)
        for(int i = 0; i < 3; i++) {
            digitalWrite(LED_GPIO, HIGH); vTaskDelay(pdMS_TO_TICKS(100));
            digitalWrite(LED_GPIO, LOW);  vTaskDelay(pdMS_TO_TICKS(100));
        }
        vTaskDelay(pdMS_TO_TICKS(600)); // Pause between SOS cycles
    }
    else { // PRIORITY 3: DEFAULT BEHAVIOR (Applied when AI = -1, 0, 2, 4)
        if (xQueuePeek(queue, &receivedData, 0) == pdTRUE) {
            current_temperature = receivedData.temperature;
            if (current_temperature >= 32.0) {
                current_delay = pdMS_TO_TICKS(250);
            }
            else if (current_temperature >= 28.0 && current_temperature < 32.0) {
                current_delay = pdMS_TO_TICKS(500);
            }
            else if (current_temperature >= 20.0 && current_temperature < 28.0) {
                current_delay = pdMS_TO_TICKS(750);
            }        
            else {
                current_delay = pdMS_TO_TICKS(1000);
            }
        }

        if (led_is_on) {
            digitalWrite(LED_GPIO, HIGH); 
        } else {
            digitalWrite(LED_GPIO, LOW);  
        }

        led_is_on = !led_is_on;
        vTaskDelay(current_delay);    
    }
  }
}

#include "led_blinky.h"

void led_blinky(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);

  QueueHandle_t queue = (QueueHandle_t)pvParameters;
  SensorData_t receivedData;
  int ai_state = -1;

  TickType_t current_delay = pdMS_TO_TICKS(1000);
  bool led_is_on = false;
  float current_temperature = 0.0;

  while(1) {                        
    if (aiQueue != NULL) {
        xQueuePeek(aiQueue, &ai_state, 0);
    }
    
    // NHÁNH ƯU TIÊN: CẢNH BÁO TỪ AI
    if (ai_state == 1) { // FIRE_RISK
        // Chớp nháy liên tục
        digitalWrite(LED_GPIO, HIGH); vTaskDelay(pdMS_TO_TICKS(50));
        digitalWrite(LED_GPIO, LOW);  vTaskDelay(pdMS_TO_TICKS(50));
    }
    else if (ai_state == 3) { // SENSOR_ERROR
        // Nháy mã SOS (3 nháy nhanh, nghỉ lâu)
        for(int i = 0; i < 3; i++) {
            digitalWrite(LED_GPIO, HIGH); vTaskDelay(pdMS_TO_TICKS(100));
            digitalWrite(LED_GPIO, LOW);  vTaskDelay(pdMS_TO_TICKS(100));
        }
        vTaskDelay(pdMS_TO_TICKS(600)); // Nghỉ giữa các nhịp SOS
    }
    else { // NHÁNH MẶC ĐỊNH (Áp dụng khi AI = -1, 0, 2, 4)
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

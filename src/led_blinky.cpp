#include "led_blinky.h"

void led_blinky(void *pvParameters){
  pinMode(LED_GPIO, OUTPUT);

  QueueHandle_t queue = (QueueHandle_t)pvParameters;
  SensorData_t receivedData;

  TickType_t current_delay = pdMS_TO_TICKS(1000);
  bool led_is_on = false;
  float current_temperature = 0.0;
  while(1) {                        

    if (xQueuePeek(queue, &receivedData, 0) == pdTRUE) {

        current_temperature = receivedData.temperature;
        if (current_temperature >= 32.0) {
            current_delay = pdMS_TO_TICKS(250);
        }
        else if (current_temperature >= 28.0 && current_temperature < 32.0) {
            current_delay = pdMS_TO_TICKS(500);
        }
        else if (current_temperature >= 20 && current_temperature < 28.0) {
            current_delay = pdMS_TO_TICKS(750);
        }        
        else {
            current_delay = pdMS_TO_TICKS(1000);
        }
    }

    if (led_is_on) {
      digitalWrite(LED_GPIO, HIGH);  // turn the LED ON
    }
    else {
      digitalWrite(LED_GPIO, LOW);  // turn the LED OFF
    }

    led_is_on = !led_is_on;
    vTaskDelay(current_delay);    
  }
}

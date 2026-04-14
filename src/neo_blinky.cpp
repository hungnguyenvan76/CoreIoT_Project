#include "neo_blinky.h"

void neo_blinky(void *pvParameters){

    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    SensorData_t receivedData;

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    // Set all pixels to off to start
    strip.clear();
    strip.show();

    uint32_t current_color = strip.Color(0, 0, 0); 
    TickType_t current_delay = pdMS_TO_TICKS(1000);
    bool led_is_on = false;
    float current_humidity = 0.0;

    while(1) {      
        /*
        strip.setPixelColor(0, strip.Color(255, 0, 0)); // Set pixel 0 to red
        strip.show(); // Update the strip

        // Wait for 500 milliseconds
        vTaskDelay(500);

        // Set the pixel to off
        strip.setPixelColor(0, strip.Color(0, 0, 0)); // Turn pixel 0 off
        strip.show(); // Update the strip

        // Wait for another 500 milliseconds
        vTaskDelay(500);
        */                    
        if (xQueuePeek(queue, &receivedData, 0) == pdTRUE) {

            current_humidity = receivedData.humidity;
            if (current_humidity < 20.0) {
                current_color = strip.Color(255, 0, 0); // set pixel 0 to red
                current_delay = pdMS_TO_TICKS(250);
            }
            else if (current_humidity >= 20.0 && current_humidity < 40.0) {
                current_color = strip.Color(255, 0, 0); // set pixel 0 to red
                current_delay = pdMS_TO_TICKS(500);
            }            
            else if (current_humidity >= 40.0 && current_humidity < 70.0) {
                current_color = strip.Color(0, 255, 0); // set pixel 0 to green
                current_delay = pdMS_TO_TICKS(750);
            }
            else {
                current_color = strip.Color(0, 0, 255);
                current_delay = pdMS_TO_TICKS(1000);
            }
        }
        if (led_is_on) {
            strip.setPixelColor(0, current_color); 
        }
        else {
            strip.setPixelColor(0, strip.Color(0, 0, 0));
        }
        strip.show(); //update
        led_is_on = !led_is_on;
        vTaskDelay(current_delay);
    }
}

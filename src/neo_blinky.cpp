#include "neo_blinky.h"

void neo_blinky(void *pvParameters){

    QueueHandle_t humidityQueue = (QueueHandle_t)pvParameters;

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
        if (xQueueReceive(humidityQueue, &current_humidity, 0)) {
            if (current_humidity < 40.0) {
                current_color = strip.Color(255, 0, 0); // set pixel 0 to red
                led_is_on = true;
                current_delay = pdMS_TO_TICKS(1000);
            }
            else if (current_delay >= 40 && current_delay < 70.0) {
                current_color = strip.Color(0, 255, 0); // set pixel 0 to green
                led_is_on = true;
                current_delay = pdMS_TO_TICKS(500);
            }
            else {
                current_color = strip.Color(0, 0, 255);
                led_is_on = true;
                current_delay = pdMS_TO_TICKS(200);
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

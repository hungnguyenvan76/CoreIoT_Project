#include "neo_blinky.h"

void neo_blinky(void *pvParameters){

    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    SensorData_t receivedData;
    int ai_state = -1;

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    // Set all pixels to off to start
    strip.clear();
    strip.show();

    uint32_t current_color = strip.Color(0, 255, 0); // Mặc định xanh lá
    TickType_t current_delay = pdMS_TO_TICKS(1000);
    bool led_is_on = false;
    float current_humidity = 0.0;

    while(1) {                      
        if (aiQueue != NULL) {
            xQueuePeek(aiQueue, &ai_state, 0);
        }

        // NHÁNH ƯU TIÊN: CẢNH BÁO TỪ AI
        if (ai_state == 2) { // MOLD_RISK
            // Double Blink (Xanh dương)
            strip.setPixelColor(0, strip.Color(0, 0, 255)); strip.show(); vTaskDelay(pdMS_TO_TICKS(150));
            strip.setPixelColor(0, strip.Color(0, 0, 0));   strip.show(); vTaskDelay(pdMS_TO_TICKS(150));
            strip.setPixelColor(0, strip.Color(0, 0, 255)); strip.show(); vTaskDelay(pdMS_TO_TICKS(150));
            strip.setPixelColor(0, strip.Color(0, 0, 0));   strip.show(); vTaskDelay(pdMS_TO_TICKS(800));
        }
        else if (ai_state == 3) { // SENSOR_ERROR
            // Cảnh báo chớp Đỏ cực nhanh
            strip.setPixelColor(0, strip.Color(255, 0, 0)); strip.show(); vTaskDelay(pdMS_TO_TICKS(80));
            strip.setPixelColor(0, strip.Color(0, 0, 0));   strip.show(); vTaskDelay(pdMS_TO_TICKS(80));
        }
        else { // NHÁNH MẶC ĐỊNH: Tất cả sẽ nháy Xanh Lá với chu kỳ theo độ ẩm
            if (xQueuePeek(queue, &receivedData, 0) == pdTRUE) {
                current_humidity = receivedData.humidity;
                current_color = strip.Color(0, 255, 0);
                
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
    }
}

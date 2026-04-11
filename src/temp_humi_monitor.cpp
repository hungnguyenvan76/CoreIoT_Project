#include "temp_humi_monitor.h"
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);


void temp_humi_monitor(void *pvParameters){

    Wire.begin(11, 12);
    Serial.begin(115200);
    dht20.begin();

    while (1){
        /* code 
        
        dht20.read();
        // Reading temperature in Celsius
        float temperature = dht20.getTemperature();
        // Reading humidity
        float humidity = dht20.getHumidity();

        

        // Check if any reads failed and exit early
        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity =  -1;
            //return;
        }

        //Update global variables for temperature and humidity
        glob_temperature = temperature;
        glob_humidity = humidity;

        // Print the results
        
        Serial.print("Humidity: ");
        Serial.print(humidity);
        Serial.print("%  Temperature: ");
        Serial.print(temperature);
        Serial.println("°C");
        
        vTaskDelay(5000);
        */
        // Task2
        QueueHandle_t queue = (QueueHandle_t)pvParameters;
        while (1) {
            float new_humidity = dht20.getHumidity();
            if (isnan(new_humidity)) {
                Serial.println("Failed to read from DHT sensor!");
                new_humidity =  -1;
                //return;
            }
            xQueueOverwrite(queue, &new_humidity);
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
    
}

#include "temp_humi_monitor.h"
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);


void temp_humi_monitor(void *pvParameters){

    Wire.begin(11, 12);
    Serial.begin(115200);
    dht20.begin();
    /*
    while (1){
         code 
        
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
        
    }*/
    // Read temp & humi 
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    SensorData_t data; 

    while(1) {
        dht20.read();
        data.temperature = dht20.getTemperature();
        data.humidity = dht20.getHumidity();    
        
        // Check if any reads failed and exit early
        if (isnan(data.temperature) || isnan(data.humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            data.temperature = data.humidity =  -1;
            //return;
        }

        // Write into Queue
        xQueueOverwrite(queue, &data);
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

}

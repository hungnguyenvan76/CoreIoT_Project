#include "temp_humi_monitor.h"
DHT20 dht20;
LiquidCrystal_I2C lcd(33,16,2);

void temp_humi_monitor(void *pvParameters){
    Wire.begin(11, 12);
    Serial.begin(115200);
    dht20.begin();
    
    lcd.begin();        
    lcd.backlight();    // Turn on background 

    lcd.clear();        // Clear console
    lcd.setCursor(0, 0); 
    
    // Read temp & humi 
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    SensorData_t data; 

    while(1) {
        dht20.read();
        data.temperature = dht20.getTemperature();
        data.humidity    = dht20.getHumidity();    
        
        // Check if any reads failed
        if (isnan(data.temperature) || isnan(data.humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            data.temperature = data.humidity =  -1;
        }
        
        String sensorMsg = "\n==================================================\n";
        sensorMsg += "[SENSOR] Temp: " + String(data.temperature) + " *C | Humi: " + String(data.humidity) + " %";
        Serial.println(sensorMsg);

        // lcd
        lcd.setCursor(0, 0);
        if(data.temperature == -1) {
            lcd.print("Error!"); 
        } else {
            lcd.print(data.temperature, 1); // 25.4
            lcd.print(" *C");              
        }

        lcd.setCursor(8, 0);
        if(data.humidity == -1) {
            lcd.print("Error!");
        } else {
            lcd.print(data.humidity, 1);
            lcd.print(" %  ");
        }

        lcd.setCursor(0, 1);
        if (data.temperature > 40.0 || data.temperature < 15.0 || data.humidity > 75.0 || data.humidity < 30.0)
            lcd.print("CRITICAL!");
        else if ((data.temperature > 30 || data.temperature < 20) || (data.humidity > 60 || data.humidity < 40))
            lcd.print("WARNING!");
        else
            lcd.print("NORMAL!");

        // Write into Queue
        xQueueOverwrite(queue, &data);

        // Send data to Webserver
        String jsonData = "{\"temperature\": " + String(data.temperature) + ", \"humidity\": " + String(data.humidity) + "}";
        Webserver_sendata(jsonData);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }

}
    
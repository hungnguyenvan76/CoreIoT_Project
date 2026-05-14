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

        int ai_state = -1;
        if (aiQueue != NULL) {
            xQueuePeek(aiQueue, &ai_state, 0);
        }

        lcd.setCursor(0, 0);
        lcd.print(data.temperature, 1); // 25.4
        lcd.print(" *C");  
        lcd.setCursor(8, 0);
        lcd.print(data.humidity, 1);
        lcd.print(" %  ");
        lcd.setCursor(0, 1);

        if(ai_state == -1) {
            lcd.print("COLLECTING DATA!");
        } else if (ai_state == 0){
            lcd.print("STATE: NORMAL   ");
        } else if (ai_state == 1){
            lcd.print("STATE: FIRE_RISK");
        } else if (ai_state == 2) {
            lcd.print("STATE: MOLD RISK");
        } else if (ai_state == 3) {
            lcd.print("STATE: ERROR!   ");
        } else if (ai_state == 4) {
            lcd.print("STATE: AC ON    ");
        }

        // Write into Queue
        xQueueOverwrite(queue, &data);

        // Send data to Webserver
        String jsonData = "{\"temperature\": " + String(data.temperature) + ", \"humidity\": " + String(data.humidity) + "}";
        Webserver_sendata(jsonData);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }

}
    
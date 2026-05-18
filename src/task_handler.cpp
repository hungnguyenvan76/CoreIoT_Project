#include <task_handler.h>

// ---- Process WebSocket ----
void handleWebSocketMessage(String message)
{
    wsLedQueue = xQueueCreate(1, sizeof(WsLedConfig_t));
    wsNeoQueue = xQueueCreate(1, sizeof(WsNeoConfig_t));

    // Initialize the default initial value for the Queue (run in AUTO mode)
    WsLedConfig_t defaultLed = {false, false, 1000};
    WsNeoConfig_t defaultNeo = {false, false, 1000, 0, 0, 0};
    xQueueOverwrite(wsLedQueue, &defaultLed);
    xQueueOverwrite(wsNeoQueue, &defaultNeo);

    StaticJsonDocument<256> doc;
    if (deserializeJson(doc, message)) return;

    if (doc["page"] == "setting") {
        String WIFI_SSID = doc["value"]["ssid"].as<String>();
        String WIFI_PASS = doc["value"]["password"].as<String>();
        String CORE_IOT_TOKEN = doc["value"]["token"].as<String>();
        String CORE_IOT_SERVER = doc["value"]["server"].as<String>();
        String CORE_IOT_PORT = doc["value"]["port"].as<String>();

        Serial.println("📥 Nhận cấu hình từ WebSocket:");
        Serial.println("SSID: " + WIFI_SSID);
        Serial.println("PASS: " + WIFI_PASS);
        Serial.println("TOKEN: " + CORE_IOT_TOKEN);
        Serial.println("SERVER: " + CORE_IOT_SERVER);
        Serial.println("PORT: " + CORE_IOT_PORT);

        // Save config
        Save_info_File(WIFI_SSID, WIFI_PASS, CORE_IOT_TOKEN, CORE_IOT_SERVER, CORE_IOT_PORT);

        // Reply client (optional)
        String msg = "{\"status\":\"ok\",\"page\":\"setting_saved\"}";
        ws.textAll(msg);
    }

    if (doc["device"] == "single_led") {
        WsLedConfig_t ledCfg;
        
        if (strcmp(doc["state"], "AUTO") == 0) {
            ledCfg.isManual = false; // Return rights to AI / Sensor
        } else {
            ledCfg.isManual = true;  // Take priority
            ledCfg.isOn = (strcmp(doc["state"], "ON") == 0);
            ledCfg.delayMs = doc["delay"] | 1000;
        }
        
        if(wsLedQueue != NULL) {
            xQueueOverwrite(wsLedQueue, &ledCfg);
        }
        return;
    }

    if (doc["device"] == "neopixel") {
        WsNeoConfig_t neoCfg;

        if (strcmp(doc["state"], "AUTO") == 0) {
            neoCfg.isManual = false;
        } else {
            neoCfg.isManual = true;
            neoCfg.isOn = (strcmp(doc["state"], "ON") == 0);
            neoCfg.delayMs = doc["delay"] | 1000;
            
            const char* hex = doc["color"] | "#00ff00";
            long colorVal = strtol(hex + 1, NULL, 16);
            neoCfg.r = (colorVal >> 16) & 0xFF;
            neoCfg.g = (colorVal >>  8) & 0xFF;
            neoCfg.b = (colorVal >>  0) & 0xFF;
        }

        if(wsNeoQueue != NULL) {
            xQueueOverwrite(wsNeoQueue, &neoCfg);
        }
        return;
    }
}

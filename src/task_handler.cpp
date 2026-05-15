#include <task_handler.h>

Ticker ledTicker;
Ticker neoTicker;

bool ledPhysicalState = false;
bool neoPhysicalState = false;
Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
uint8_t neoR = 0, neoG = 255, neoB = 0;

// ---- Callback chớp LED ----
void blinkLed() {
    ledPhysicalState = !ledPhysicalState;
    pinMode(LED_GPIO, OUTPUT);
    digitalWrite(LED_GPIO, ledPhysicalState);
}

// ---- Callback chớp Neo ----
void blinkNeo() {
    neoPhysicalState = !neoPhysicalState;
    if (neoPhysicalState) {
        strip.setPixelColor(0, strip.Color(neoR, neoG, neoB));
    } else {
        strip.clear();
    }
    strip.show();
}

// ---- Xử lý WebSocket ----
void handleWebSocketMessage(String message)
{
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

        // 👉 Gọi hàm lưu cấu hình
        Save_info_File(WIFI_SSID, WIFI_PASS, CORE_IOT_TOKEN, CORE_IOT_SERVER, CORE_IOT_PORT);

        // Phản hồi lại client (tùy chọn)
        String msg = "{\"status\":\"ok\",\"page\":\"setting_saved\"}";
        ws.textAll(msg);
    }

    if (doc["device"] == "single_led") {
        bool isOn   = strcmp(doc["state"], "ON") == 0;
        float delaySec = (doc["delay"] | 1000) / 1000.0f;

        ledTicker.detach();              // dừng timer cũ
        digitalWrite(LED_GPIO, LOW);      // tắt trước

        if (isOn) {
            ledTicker.attach(delaySec, blinkLed);  // tự chớp
        }
        return;
    }

    if (doc["device"] == "neopixel") {
        bool isOn     = strcmp(doc["state"], "ON") == 0;
        float delaySec = (doc["delay"] | 1000) / 1000.0f;
        const char* hex = doc["color"] | "#00ff00";

        long colorVal = strtol(hex + 1, NULL, 16);
        neoR = (colorVal >> 16) & 0xFF;
        neoG = (colorVal >>  8) & 0xFF;
        neoB = (colorVal >>  0) & 0xFF;

        neoTicker.detach();
        strip.clear(); strip.show();   // tắt trước

        if (isOn) {
            neoTicker.attach(delaySec, blinkNeo);  // tự chớp
        }
        return;
    }
}

#include "global.h"

// Core application modules
#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
#include "tinyml.h"
#include "coreiot.h"

// System and networking tasks
#include "task_check_info.h"
#include "task_toogle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"

// System Task
void system_monitor_task(void *pvParameters) {
  while (1) {
    if (check_info_File(1)) {
      if (!Wifi_reconnect()) {
        Webserver_stop();
      } else {
        //CORE_IOT_reconnect();
      }
    }
    Webserver_reconnect();
    
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}

void setup()
{
  Serial.begin(115200);
  
  // Initialize filesystem and check stored network credentials
  check_info_File(0);

  // --- REPORT EVALUATION TASK ONLY ---
  // Uncomment the line below to run the TinyML benchmarking sequence
  // xTaskCreate(evaluate_tinyml_task, "Eval TinyML", 8192, NULL, 2, NULL);

  // --- MULTI-ENVIRONMENT ARCHITECTURE ---
  // The is_board_1 macro is defined in platformio.ini to compile 
  // different logical tasks for different physical devices.
  if(is_board_1){
    // Board 1: Acts as the AI Telemetry Node (Data Producer & AI Inference)
    if (sensorQueue && aiQueue) {
      xTaskCreate(temp_humi_monitor, "Task Sensor", 2048, (void *)sensorQueue, 2, NULL);
      xTaskCreate(neo_blinky, "Task NEO", 2048, (void *)sensorQueue, 2, NULL);
      xTaskCreate(led_blinky, "Task LED", 2048, (void *)sensorQueue, 2, NULL);
      xTaskCreate(tiny_ml_task, "Tiny ML Task", 4096, (void *)sensorQueue, 2, NULL);
      xTaskCreate(Task_CoreIOT_Publish, "CoreIOT_Pub_Task", 4096, (void *)sensorQueue, 2, NULL);
    }
  } else {
    // Board 2: Acts as the Neo Actuator Node (RPC Command Receiver & Visual Indicator)
    if (sensorQueue && aiQueue) {
      // NeoPixel LED Task version of Board 2 to control the NeoPixel LED based on AI risks received via RPC
      xTaskCreate(neo_blinky, "Task NEO", 2048, (void *)sensorQueue, 2, NULL);
    }
  }
  
  // Task to monitor the physical BOOT button for factory resetting network credentials
  xTaskCreate(Task_Toogle_BOOT, "Task_Toogle_BOOT", 4096, NULL, 2, NULL);
}

void loop()
{
  // The main loop handles network connectivity lifecycle
  if (check_info_File(1)) { // Check if valid WiFi credentials exist
    if (!Wifi_reconnect()) {
      // If WiFi connection fails, stop the AP Webserver to save power
      Webserver_stop();
    } else {
      // If WiFi is connected (STA mode), connect to CoreIoT MQTT broker
      CORE_IOT_reconnect();
    }
  } else {
    // If no credentials, handle DNS requests for the Captive Portal in AP mode
    handleDNS();
  }
  
  // Keep the local Webserver responsive for incoming WebSocket connections
  Webserver_reconnect();

  vTaskDelay(pdMS_TO_TICKS(10));
}




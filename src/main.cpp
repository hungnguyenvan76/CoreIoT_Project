#include "global.h"

#include "led_blinky.h"
#include "neo_blinky.h"
#include "temp_humi_monitor.h"
// #include "mainserver.h"
#include "tinyml.h"
#include "coreiot.h"

// include task
#include "task_check_info.h"
#include "task_toogle_boot.h"
#include "task_wifi.h"
#include "task_webserver.h"
#include "task_core_iot.h"

/*
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
    
    // avoid Watchdog Reset
    vTaskDelay(pdMS_TO_TICKS(100)); 
  }
}
*/
void setup()
{
  Serial.begin(115200);
  check_info_File(0);

  QueueHandle_t sensorQueue = xQueueCreate(1, sizeof(SensorData_t));
  if (sensorQueue) {
      xTaskCreate(temp_humi_monitor, "Task Sensor", 2048, (void *)sensorQueue, 2, NULL);
      xTaskCreate(neo_blinky, "Task NEO", 2048, (void *)sensorQueue, 2, NULL);
      xTaskCreate(led_blinky, "Task LED", 2048, (void *)sensorQueue, 2, NULL);
      xTaskCreate(tiny_ml_task, "Tiny ML Task", 8192, (void *)sensorQueue, 2, NULL);
  }

  // xTaskCreate(main_server_task, "Task Main Server" ,8192  ,NULL  ,2 , NULL);
  xTaskCreate(coreiot_task, "CoreIOT Task" ,4096  ,NULL  ,2 , NULL);
  xTaskCreate(Task_Toogle_BOOT, "Task_Toogle_BOOT", 4096, NULL, 2, NULL);
}

void loop()
{
  if (check_info_File(1))
  {
    if (!Wifi_reconnect())
    {
      Webserver_stop();
    }
    else
    {
      //CORE_IOT_reconnect();
    }
  }
  Webserver_reconnect();

  handleDNS();

  vTaskDelay(pdMS_TO_TICKS(10));
}




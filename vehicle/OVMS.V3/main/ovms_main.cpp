#include "ovms_log.h"
static const char *TAG = "ovms_main";

#include <string>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <stdio.h>
#include <string.h>
#include "ovms.h"
#include "ovms_peripherals.h"
#include "ovms_housekeeping.h"
#include "ovms_events.h"
#include "ovms_config.h"
#include "ovms_nvs.h"
#include "ovms_module.h"
#include <esp_task_wdt.h>

extern "C"
  {
  void app_main(void);
  }

static class FrameworkInit
  {
  public:
    inline FrameworkInit()
      {
      ESP_LOGI(TAG, "Set default logging level for * to %s",
        CONFIG_LOG_DEFAULT_LEVEL == 5 ? "VERBOSE" :
        CONFIG_LOG_DEFAULT_LEVEL == 4 ? "DEBUG" :
        CONFIG_LOG_DEFAULT_LEVEL == 3 ? "INFO" :
        CONFIG_LOG_DEFAULT_LEVEL == 2 ? "WARN" :
        CONFIG_LOG_DEFAULT_LEVEL == 1 ? "ERROR" : "None");
      esp_log_level_set("*",(esp_log_level_t)CONFIG_LOG_DEFAULT_LEVEL);

      ESP_LOGI(TAG, "Initialising WATCHDOG...");
      esp_task_wdt_init(120, true);
      }
  } fwi  __attribute__ ((init_priority (0150)));

Housekeeping* MyHousekeeping = NULL;
Peripherals* MyPeripherals = NULL;

void app_main(void)
  {

  // This must be the first call, it initializes the NVS subsystem which is
  // used by other components - among which the Wifi
  MyNonVolatileStorage.Init();

  ESP_LOGI(TAG, "Executing on CPU core %d",xPortGetCoreID());
  AddTaskToMap(xTaskGetCurrentTaskHandle());

  ESP_LOGI(TAG, "Handling Non-Volatile stored values...");
  MyNonVolatileStorage.Start();

  ESP_LOGI(TAG, "Mounting CONFIG...");
  MyConfig.mount();

  ESP_LOGI(TAG, "Configure logging...");
  MyCommandApp.ConfigureLogging();

  ESP_LOGI(TAG, "Registering default configs...");
  MyConfig.RegisterParam("vehicle", "Vehicle", true, true);

  ESP_LOGI(TAG, "Starting HOUSEKEEPING...");
  MyHousekeeping = new Housekeeping();
  }

#include "string.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

int get_second_sys_time(void);

long int get_msecond_sys_time(void);
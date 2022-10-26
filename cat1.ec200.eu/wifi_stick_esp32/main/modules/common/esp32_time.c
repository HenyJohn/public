#include "esp32_time.h"
#include "string.h"
#include "time.h"
#include "stdio.h"
#include <sys/time.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <stdint.h>

int get_second_sys_time(void)
{
    return (esp_timer_get_time() / (1000 * 1000));
}

long int get_msecond_sys_time(void)
{
    return (esp_timer_get_time() / 1000);
}
#define TIME_PRINT_ENABLE 1
void uptime_print(char *msg)
{
    if (TIME_PRINT_ENABLE == 1)
    {
        printf("\nuptime_ms=%ld ----------------------------------\n", get_msecond_sys_time());
        printf(msg);
    }
}

int time_legal_parse(void)
{
    struct tm currtime;
    time_t t = time(NULL);

    localtime_r(&t, &currtime);
    currtime.tm_year += 1900;
    //printf("current time is %d \n", currtime.tm_year);
    printf("save data current time  %d %d %d %d %d %d \n", currtime.tm_year, currtime.tm_mon + 1,
           currtime.tm_mday, currtime.tm_hour, currtime.tm_min, currtime.tm_sec);
    if (currtime.tm_year >= 2020)
        return 0;
    return -1;
}
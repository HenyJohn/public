#ifndef CAT1_H
#define CAT1_H

#include <unistd.h>
#include <time.h>
#include "data_process.h"

int cat1_reboot(void);
int cat1_echo_disable(void);
int get_csq(void);
int cat1_get_ccid(char *ccid);

int cat1_get_csq(void);
int cat1_get_cereg_status(void);
int cat1_get_cgreg_status(void);
int cat1_get_net_mode_value(void);

void cat1_set_data_call_ok(void);
void cat1_set_data_call_error(void);
int cat1_get_data_call_status(void); // 0, 1=OK
int check_data_call_status(void);    //-1, 0=OK
int data_call(void);
int data_call_clear(void); // fortest

void cat1_set_mqtt_ok(int index);
void cat1_set_mqtt_error(int index);
int cat1_get_mqtt_status(int index);

int cat1_mqtt_release(int index);
int cat1_mqtt_conn(int index, char *host, int port, char *client_id, char *username, char *password);
int cat1_mqtt_pub(int index, char *topic, char *payload);
int cat1_mqtt_sub(int index, char *topic);
int cat1_mqtt_recv(void);

int cat1_http_get_time(char *url, struct tm *p_tm);
int http_get_file(char *url, char *file_name, int *real_body_len);

#endif
#ifndef CAT1_H
#define CAT1_H

#include <unistd.h>
#include <time.h>

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
int cat1_get_data_call_status(void); //0, 1=OK
int check_data_call_status(void);    //-1, 0=OK
int data_call(char *apn);
int data_call_clear(void); //fortest

int cat1_http_get_time(char *url, struct tm *p_tm);
int http_get_file(char *url, char *file_name, int *real_body_len);

/** tianhe tcp*/
int cat1_tcp_state_check(void);
int cat1_get_tcp_status(void);
int cat1_tcp_conn(char *host, int port);
int cat1_tcp_send(char *buf, int len);
/** 设备发起请求后，接收天合格式的TCP响应帧。内部过滤处理天合服务端的请求，格式不符的丢弃*/
int cat1_tcp_recv(char *msg, int *size);
/** 服务端请求处理:详见tianhe.c中的各种handler函数*/
void server_request_handler(void);

/** 附加*/
void cat1_set_tcp_ok(void);
void cat1_set_tcp_error(void);

#endif
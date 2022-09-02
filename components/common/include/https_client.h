/**
 * @file https_client.h
 * @author your name (you@domain.com)
 * @brief  for kaco-lanstick 
 * @version 0.1
 * @date 2022-08-02
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef _HTTPS_CLIENT_H
#define _HTTPS_CLIENT_H
#include "Asw_global.h"
#include "protocol_examples_common.h"
#include "esp_tls.h"

void https_postdata(char *data, int *code, int *res);

#endif
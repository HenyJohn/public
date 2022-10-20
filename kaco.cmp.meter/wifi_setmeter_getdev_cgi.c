  
/** setting.cgi **/
else if (device == 3)
    {
        if (strcmp(action, "setmeter") == 0)
        {
            //char buf[100] = {0};
            //int num = 0;
            //meter_regulate_t meter = {0};
            //general_query(NVS_METER, &meter);
            meter_setdata meter = {0};
            general_query(NVS_METER_SET, (void *)&meter);

            getJsonNum(&meter.enb, "enb", value);
            printf("enb: %d\n", meter.enb);

            getJsonNum(&meter.mod, "mod", value);
            printf("mod: %d\n", meter.mod);

            getJsonNum(&meter.target, "target", value);
            printf("target: %d\n", meter.target);

            getJsonNum(&meter.reg, "regulate", value);
            printf("regulate: %d\n", meter.reg);

            getJsonNum(&meter.enb_pf, "enb_PF", value);
            printf("enb_PF: %d\n", meter.enb_pf);

            getJsonNum(&meter.target_pf, "target_PF", value);
            printf("target_PF: %d\n", meter.target_pf);

            int abs0=0;
            int offs=0; 
            getJsonNum(&abs0, "abs", value);
            printf("abs0: %d\n",     abs0);
            getJsonNum(&offs, "offset", value);
            printf("offs: %d\n",     offs);
            
            if(meter.reg == 10)
            {
                meter.reg = 0X100;
                if(abs0 == 1 && meter.target == 0)
                {
                    if(offs <= 0 || offs>100)
                        offs = 5;

                    meter.reg |= offs;
                }
            }
            meter.date = get_current_days();

            printf("cgi ------enb:%d, mod:%d, regulate:%d, target:%d, date:%d enabpf:%d tarpf:%d \n", meter.enb, meter.mod, meter.reg, meter.target, meter.date, meter.enb_pf, meter.target_pf);

            general_add(NVS_METER_SET, (void *)&meter);
            //httpd_resp_send(req, POST_RES_OK, strlen(POST_RES_OK));
            //sleep(2);
            //esp_restart();
            send_cgi_msg(60, "clearmeter", 9, NULL);
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "request not found, 404");
        }
    }


/** getdev.cgi **/

  else if (strcmp(para->device, "3") == 0)
    {
        cJSON *res = cJSON_CreateObject();
        char *msg = NULL;
        char i = 0;
        int all_pac = 0;
        int all_prc = 0;
        int reg_tmp=5;
        int abs=0;
        int offset=0;
        meter_setdata meter = {0};
        //general_query(NVS_METER, &meter);
        general_query(NVS_METER_SET, (void *)&meter);

        cJSON_AddNumberToObject(res, "mod", meter.mod);
        cJSON_AddNumberToObject(res, "enb", meter.enb);
     
        cJSON_AddNumberToObject(res, "exp_m",    meter.target);
        if(meter.reg > 0x99)
            reg_tmp=10;
        cJSON_AddNumberToObject(res, "regulate", reg_tmp);
        cJSON_AddNumberToObject(res, "enb_PF", meter.enb_pf);
        cJSON_AddNumberToObject(res, "target_PF", meter.target_pf);
        //add abs, offset
         if(meter.reg > 0x100)
        {
            abs=1;
            offset=meter.reg-0x100;
        }
        cJSON_AddNumberToObject(res, "abs",  abs);
        cJSON_AddNumberToObject(res, "abs_offset", offset);

        for (i = 0; i < inv_real_num; i++)
        {
            if (strlen(cgi_inv_arr[i].regInfo.sn) < 10)
                continue;
            all_pac += cgi_inv_arr[i].invdata.pac;
            all_prc += cgi_inv_arr[i].regInfo.rated_pwr;
        }
        cJSON_AddNumberToObject(res, "total_pac", all_pac);
        cJSON_AddNumberToObject(res, "total_fac", all_prc);
        cJSON_AddNumberToObject(res, "meter_pac", (int)inv_meter.invdata.pac);

        msg = cJSON_PrintUnformatted(res);
        httpd_resp_send(req, msg, strlen(msg));

        free(msg);
        cJSON_Delete(res);
    }

/** getdevdata.cgi **/
 else if (strcmp(para->device, "3") == 0)
    {
        cJSON *res = cJSON_CreateObject(); //Inverter inv_meter = {0};
        char *msg = NULL;

        cJSON_AddNumberToObject(res, "flg", inv_meter.invdata.status);
        cJSON_AddStringToObject(res, "tim", inv_meter.invdata.time);
        //printf("inv_meter.invdata.pac %d \n", inv_meter.invdata.pac);
        cJSON_AddNumberToObject(res, "pac", (int)inv_meter.invdata.pac);
        cJSON_AddNumberToObject(res, "itd", inv_meter.invdata.con_stu);
        cJSON_AddNumberToObject(res, "otd", inv_meter.invdata.e_today);
        cJSON_AddNumberToObject(res, "iet", inv_meter.invdata.e_total);//h_total);
        cJSON_AddNumberToObject(res, "oet", inv_meter.invdata.h_total);//e_total);
        cJSON_AddNumberToObject(res, "mod", mtconfig.mod);
        cJSON_AddNumberToObject(res, "enb", mtconfig.enb);
        // // meter_setdata mtconfigy={0};
        // // general_query(NVS_METER_SET, (void *)&mtconfigy);
        // cJSON_AddNumberToObject(res, "oet", mtconfigy.reg);
        // //printf("read ---->enb:%d, mod:%d, regulate:%d, target:%d, date:%d \n", mtconfig.enb, mtconfig.mod, mtconfig.reg, mtconfig.target, mtconfig.date);

        msg = cJSON_PrintUnformatted(res);
        httpd_resp_send(req, msg, strlen(msg));

        free(msg);
        cJSON_Delete(res);
    }

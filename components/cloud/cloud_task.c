#include "cloud_task.h"
#include "asw_protocol_ftp.h"

/*this thread is mqtt connect and upload*/

void cloud_task(void *pvParameters)
{
    TDCEvent mqtt_state = dcIdle;
    int lost_data_flag = 0;
    int get_new_invdata = 0;
    kaco_inv_data_t inv_data = {0};
    Inv_data met_data = {0}; // kaco-lanstick 20220802 +

    // int tmp_state_publish_old = -1;

    uint8_t m_mqtt_connect_enable = -1;

    asw_ftp_init(); // kaco->ftp

    cloud_setup();

    sleep(10); // sleep(180); [  debug]
    static uint8_t i = 0;

    while (1)
    {

        if (g_stick_run_mode == Work_Mode_AP_PROV || (g_stick_run_mode == Work_Mode_LAN && (get_eth_connect_status() != 0)))
        {
            m_mqtt_connect_enable = 0;
            mqtt_state = dcIdle;
            netework_state = 1; ////0-网络连接正常,1-网络链接异常
            g_state_mqtt_connect = -1;
            //  printf("\n============ FFFFFFF ======== net state:%d,%d \n",get_eth_connect_status(),netework_state);
        }
        else
        {
            m_mqtt_connect_enable = 1;
        }

        if (!update_inverter && m_mqtt_connect_enable) // v2.0.0 add   TODOs
        {
            
            switch (mqtt_state)
            {
            case dcIdle:
                mqtt_state = get_net_state();
                if (mqtt_state == dcalilyun_publish)
                    netework_state = 0;
                break;

            case dcalilyun_publish:
                mqtt_state = asw_data_publish(&get_new_invdata, inv_data, met_data, &lost_data_flag);

                break;
            default:
                break;
            }
        }
        // kaco-lanstick 20220802 +-
        recive_invdata(&get_new_invdata, &inv_data, &met_data, &lost_data_flag);

        vTaskDelay(50 / portTICK_PERIOD_MS); // usleep(50 * 1000);  [  change]
    }
}

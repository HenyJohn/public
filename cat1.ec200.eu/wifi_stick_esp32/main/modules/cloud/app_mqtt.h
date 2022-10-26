#ifndef APP_MQTT_H
#define APP_MQTT_H

typedef enum tagDCCommEvt{
    dcIdle,                //!< dcIdle
    dcalilyun_authenticate,//!< dcalilyun_authenticate
    dcalilyun_read,        //!< dcalilyun_read
    dcalilyun_publish,     //!< dcalilyun_publish
    dcalilyun_heartbeat,   //!< dcalilyun_heartbeat
    processing_data
}TDCEvent;

typedef enum SYNC_STATE
{
    CLOUD_SYNC_INIT,
    CLOUD_SYNC_MONITOR,
    CLOUD_SYNC_INVERTER,
    CLOUD_SYNC_FINISHED
}SYNC_STATE;



#endif
#include "station_include.h"

extern stationSysMsgbox_t  sensor_to_netconn_buf;
// encoding / decoding message sent from the peer / received from the peer,
// JSON data structure is applied to this project as application-level data
// message transmitting among connections.

// do request after decoding application message (received from the peer)
// These requests in this project can be :
// * adjust threshold of soil moisture (when to water your plant & keep it moist)
// * adjust threshold of air temporature (when to power cooling fan)
// * control the light of garden
// * retrieve environment condition e.g. current air temporature / humidity,
//   soil moisture, lighting condition ... etc

// collecting parameters of environment around the garden, then:
// * encode the message with these parameters to JSON-based string.
// * send encoded message out (any network protocol implemented in src/network)
static gMonStatus staEncodeOutflightAppMsg(const gmonStr_t **msg_out)
{
    const uint32_t  block_time = 0;
    gmonSensorRecord_t  *new_record = NULL;
    gMonStatus status = GMON_RESP_OK;
    uint8_t  num_records_read = 0;
    do {
        status = staSysMsgBoxGet(sensor_to_netconn_buf, (void **)&new_record, block_time);
        if(new_record != NULL) {
            // contruct JSON-data with sensor records, append to application bytes
            staAddRecordToAppMsg(new_record);
            staFreeSensorRecord(new_record);
            new_record = NULL;
            num_records_read++;
        }
    } while (status == GMON_RESP_OK);
    if(num_records_read == 0) {
        status = GMON_RESP_SKIP;
    } else {
        *msg_out = staGetAppMsgOutflight();
        status = GMON_RESP_OK;
    }
    return status;
} // end of staEncodeOutflightAppMsg


void  stationNetConnHandlerTaskFn(void* params)
{ 
    const gmonStr_t  *app_msg_send = NULL;
    const gmonStr_t  *app_msg_recv = NULL;
    gardenMonitor_t    *gmon = NULL;
    gMonStatus status = GMON_RESP_OK;

    gmon = (gardenMonitor_t *)params;
    gmon->netconn_interval_ms = GMON_CFG_NETCONN_START_INTERVAL_MS;
    while(1) {
        stationSysDelayMs(gmon->netconn_interval_ms);
        status = staEncodeOutflightAppMsg(&app_msg_send);
        if(status == GMON_RESP_SKIP) { continue; }
        // TODO: pause irrigation if pump hasn't been turned off.
        // TODO: start network connection to MQTT broker
        status = stationNetConnEstablish(gmon->netconn);
        if(status != GMON_RESP_OK) { continue; }
        // TODO: publish encoded JSON data
        // TODO: check any update from user including : threshold of each output device trigger,
        //       time interval of the working tasks, it must be JSON-based
        status = stationNetConnClose(gmon->netconn);
        // TODO: return connection status to display device ?
        // TODO: decode received JSON data (as user update)
        // TODO: resume irrigation if necessary (may recheck the updated threshold of soil moisture)
    } // end of loop
} // end of stationNetConnHandlerTaskFn


#include "station_include.h"

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

gMonStatus  staSetNetConnTaskInterval(gardenMonitor_t  *gmon, unsigned int new_interval)
{
    gMonStatus  status = GMON_RESP_OK;
    if(gmon != NULL) {
        if(new_interval >= GMON_MIN_NETCONN_START_INTERVAL_MS && new_interval <= GMON_MAX_NETCONN_START_INTERVAL_MS) {
            gmon->netconn.interval_ms = new_interval;
            staUpdateAirCondChkInterval(new_interval, (unsigned short)GMON_CFG_NUM_SENSOR_RECORDS_KEEP);
            staUpdateLightChkInterval(new_interval, (unsigned short)GMON_CFG_NUM_SENSOR_RECORDS_KEEP);
        } else {
            status = GMON_RESP_INVALID_REQ;
        }
    } else {
        status = GMON_RESP_ERRARGS;
    }
    return status;
} // end of staSetNetConnTaskInterval


void  stationNetConnHandlerTaskFn(void* params)
{
    const int   read_timeout_ms = 6000;
    gmonStr_t  *app_msg_send = NULL;
    gmonStr_t  *app_msg_recv = NULL;
    gardenMonitor_t    *gmon = NULL;
    gMonStatus  status = GMON_RESP_OK;
    uint8_t     num_reconn = 0;
    uint8_t     num_read_user_msg = 0;

    gmon = (gardenMonitor_t *)params;
    //// staSetNetConnTaskInterval(gmon, (unsigned int)GMON_CFG_NETCONN_START_INTERVAL_MS);
    app_msg_recv = staGetAppMsgInflight();
    app_msg_send = staGetAppMsgOutflight();

    while(1) {
        stationSysDelayMs(gmon->netconn.interval_ms);
        status = staRefreshAppMsgOutflight();
        if(status == GMON_RESP_SKIP) { continue; }
        // TODO: pause irrigation if pump hasn't been turned off.
        // start network connection to MQTT broker
        num_reconn = 3;
        while (num_reconn > 0) {
            status = stationNetConnEstablish(gmon->netconn.handle_obj);
            if(status == GMON_RESP_OK) {
                // publish encoded JSON data
                status  = stationNetConnSend(gmon->netconn.handle_obj, app_msg_send);
            }
            if(status == GMON_RESP_OK) {
                num_read_user_msg = 2;
                // check any update from user including : threshold of each output device trigger,
                // time interval of the working tasks, it must be JSON-based
                while(num_read_user_msg > 0) {
                    status  = stationNetConnRecv(gmon->netconn.handle_obj, app_msg_recv, read_timeout_ms);
                    num_read_user_msg = (status == GMON_RESP_OK)? 0: (num_read_user_msg - 1);
                } //  end of while loop num_read_user_msg
            }
            stationNetConnClose(gmon->netconn.handle_obj);
            num_reconn = (status == GMON_RESP_OK)? 0: (num_reconn - 1);
        } // end of while loop num_reconn
        // TODO: decode received JSON data (as user update)
        if(status == GMON_RESP_OK) {
            status = staDecodeAppMsgInflight(gmon);
        }
        // TODO: return connection status to display device, with status
        // TODO: resume irrigation if necessary (may recheck the updated threshold of soil moisture)
    } // end of loop
} // end of stationNetConnHandlerTaskFn


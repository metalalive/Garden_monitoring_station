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

gMonStatus  staSetNetConnTaskInterval(gardenMonitor_t  *gmon, unsigned int new_interval) {
    gMonStatus  status = GMON_RESP_OK;
    if(gmon != NULL) {
        if(new_interval >= GMON_MIN_NETCONN_START_INTERVAL_MS && new_interval <= GMON_MAX_NETCONN_START_INTERVAL_MS) {
            gmon->netconn.interval_ms = new_interval;
        } else {
            status = GMON_RESP_INVALID_REQ;
        }
    } else {
        status = GMON_RESP_ERRARGS;
    }
    // XASSERT(status >= 0);
    return status;
}


void  stationNetConnHandlerTaskFn(void* params)
{
    const int   read_timeout_ms = 6000;
    gMonStatus  send_status = GMON_RESP_OK, recv_status = GMON_RESP_OK;
    uint8_t     num_reconn = 0;
    gmonStr_t  *app_msg_recv = NULL, *app_msg_send = NULL;

    gardenMonitor_t    *gmon = (gardenMonitor_t *)params;
    while(1) {
        stationSysDelayMs(gmon->netconn.interval_ms);
        app_msg_recv = staGetAppMsgInflight(gmon);
        app_msg_send = staGetAppMsgOutflight(gmon);
        // pause the working output device(s) that requires to rapidly frequently refresh
        // sensor data due to the network latency.
        staPauseWorkingRealtimeOutdevs(gmon);
        // start network connection to MQTT broker
        recv_status = GMON_RESP_SKIP; // this station might not always receive update from remote user
        num_reconn = 3;
        while (num_reconn > 0) {
            send_status = stationNetConnEstablish(gmon->netconn.handle_obj);
            if(send_status == GMON_RESP_OK) {
                // publish encoded JSON data
                send_status  = stationNetConnSend(gmon->netconn.handle_obj, app_msg_send);
            }
            if(send_status == GMON_RESP_OK) {
                // check any update from user including : threshold of each output device trigger,
                // time interval of the working tasks, it must be JSON-based
                recv_status  = stationNetConnRecv(gmon->netconn.handle_obj, app_msg_recv, read_timeout_ms);
            }
            stationNetConnClose(gmon->netconn.handle_obj);
            num_reconn = (send_status == GMON_RESP_OK)? 0: (num_reconn - 1);
        } // end of while loop num_reconn
        // decode received JSON data (as user update)
        if(recv_status == GMON_RESP_OK) {
            staDecodeAppMsgInflight(gmon);
            staUpdatePrintStrThreshold(gmon); // update threshold data to display device
        }
        gmon->user_ctrl.last_update.ticks = stationGetTicksPerDay();
        gmon->user_ctrl.last_update.days  = stationGetDays();
        gmon->netconn.status.sent = send_status;
        gmon->netconn.status.recv = recv_status;
        staUpdatePrintStrNetConn(gmon);  // update network connection status to display device
    } // end of loop
} // end of stationNetConnHandlerTaskFn

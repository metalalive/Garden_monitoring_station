#include "station_include.h"

struct gMonNetStatus {
    gMonStatus send;
    gMonStatus recv;
};

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

gMonStatus staSetNetConnTaskInterval(gMonNet_t *net_handle, unsigned int new_interval) {
    gMonStatus status = GMON_RESP_OK;
    if (net_handle != NULL) {
        unsigned char valid = new_interval >= GMON_MIN_NETCONN_START_INTERVAL_MS &&
                              new_interval <= GMON_MAX_NETCONN_START_INTERVAL_MS;
        if (valid) {
            net_handle->interval_ms = new_interval;
        } else {
            status = GMON_RESP_INVALID_REQ;
        }
    } else {
        status = GMON_RESP_ERRARGS;
    }
    // XASSERT(status >= 0);
    return status;
}

static struct gMonNetStatus staNetConnIteration(
    gMonNet_t *net_handle, gmonStr_t *app_msg_recv, gmonStr_t *app_msg_send, uint8_t num_reconn
) {
    // this station might not always receive update from remote user
    gMonStatus send_status = GMON_RESP_OK, recv_status = GMON_RESP_SKIP;
    // start network connection to MQTT broker
    while (num_reconn > 0) {
        send_status = stationNetConnEstablish(net_handle);
        if (send_status == GMON_RESP_OK) {
            // publish encoded JSON data
            send_status = stationNetConnSend(net_handle, app_msg_send);
        }
        if (send_status == GMON_RESP_OK) {
            // check any update from user including : threshold of each output device trigger,
            // time interval of the working tasks, it must be JSON-based
            recv_status = stationNetConnRecv(net_handle, app_msg_recv);
        }
        stationNetConnClose(net_handle);
        num_reconn = (send_status == GMON_RESP_OK) ? 0 : (num_reconn - 1);
    }
    struct gMonNetStatus out = {.send = send_status, .recv = recv_status};
    return out;
}

void stationNetConnHandlerTaskFn(void *params) {
    gMonDisplayBlock_t *dblk = NULL;
    gardenMonitor_t    *gmon = (gardenMonitor_t *)params;
    while (1) {
        stationSysDelayMs(gmon->netconn.interval_ms);
        gmonStr_t *app_msg_recv = staGetAppMsgInflight(gmon);
        gmonStr_t *app_msg_send = staGetAppMsgOutflight(gmon);
        // pause the working output device(s) that requires to rapidly frequently refresh
        // sensor data due to the network latency.
        staPauseWorkingActuators(gmon);
        struct gMonNetStatus status = staNetConnIteration(&gmon->netconn, app_msg_recv, app_msg_send, 3);
        // decode received JSON data (as user update)
        if (status.recv == GMON_RESP_OK) {
            gMonStatus decode_status = staDecodeAppMsgInflight(gmon);
            if (decode_status == GMON_RESP_OK) {
                // update threshold to display device
                dblk = &gmon->display.blocks[GMON_BLOCK_SENSOR_THRESHOLD];
                dblk->render(&dblk->content, gmon);
            }
        }
        gmon->user_ctrl.last_update.ticks = stationGetTicksPerDay(&gmon->tick);
        gmon->user_ctrl.last_update.days = stationGetDays(&gmon->tick);
        // update network connection status to display device
        dblk = &gmon->display.blocks[GMON_BLOCK_NETCONN_STATUS];
        dblk->render(&dblk->content, gmon);
    }
}

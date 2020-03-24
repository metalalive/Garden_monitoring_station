#include "mqtt_include.h"
#include "station_include.h"


static gMonStatus  mqttRespToGMonResp(mqttRespStatus status_in)
{
    gMonStatus  status_out = GMON_RESP_OK;
    switch(status_in) {
        case MQTT_RESP_OK:
            status_out = GMON_RESP_OK     ;break;
        case MQTT_RESP_ERRARGS:
            status_out = GMON_RESP_ERRARGS;break;
        case MQTT_RESP_ERRMEM:
            status_out = GMON_RESP_ERRMEM ;break;
        case MQTT_RESP_ERR_CONN:
            status_out = GMON_RESP_ERR_CONN;break;
        case MQTT_RESP_ERR_SECURE_CONN:
            status_out = GMON_RESP_ERR_SECURE_CONN;break;
        case MQTT_RESP_MALFORMED_DATA:
            status_out = GMON_RESP_MALFORMED_DATA ;break;
        case MQTT_RESP_TIMEOUT:
            status_out = GMON_RESP_TIMEOUT;break;
        default:
            status_out = GMON_RESP_ERR    ;break;
    } // end of switch case statement
    return status_out;
} // end of mqttRespToGMonResp


gMonStatus  stationNetConnInit(void **connobj)
{
    const int cmd_timeout_ms = 6000;
    mqttCtx_t     *mctx = NULL;
    gMonStatus     status   = GMON_RESP_OK;
    mqttRespStatus mqtt_status = MQTT_RESP_OK;
    if(connobj == NULL) {
        status = GMON_RESP_ERRARGS;
    } else {
        mqtt_status = mqttClientInit(&mctx, cmd_timeout_ms);
        status = mqttRespToGMonResp(mqtt_status);
        if(mqtt_status == MQTT_RESP_OK) {
            *(mqttCtx_t **)connobj = mctx;
        }
    }
    return status;
} // end of stationNetConnInit


gMonStatus  stationNetConnDeinit(void *connobj)
{
    mqttCtx_t   *mctx = NULL;
    gMonStatus   status   = GMON_RESP_OK;
    if(connobj == NULL) {
        status = GMON_RESP_ERRARGS;
    } else {
        mctx = (mqttCtx_t *)connobj;
        if(mctx->drbg != NULL) {
            mqttDRBGdeinit(mctx->drbg);
            mctx->drbg = NULL;
        }
        status = mqttRespToGMonResp(mqttClientDeinit(mctx));
    }
    return status;
} // end of stationNetConnDeinit


gMonStatus  stationNetConnEstablish(void *connobj)
{
    if(connobj == NULL) { return GMON_RESP_ERRARGS; }
    mqttRespStatus  status = MQTT_RESP_OK;
    mqttCtx_t    *mctx = NULL;

    mctx = (mqttCtx_t *)connobj;
    if(mctx->drbg == NULL) {
        status = mqttDRBGinit(&mctx->drbg);
        if(status != MQTT_RESP_OK) { goto done; }
    }
    //// status = mqttNetconnStart( mctx );
    //// if(status != MQTT_RESP_OK) { goto done; }
    //// status = mqttSendConnect( mctx, &patt_in->connack );
done:
    return mqttRespToGMonResp(status);
} // end of stationNetConnEstablish


gMonStatus  stationNetConnClose(void *connobj)
{
    if(connobj == NULL) { return GMON_RESP_ERRARGS; }
    mqttRespStatus  status = MQTT_RESP_OK;
    mqttCtx_t    *mctx = NULL;

    mctx = (mqttCtx_t *)connobj;
    //// status = mqttSendDisconnect( mctx );
    //// status =  mqttNetconnStop( mctx );
    return mqttRespToGMonResp(status);
} // end of stationNetConnClose


gMonStatus  stationNetConnSend(void *connobj, gmonStr_t *app_msg)
{
    if(connobj == NULL || app_msg == NULL || app_msg->data == NULL) {
        return GMON_RESP_ERRARGS;
    }
    mqttRespStatus  status = MQTT_RESP_OK;
    mqttCtx_t    *mctx = NULL;

    mctx = (mqttCtx_t *)connobj;
    return mqttRespToGMonResp(status);
} // end of stationNetConnSend



static const char* gmon_test_recv_user_msg = "\
{\
\"interval\":{\"sensorread\":3450,\"netconn\":67890},\
\"threshold\":{\"soilmoist\":1234,\"airtemp\":101,\"light\":4321,\"daylength\":36005000}\
}";


gMonStatus  stationNetConnRecv(void *connobj, gmonStr_t *app_msg, int timeout_ms)
{
    if(connobj == NULL || app_msg == NULL || app_msg->data == NULL) {
        return GMON_RESP_ERRARGS;
    }
    mqttRespStatus  status = MQTT_RESP_OK;
    mqttCtx_t    *mctx = NULL;

    // TODO: only for testing purpose
    short len = XSTRLEN(gmon_test_recv_user_msg);
    XASSERT(len <= app_msg->len);
    XMEMCPY(app_msg->data, gmon_test_recv_user_msg, len);

    mctx = (mqttCtx_t *)connobj;
    return mqttRespToGMonResp(status);
} // end of stationNetConnRecv


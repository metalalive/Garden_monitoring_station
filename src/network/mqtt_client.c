#include "mqtt_include.h"
#include "station_include.h"

#define  GMON_MQTT_CMD_TIMEOUT_MS           5000
#define  GMON_MQTT_TOPIC_ALIAS_MAX          3
#define  GMON_MQTT_TOPIC_ALIAS_GARDEN_LOG   1
#define  GMON_MQTT_TOPIC_ALIAS_GARDEN_CTRL  2
#define  GMON_MQTT_CLIENT_ID       GMON_CFG_NETCONN_CLIENT_ID

// memder "unsubs" :
// * has the same struct as "subs"
// * shares the same address location with "subs" in send_pkt,
// the setup / clean function of "subs" can be reused for "unsubs"
#define  mqttSetupCmdUnsubscribe(unsubs)  mqttSetupCmdSubscribe((unsubs))
#define  mqttCleanCmdUnsubscribe(unsubs)  mqttCleanCmdSubscribe((unsubs))

static const char *gmon_mqtt_topic_str_log   = (const char *)&("garden/log");
static const char *gmon_mqtt_topic_str_ctrl  = (const char *)&("garden/ctrl");

static mqttTopic_t  gmon_mqtt_subscribe_topic;
static gmonStr_t    gmon_mqtt_client_id;
static mqttStr_t   *gmon_mqtt_broker_username;
static mqttStr_t   *gmon_mqtt_broker_password;

static gardenMonitor_t *gmon_global_refer;


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
        case MQTT_RESP_ERR_SECURE_CONN:
            status_out = GMON_RESP_ERR_SECURE_CONN;break;
        case MQTT_RESP_MALFORMED_DATA:
            status_out = GMON_RESP_MALFORMED_DATA ;break;
        case MQTT_RESP_TIMEOUT:
            status_out = GMON_RESP_TIMEOUT;break;
        case MQTT_RESP_ERR_EXCEED_PKT_SZ:
            status_out = GMON_RESP_ERR_MSG_ENCODE; break; 
        case MQTT_RESP_ERR_CTRL_PKT_TYPE:
        case MQTT_RESP_ERR_CTRL_PKT_ID  :
        case MQTT_RESP_ERR_PROP         :
        case MQTT_RESP_ERR_PROP_REPEAT  :
        case MQTT_RESP_ERR_INTEGRITY    :
        case MQTT_RESP_INVALID_TOPIC    :
            status_out = GMON_RESP_ERR_MSG_DECODE; break; 
        case MQTT_RESP_ERR_CONN:
        default:
            status_out = GMON_RESP_ERR_CONN;break;
    } // end of switch case statement
    return status_out;
} // end of mqttRespToGMonResp


static void mqttSetupCmdConnect(mqttConn_t *mconn)
{
    mqttProp_t  *curr_prop = NULL;
    // if CLEAR flag is set, and if this client have session that is previously created on
    // the MQTT server before, then the server will clean up the previous session.
    mconn->protocol_lvl = MQTT_CONN_PROTOCOL_LEVEL;
    mconn->flgs.clean_session = 0;
    mconn->keep_alive_sec  = MQTT_DEFAULT_KEEPALIVE_SEC;
    // only allow to handle subscribed inflight message one after another, not concurrently.
    curr_prop = mqttPropertyCreate(&mconn->props, MQTT_PROP_RECV_MAX);
    curr_prop->body.u16 = 1;
    curr_prop = mqttPropertyCreate(&mconn->props, MQTT_PROP_MAX_PKT_SIZE);
    curr_prop->body.u32 = MQTT_RECV_PKT_MAXBYTES - (MQTT_RECV_PKT_MAXBYTES >> 2);
    curr_prop = mqttPropertyCreate(&mconn->props, MQTT_PROP_TOPIC_ALIAS_MAX);
    curr_prop->body.u16 = GMON_MQTT_TOPIC_ALIAS_MAX;
    // no will message to send, TODO: support will message in case that a station crashed ?
    mconn->flgs.will_enable = 0;
    mconn->lwt_msg.retain = 0;
    mconn->lwt_msg.qos = MQTT_QOS_0;
    mconn->client_id.len  = gmon_mqtt_client_id.len;
    mconn->client_id.data = gmon_mqtt_client_id.data;
    mconn->username.len  = gmon_mqtt_broker_username->len;
    mconn->username.data = gmon_mqtt_broker_username->data;
    mconn->password.len  = gmon_mqtt_broker_password->len;
    mconn->password.data = gmon_mqtt_broker_password->data;
} // end of mqttSetupCmdConnect


static void  mqttSetupCmdDisconnect(mqttPktDisconn_t *disconn)
{
    disconn->reason_code = MQTT_REASON_NORMAL_DISCONNECTION;
} // end of mqttSetupCmdDisconnect


static void  mqttSetupCmdPublish(mqttMsg_t *pubmsg, gmonStr_t *payld)
{
    mqttProp_t  *curr_prop = NULL;
    XASSERT(gmon_global_refer != NULL);
    pubmsg->retain     = 1;
    pubmsg->duplicate  = 0;
    pubmsg->qos        = MQTT_QOS_1;
    curr_prop = mqttPropertyCreate(&pubmsg->props, MQTT_PROP_MSG_EXPIRY_INTVL);
    curr_prop->body.u32 = (gmon_global_refer->netconn.interval_ms * 2) / 1000; // message expiry time in seconds
    curr_prop = mqttPropertyCreate(&pubmsg->props, MQTT_PROP_TOPIC_ALIAS);
    curr_prop->body.u16 = GMON_MQTT_TOPIC_ALIAS_GARDEN_LOG;
    pubmsg->topic.len  = XSTRLEN(gmon_mqtt_topic_str_log);
    pubmsg->topic.data = gmon_mqtt_topic_str_log;
    pubmsg->app_data_len = payld->len;
    pubmsg->buff         = payld->data;
} // end of mqttSetupCmdPublish


static void  mqttSetupCmdSubscribe(mqttPktSubs_t *subs)
{
    subs->topic_cnt = 1;
    subs->topics = &gmon_mqtt_subscribe_topic;
    gmon_mqtt_subscribe_topic.reason_code = MQTT_REASON_SUCCESS;
} // end of mqttSetupCmdSubscribe


static void  mqttCleanCmdConnect(mqttConn_t *mconn)
{
    mqttPropertyDel(mconn->props);
    XMEMSET(mconn, 0x00, sizeof(mqttConn_t));
} // end of mqttCleanCmdConnect


static void  mqttCleanCmdDisconnect(mqttPktDisconn_t *disconn)
{
     mqttPropertyDel( disconn->props );
     XMEMSET(disconn, 0x00, sizeof(mqttPktDisconn_t));
} // end of mqttCleanCmdDisconnect


static void  mqttCleanCmdPublish(mqttMsg_t *pubmsg)
{
    mqttPropertyDel(pubmsg->props);
    XMEMSET(pubmsg, 0x00, sizeof(mqttMsg_t));
} // end of mqttCleanCmdPublish


static void  mqttCleanCmdSubscribe(mqttPktSubs_t *subs)
{
    mqttPropertyDel(subs->props);
    XMEMSET(subs, 0x00, sizeof(mqttPktSubs_t));
} // end of mqttCleanCmdSubscribe


gMonStatus  stationNetConnInit(gardenMonitor_t *gmon)
{
    mqttCtx_t     *mctx = NULL;
    gMonStatus     status   = GMON_RESP_OK;
    mqttRespStatus mqtt_status = MQTT_RESP_OK;
    if(gmon == NULL) {
        status = GMON_RESP_ERRARGS;
    } else {
        gmon_global_refer = gmon;
        mqtt_status = mqttClientInit(&mctx, GMON_MQTT_CMD_TIMEOUT_MS);
        status = mqttRespToGMonResp(mqtt_status);
        if(mqtt_status == MQTT_RESP_OK) {
            gmon->netconn.handle_obj  = (void *)mctx;
            // TODO: (1) what if differnt client ID in use ?
            // (2) should update username / passwd through network connection ?
            gmon_mqtt_client_id.data = (unsigned char *)&(GMON_MQTT_CLIENT_ID);
            gmon_mqtt_client_id.len  = XSTRLEN((const char *)&(GMON_MQTT_CLIENT_ID));
            mqttAuthGetBrokerLoginInfo( &gmon_mqtt_broker_username, &gmon_mqtt_broker_password );
            gmon_mqtt_subscribe_topic.qos    = MQTT_QOS_1;
            gmon_mqtt_subscribe_topic.sub_id = 1 ;
            gmon_mqtt_subscribe_topic.alias  = GMON_MQTT_TOPIC_ALIAS_GARDEN_CTRL;
            gmon_mqtt_subscribe_topic.filter.data = gmon_mqtt_topic_str_ctrl;
            gmon_mqtt_subscribe_topic.filter.len  = XSTRLEN(gmon_mqtt_topic_str_ctrl);
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
    mqttPktHeadConnack_t  *connack = NULL;
    mqttCtx_t    *mctx = NULL;
    mqttRespStatus  status = MQTT_RESP_OK;

    mctx = (mqttCtx_t *)connobj;
    if(mctx->drbg == NULL) {
        status = mqttDRBGinit(&mctx->drbg);
        if(status != MQTT_RESP_OK) { goto done; }
    }
    status = mqttNetconnStart( mctx );
    if(status != MQTT_RESP_OK) { goto done; }
    mqttSetupCmdConnect(&mctx->send_pkt.conn);
    status = mqttSendConnect( mctx, &connack );
    mqttCleanCmdConnect(&mctx->send_pkt.conn);
    if(connack != NULL) { // check what's in CONNACK
        status = mqttChkReasonCode(connack->reason_code);
        if(status != MQTT_RESP_OK){
            mctx->err_info.reason_code = connack->reason_code;
        }
    } else { status = MQTT_RESP_ERR_CONN; }
done:
    return mqttRespToGMonResp(status);
} // end of stationNetConnEstablish


gMonStatus  stationNetConnClose(void *connobj)
{
    if(connobj == NULL) { return GMON_RESP_ERRARGS; }
    mqttRespStatus  status = MQTT_RESP_OK;
    mqttCtx_t    *mctx = NULL;

    mctx = (mqttCtx_t *)connobj;
    mqttSetupCmdDisconnect(&mctx->send_pkt.disconn);
    status = mqttSendDisconnect(mctx);
    mqttCleanCmdDisconnect(&mctx->send_pkt.disconn);
    status = mqttNetconnStop(mctx);
    return mqttRespToGMonResp(status);
} // end of stationNetConnClose


gMonStatus  stationNetConnSend(void *connobj, gmonStr_t *app_msg)
{
    if(connobj == NULL || app_msg == NULL || app_msg->data == NULL || app_msg->len == 0) {
        return GMON_RESP_ERRARGS;
    }
    mqttRespStatus     status = MQTT_RESP_OK;
    mqttPktPubResp_t  *pubresp = NULL;
    mqttCtx_t         *mctx = NULL;

    mctx = (mqttCtx_t *)connobj;
    mqttSetupCmdPublish(&mctx->send_pkt.pub_msg, app_msg);
    status = mqttSendPublish(mctx, &pubresp);
    if(mctx->send_pkt.pub_msg.qos > MQTT_QOS_0) {
        if(pubresp != NULL) { // check what's in publish response structure
            status = mqttChkReasonCode(pubresp->reason_code);
            if(status != MQTT_RESP_OK){
                mctx->err_info.reason_code = pubresp->reason_code;
            }
        } else { status = MQTT_RESP_ERR_CONN; }
    }
    mqttCleanCmdPublish(&mctx->send_pkt.pub_msg);
    return mqttRespToGMonResp(status);
} // end of stationNetConnSend



gMonStatus  stationNetConnRecv(void *connobj, gmonStr_t *app_msg, int timeout_ms)
{
    if(connobj == NULL || app_msg == NULL || app_msg->data == NULL || app_msg->len == 0) {
        return GMON_RESP_ERRARGS;
    }
    mqttPktSuback_t *suback   = NULL;
    mqttPktSuback_t *unsuback = NULL;
    mqttMsg_t    *pubmsg_recv = NULL;
    mqttCtx_t    *mctx   = NULL;
    word32        cpy_sz = 0;
    mqttRespStatus  status = MQTT_RESP_OK;
    mctx = (mqttCtx_t *)connobj;
    mqttSetupCmdSubscribe(&mctx->send_pkt.subs);
    status = mqttSendSubscribe(mctx, &suback);
    mqttCleanCmdSubscribe(&mctx->send_pkt.subs);
    if(status < 0) { goto done; }
    if(suback == NULL) {
        status = MQTT_RESP_ERR_CONN; goto done;
    }
    status = mqttChkReasonCode(suback->return_codes[0]); // only subscribe one topic
    if( status != MQTT_RESP_OK ){
        mctx->err_info.reason_code = suback->return_codes[0];
        goto done;
    }
    // wait for inflight PUBLISH message
    mqttModifyReadMsgTimeout(mctx, timeout_ms);
    status = mqttClientWaitPkt(mctx, MQTT_PACKET_TYPE_PUBLISH, 0, (void **)&pubmsg_recv );
    if(status == MQTT_RESP_OK && pubmsg_recv != NULL) {
        cpy_sz = XMIN(pubmsg_recv->app_data_len, app_msg->len);
        XMEMCPY(app_msg->data, pubmsg_recv->buff, cpy_sz); // copy payload
    } else {
        mctx->err_info.reason_code = MQTT_REASON_UNSPECIFIED_ERR;
    }
    mqttModifyReadMsgTimeout(mctx, GMON_MQTT_CMD_TIMEOUT_MS);
done:  // unsubscribe topic
    mqttSetupCmdUnsubscribe(&mctx->send_pkt.unsubs);
    mqttSendUnsubscribe(mctx, &unsuback);
    mqttCleanCmdUnsubscribe(&mctx->send_pkt.unsubs);
    if(unsuback != NULL) {
        if(mqttChkReasonCode(unsuback->return_codes[0]) != MQTT_RESP_OK) {
            mctx->err_info.reason_code = unsuback->return_codes[0];
        }
    }
    return mqttRespToGMonResp(status);
} // end of stationNetConnRecv


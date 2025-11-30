#include "mqtt_include.h"
#include "station_include.h"

#define  GMON_MQTT_CMD_TIMEOUT_MS           5000
#define  GMON_MQTT_TOPIC_ALIAS_MAX          3
#define  GMON_MQTT_TOPIC_ALIAS_GARDEN_LOG   1
#define  GMON_MQTT_TOPIC_ALIAS_GARDEN_CTRL  2
#define  GMON_MQTT_CLIENT_ID       GMON_CFG_NETCONN_CLIENT_ID
#define  GMON_MQTT_TOPIC_LOG       GMON_CFG_MQTT_TOPIC_LOG
#define  GMON_MQTT_TOPIC_USR_CTRL  GMON_CFG_MQTT_TOPIC_USR_CTRL

#define  mqttSetupCmdUnsubscribe(unsubs, ext_ctx)  mqttSetupCmdSubscribe((unsubs), (ext_ctx))
#define  mqttCleanCmdUnsubscribe(unsubs)  mqttCleanCmdSubscribe((unsubs))

typedef struct {
    mqttCtx_t    *mctx;
    mqttTopic_t   subscribe_topic;
    mqttStr_t     client_id;
    mqttStr_t    *broker_username;
    mqttStr_t    *broker_password;
} mqttExtendCtx_t;

static gMonStatus  mqttRespToGMonResp(mqttRespStatus status_in) {
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


static void mqttSetupCmdConnect(mqttConn_t *mconn, mqttExtendCtx_t *ext_ctx) {
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
    mconn->client_id = ext_ctx->client_id;
    mconn->username = *ext_ctx->broker_username;
    mconn->password = *ext_ctx->broker_password;
}

static void  mqttSetupCmdDisconnect(mqttPktDisconn_t *disconn) {
    disconn->reason_code = MQTT_REASON_NORMAL_DISCONNECTION;
}

static void  mqttSetupCmdPublish(mqttMsg_t *pubmsg, gmonStr_t *payld, unsigned int exp_interval_ms) {
    mqttProp_t  *curr_prop = NULL;
    pubmsg->retain     = 1;
    pubmsg->duplicate  = 0;
    pubmsg->qos        = MQTT_QOS_1;
    curr_prop = mqttPropertyCreate(&pubmsg->props, MQTT_PROP_MSG_EXPIRY_INTVL);
    curr_prop->body.u32 = (exp_interval_ms << 1) / 1000; // message expiry time in seconds
    curr_prop = mqttPropertyCreate(&pubmsg->props, MQTT_PROP_TOPIC_ALIAS);
    curr_prop->body.u16 = GMON_MQTT_TOPIC_ALIAS_GARDEN_LOG;
    pubmsg->topic.len  = sizeof(GMON_MQTT_TOPIC_LOG) - 1;
    pubmsg->topic.data = (byte *) GMON_MQTT_TOPIC_LOG;
    pubmsg->app_data_len = payld->len;
    pubmsg->buff         = payld->data;
}


static void  mqttSetupCmdSubscribe(mqttPktSubs_t *subs, mqttExtendCtx_t *ext_ctx) {
    subs->topic_cnt = 1;
    subs->topics = &ext_ctx->subscribe_topic;
    ext_ctx->subscribe_topic.reason_code = MQTT_REASON_SUCCESS;
}

static void  mqttCleanCmdConnect(mqttConn_t *mconn) {
    mqttPropertyDel(mconn->props);
    XMEMSET(mconn, 0x00, sizeof(mqttConn_t));
}

static void  mqttCleanCmdDisconnect(mqttPktDisconn_t *disconn) {
     mqttPropertyDel( disconn->props );
     XMEMSET(disconn, 0x00, sizeof(mqttPktDisconn_t));
}

static void  mqttCleanCmdPublish(mqttMsg_t *pubmsg) {
    mqttPropertyDel(pubmsg->props);
    XMEMSET(pubmsg, 0x00, sizeof(mqttMsg_t));
}

static void  mqttCleanCmdSubscribe(mqttPktSubs_t *subs) {
    mqttPropertyDel(subs->props);
    XMEMSET(subs, 0x00, sizeof(mqttPktSubs_t));
}


gMonStatus  stationNetConnInit(gMonNet_t *net_handle) {
    if(net_handle == NULL) {
        return GMON_RESP_ERRARGS;
    }
    mqttExtendCtx_t  *ext_ctx = NULL;
    gMonStatus status = staSetNetConnTaskInterval(net_handle, (unsigned int)GMON_CFG_NETCONN_START_INTERVAL_MS);
    if(status < 0) { return status; }

    ext_ctx = (mqttExtendCtx_t *)XMALLOC(sizeof(mqttExtendCtx_t));
    if (ext_ctx == NULL) { return GMON_RESP_ERRMEM; }
    XMEMSET(ext_ctx, 0x00, sizeof(mqttExtendCtx_t));

    mqttRespStatus mqtt_status = mqttClientInit(&ext_ctx->mctx, GMON_MQTT_CMD_TIMEOUT_MS);
    status = mqttRespToGMonResp(mqtt_status);
    if(mqtt_status == MQTT_RESP_OK) {
        net_handle->lowlvl = (void *)ext_ctx;
        net_handle->read_timeout_ms = 6000; // TODO, make it configurable ?
        // TODO: (1) what if differnt client ID in use ?
        // (2) should update username / passwd through network connection ?
        ext_ctx->client_id.data = (byte *) GMON_MQTT_CLIENT_ID;
        ext_ctx->client_id.len  = sizeof(GMON_MQTT_CLIENT_ID) - 1;
        mqttAuthGetBrokerLoginInfo( &ext_ctx->broker_username, &ext_ctx->broker_password );
        ext_ctx->subscribe_topic.qos    = MQTT_QOS_1;
        ext_ctx->subscribe_topic.sub_id = 1 ;
        ext_ctx->subscribe_topic.alias  = GMON_MQTT_TOPIC_ALIAS_GARDEN_CTRL;
        ext_ctx->subscribe_topic.filter.data = (byte *) GMON_MQTT_TOPIC_USR_CTRL;
        ext_ctx->subscribe_topic.filter.len  = sizeof(GMON_MQTT_TOPIC_USR_CTRL) - 1;
    }
    return status;
} // end of stationNetConnInit


gMonStatus  stationNetConnDeinit(gMonNet_t *net_handle) {
    gMonStatus   status   = GMON_RESP_OK;
    if(net_handle == NULL) {
        status = GMON_RESP_ERRARGS;
    } else {
        mqttExtendCtx_t  *ext_ctx = (mqttExtendCtx_t *)net_handle->lowlvl;
        if (ext_ctx != NULL) {
            mqttCtx_t   *mctx = ext_ctx->mctx;
            if(mctx->drbg != NULL) {
                mqttDRBGdeinit(mctx->drbg);
                mctx->drbg = NULL;
            }
            status = mqttRespToGMonResp(mqttClientDeinit(mctx));
            XFREE(ext_ctx); // Free the allocated mqttExtendCtx_t
        }
        net_handle->lowlvl = NULL;
    }
    return status;
}

gMonStatus  stationNetConnEstablish(gMonNet_t *net_handle) {
    if(net_handle == NULL) { return GMON_RESP_ERRARGS; }
    mqttPktHeadConnack_t  *connack = NULL;
    mqttRespStatus  status = MQTT_RESP_OK;
    mqttExtendCtx_t  *ext_ctx = (mqttExtendCtx_t *)net_handle->lowlvl;
    mqttCtx_t    *mctx = ext_ctx->mctx;
    if(mctx->drbg == NULL) {
        status = mqttDRBGinit(&mctx->drbg);
        if(status != MQTT_RESP_OK) { goto done; }
    }
    status = mqttSysNetInit();
    if(status != MQTT_RESP_OK) { goto done; }
    status = mqttNetconnStart( mctx );
    if(status != MQTT_RESP_OK) { goto done; }
    mqttSetupCmdConnect(&mctx->send_pkt.conn, ext_ctx);
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
}

gMonStatus  stationNetConnClose(gMonNet_t *net_handle) {
    if(net_handle == NULL) { return GMON_RESP_ERRARGS; }
    mqttExtendCtx_t  *ext_ctx = (mqttExtendCtx_t *)net_handle->lowlvl;
    mqttCtx_t    *mctx = ext_ctx->mctx;
    mqttSetupCmdDisconnect(&mctx->send_pkt.disconn);
    mqttRespStatus  status = mqttSendDisconnect(mctx);
    mqttCleanCmdDisconnect(&mctx->send_pkt.disconn);
    status = mqttNetconnStop(mctx);
    mqttSysNetDeInit();
    return mqttRespToGMonResp(status);
}

gMonStatus  stationNetConnSend(gMonNet_t *net_handle, gmonStr_t *app_msg) {
    if(net_handle == NULL || app_msg == NULL || app_msg->data == NULL || app_msg->len == 0) {
        return GMON_RESP_ERRARGS;
    }
    mqttPktPubResp_t  *pubresp = NULL;
    mqttExtendCtx_t   *ext_ctx = (mqttExtendCtx_t *)net_handle->lowlvl;
    mqttCtx_t         *mctx = ext_ctx->mctx;
    mqttSetupCmdPublish(&mctx->send_pkt.pub_msg, app_msg, net_handle->interval_ms);
    mqttRespStatus     status = mqttSendPublish(mctx, &pubresp);
    if(mctx->send_pkt.pub_msg.qos > MQTT_QOS_0) {
        if(pubresp != NULL) { // check what's in publish response structure
            status = mqttChkReasonCode(pubresp->reason_code);
            if(status != MQTT_RESP_OK){
                mctx->err_info.reason_code = pubresp->reason_code;
            }
        } else { status = MQTT_RESP_ERR_CONN; }
    }
    mqttCleanCmdPublish(&mctx->send_pkt.pub_msg);
    net_handle->status.sent = mqttRespToGMonResp(status);
    return net_handle->status.sent;
}

gMonStatus  stationNetConnRecv(gMonNet_t *net_handle, gmonStr_t *app_msg) {
    if(net_handle == NULL || app_msg == NULL || app_msg->data == NULL || app_msg->len == 0) {
        return GMON_RESP_ERRARGS;
    }
    mqttPktSuback_t  *suback   = NULL, *unsuback = NULL;
    mqttMsg_t        *pubmsg_recv = NULL;
    mqttExtendCtx_t  *ext_ctx = (mqttExtendCtx_t *)net_handle->lowlvl;
    mqttCtx_t        *mctx   = ext_ctx->mctx;
    word32        cpy_sz = 0;
    mqttRespStatus  status = MQTT_RESP_OK;
    mqttSetupCmdSubscribe(&mctx->send_pkt.subs, ext_ctx);
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
    mqttModifyReadMsgTimeout(mctx, net_handle->read_timeout_ms);
    status = mqttClientWaitPkt(mctx, MQTT_PACKET_TYPE_PUBLISH, 0, (void **)&pubmsg_recv );
    if(status == MQTT_RESP_OK && pubmsg_recv != NULL) {
        cpy_sz = XMIN(pubmsg_recv->app_data_len, app_msg->len);
        XMEMCPY(app_msg->data, pubmsg_recv->buff, cpy_sz); // copy payload
    } else {
        mctx->err_info.reason_code = MQTT_REASON_UNSPECIFIED_ERR;
    }
    mqttModifyReadMsgTimeout(mctx, GMON_MQTT_CMD_TIMEOUT_MS);
done:  // unsubscribe topic
    mqttSetupCmdUnsubscribe(&mctx->send_pkt.unsubs, ext_ctx);
    mqttSendUnsubscribe(mctx, &unsuback);
    mqttCleanCmdUnsubscribe(&mctx->send_pkt.unsubs);
    if(unsuback != NULL) {
        if(mqttChkReasonCode(unsuback->return_codes[0]) != MQTT_RESP_OK) {
            mctx->err_info.reason_code = unsuback->return_codes[0];
        }
    }
    net_handle->status.recv = mqttRespToGMonResp(status);
    return net_handle->status.recv;
} // end of stationNetConnRecv


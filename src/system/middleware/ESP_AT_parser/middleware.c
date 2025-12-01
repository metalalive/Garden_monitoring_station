#include "station_include.h"

gMonStatus staSysCvtResp(int resp_in) {
    gMonStatus resp_out = GMON_RESP_OK;
    switch ((espRes_t)resp_in) {
    case espOK:
        resp_out = GMON_RESP_OK;
        break;
    case espERRARGS:
        resp_out = GMON_RESP_ERRARGS;
        break;
    case espERRMEM:
        resp_out = GMON_RESP_ERRMEM;
        break;
    case espTIMEOUT:
        resp_out = GMON_RESP_TIMEOUT;
        break;
    default:
        resp_out = GMON_RESP_ERR;
        break;
    }
    return resp_out;
} // end of staSysCvtResp

gMonStatus stationSysCreateTask(
    const char *task_name, stationSysTaskFn_t task_fp, void *const args, size_t stack_sz, uint32_t prio,
    uint8_t isPrivileged, stationSysTask_t *task_ptr
) {
    espRes_t response;
    response = eESPsysThreadCreate(
        (espSysThread_t *)task_ptr, task_name, task_fp, args, stack_sz, (espSysThreadPrio_t)prio, isPrivileged
    );
    ESP_ASSERT(response == espOK);
    // in this system port, if thread scheduler hasn't been running,
    // then we launch thread scheduler at here immediately after the first thread is created
    if (eESPsysGetTskSchedulerState() == ESP_SYS_TASK_SCHEDULER_NOT_STARTED) {
        eESPsysTskSchedulerStart();
    }
    // in FreeRTOS port, CPU might never arrive in here after thread scheduler  is running
    return (response == espOK ? GMON_RESP_OK : GMON_RESP_ERR);
} // end of stationSysCreateTask

gMonStatus stationSysTaskDelete(stationSysTask_t *task_ptr) {
    espRes_t response;
    response = eESPsysThreadDelete((espSysThread_t *)task_ptr);
    return (response == espOK ? GMON_RESP_OK : GMON_RESP_ERR);
} // end of stationSysTaskDelete

gMonStatus stationSysInit(void) { return stationPlatformInit(); }

gMonStatus stationSysDelayUs(unsigned short time_us) {
    return staPlatformDelayUs(time_us);
} // end of stationSysDelayUs

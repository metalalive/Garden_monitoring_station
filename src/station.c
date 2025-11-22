#include "station_include.h"

static gardenMonitor_t  *garden_monitor;

static gMonStatus stationInit(gardenMonitor_t **gmon)
{
    if(gmon == NULL) { return GMON_RESP_ERRARGS; }
    gMonStatus status = GMON_RESP_OK;
    *gmon = XCALLOC(sizeof(gardenMonitor_t), 0x1);
    status = staSetNetConnTaskInterval(*gmon, (unsigned int)GMON_CFG_NETCONN_START_INTERVAL_MS);
    if(status < 0) { goto done; }
    status = stationNetConnInit(*gmon);
    if(status < 0) { goto done; }
    status = stationSysInit();
    if(status < 0) { goto done; }
    status = staAppMsgInit(*gmon);
    if(status < 0) { goto done; }
    status = staDaylightTrackInit(*gmon);
    if(status < 0) { goto done; }
    status = staAirCondTrackInit();
    if(status < 0) { goto done; }
    status = stationIOinit(*gmon);
    if(status < 0) { goto done; }
    status = staDisplayInit(*gmon);
done:
    return status;
} // end of stationInit


static gMonStatus stationDeinit(gardenMonitor_t *gmon)
{
    if(gmon == NULL) { return GMON_RESP_ERRARGS; }
    gMonStatus status = GMON_RESP_OK;
    status = staDisplayDeInit(gmon);
    status = stationIOdeinit(gmon);
    status = stationNetConnDeinit(gmon->netconn.handle_obj);
    status = stationPlatformDeinit();
    status = staAppMsgDeinit(gmon);
    gmon->netconn.handle_obj = NULL;
    XMEMFREE(gmon);
    return status;
} // end of stationDeinit


// create the threads that :
// * read from sensor periodically
// * control device of the target garden,
// * display status to LCD/OLED device (if exists)
// * network connection, encode / decode application message
static void  stationInitTaskFn(void *param)
{
    gardenMonitor_t  *gmon = NULL;
    stationSysTask_t  task_ptr = NULL;
    unsigned  short   task_stack_size = 0;
    const unsigned char isPrivileged = 0x1;

    task_ptr = NULL;
    task_stack_size = 0x80; // Use same stack size as sensorReader for consistency
    stationSysCreateTask("pumpCtrler", (stationSysTaskFn_t)pumpControllerTaskFn,
                         (void *)gmon, task_stack_size, GMON_TASKS_PRIO_MIN, isPrivileged, &task_ptr);
    gmon->tasks.pump_controller = (void *)task_ptr;

    task_ptr = NULL;
    task_stack_size = 0x80;
    stationSysCreateTask("airMonitor", (stationSysTaskFn_t)airQualityMonitorTaskFn,
                         (void *)gmon, task_stack_size, GMON_TASKS_PRIO_MIN, isPrivileged, &task_ptr);
    gmon->tasks.air_quality_monitor = (void *)task_ptr;
    
    task_ptr = NULL;
    task_stack_size = 0x80;
    stationSysCreateTask("lightCtrler", (stationSysTaskFn_t)lightControllerTaskFn,
                         (void *)gmon, task_stack_size, GMON_TASKS_PRIO_MIN, isPrivileged, &task_ptr);
    gmon->tasks.light_controller = (void *)task_ptr;

    task_ptr = NULL;
    task_stack_size = 0x38;
    stationSysCreateTask("DataAggregator", (stationSysTaskFn_t)stationSensorDataAggregatorTaskFn,
                         (void *)gmon, task_stack_size, GMON_TASKS_PRIO_MIN, isPrivileged, &task_ptr);
    gmon->tasks.sensor_data_aggregator_net = (void *)task_ptr;

    task_ptr = NULL;
    task_stack_size = 0x158;
    stationSysCreateTask("netConnHandler", (stationSysTaskFn_t)stationNetConnHandlerTaskFn,
                      (void *)gmon, task_stack_size, (GMON_TASKS_PRIO_MIN + 1), isPrivileged, &task_ptr);
    gmon->tasks.netconn_handler = (void *)task_ptr;
#ifdef  GMON_CFG_ENABLE_DISPLAY
    task_ptr = NULL;
    task_stack_size = 0x80;
    stationSysCreateTask("DisplayHandler", (stationSysTaskFn_t)stationDisplayTaskFn,
                         (void *)gmon, task_stack_size, GMON_TASKS_PRIO_MIN, isPrivileged, &task_ptr);
    gmon->tasks.display_handler = (void *)task_ptr;
#endif // end of GMON_CFG_ENABLE_DISPLAY

    stationSysTaskDelete(NULL);
} // end of stationInitTaskFn


static void stationCreateWorkingTasks(gardenMonitor_t *gmon)
{
    stationSysTask_t  init_task = NULL;
    const unsigned short task_stack_size = 0x60;
    const unsigned char  isPrivileged = 0x1;

    stationSysCreateTask("monStaInitTask", (stationSysTaskFn_t)stationInitTaskFn,
                         (void *)gmon, task_stack_size, (GMON_TASKS_PRIO_MIN + 2),
                         isPrivileged, &init_task );

    stationSysTaskWaitUntilExit(&init_task, NULL);
} // end of stationCreateWorkingTasks


int main (int argc, char** argv)
{
    gMonStatus status = GMON_RESP_OK;
    garden_monitor = NULL;
    status =  stationInit(&garden_monitor);
    if(status == GMON_RESP_OK) {
        XASSERT(garden_monitor != NULL);
        stationCreateWorkingTasks(garden_monitor);
    } else { // terminate immediately on initialization failure
        stationDeinit(garden_monitor);
    }
    XASSERT(0);
    return 0;
} // end of main


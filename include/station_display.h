#ifndef STATION_DISPLAY_H
#define STATION_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    unsigned short        width;
    unsigned short        height;
    const unsigned short *bitmap;
} gmonPrintFont_t;

typedef struct {
    gmonStr_t        str;
    gmonPrintFont_t *font;
    short            posx;
    short            posy;
} gmonPrintInfo_t;

typedef gMonStatus (*gMonRenderFn_t)(gmonPrintInfo_t *content, void *app_ctx);

typedef enum {
    GMON_BLOCK_SENSOR_RECORD = 0,
    GMON_BLOCK_SENSOR_THRESHOLD,
    GMON_BLOCK_ACTUATOR_STATUS,
    GMON_BLOCK_NETCONN_STATUS,
} gMonBlockType_t;

typedef struct {
    // it is necessary if the same information block has been split to several
    // blocks for display purpose.
    gMonBlockType_t btype;
    gmonPrintInfo_t content;
    gMonRenderFn_t  render;
} gMonDisplayBlock_t;

typedef struct {
    gMonDisplayBlock_t blocks[GMON_DISPLAY_NUM_PRINT_STRINGS];
    unsigned short     num_blocks;
    gmonPrintFont_t    fonts[1]; // TODO, Support multiple fonts
    struct {
        unsigned int scroll_speed;
        unsigned int refresh_rate_ms;
    } config;
} gMonDisplayContext_t;

// ----------------------------
gMonStatus     staDisplayDevInit(void);
gMonStatus     staDisplayDevDeInit(void);
gMonStatus     staDisplayRefreshScreen(void);
unsigned short staDisplayDevGetScreenWidth(void);
unsigned short staDisplayDevGetScreenHeight(void);
gMonStatus     staDiplayDevPrintString(gmonPrintInfo_t *);

gMonStatus staDisplayInit(struct gardenMonitor_s *);
gMonStatus staDisplayDeInit(struct gardenMonitor_s *);

void stationDisplayTaskFn(void *params);

#ifdef __cplusplus
}
#endif
#endif // end of STATION_DISPLAY_H

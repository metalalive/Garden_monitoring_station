#include "station_include.h"

// For reset & initialization command sequence, please refer to SSD1306 OLED specification
// , Chapter 10 : command description
// TODO: refactor
#define  GMON_CFG_OLED_SSD1315_SCREEN_WIDTH   128
#define  GMON_CFG_OLED_SSD1315_SCREEN_HEIGHT  64
#define  OLED_SSD1315_SCREEN_FRAMEBUF_NBYTES  ((GMON_CFG_OLED_SSD1315_SCREEN_WIDTH * GMON_CFG_OLED_SSD1315_SCREEN_HEIGHT) >> 3)

typedef struct {
    uint16_t  screen_width;
    uint16_t  screen_height;
    short     curr_x;
    short     curr_y;
    uint8_t   inverted:1;
    uint8_t   initialized:1;
} oled_t;

static unsigned char gmon_ssd1315_framebuf[OLED_SSD1315_SCREEN_FRAMEBUF_NBYTES];
static void   *ssd1315_display_pin_spi;
static void   *ssd1315_display_pin_rst;
static void   *ssd1315_display_pin_dc;
static oled_t  oled_dev;


static gMonStatus staDisplaySetGPIOpin(void *pinstruct, uint8_t pin_state)
{
    gMonStatus status = GMON_RESP_SKIP;
    if(pinstruct) {
        status = staPlatformWritePin(pinstruct, pin_state);
    }
    return status;
} // end of staDisplaySetGPIOpin


static gMonStatus  staOLEDsendCmd(unsigned char cmdbyte)
{
    unsigned short datasize = 0x1;
    gMonStatus status = GMON_RESP_SKIP;
    status = staDisplaySetGPIOpin(ssd1315_display_pin_dc, GMON_PLATFORM_PIN_RESET);
    if(status < 0) { goto done; }
    status = staPlatformSPItransmit(ssd1315_display_pin_spi, &cmdbyte, datasize);
done:
    return status;
} // end of staOLEDsendCmd


static gMonStatus staOLEDsendData (unsigned char *pdata, unsigned short datasize)
{
    gMonStatus status = GMON_RESP_SKIP;
    status = staDisplaySetGPIOpin(ssd1315_display_pin_dc, GMON_PLATFORM_PIN_SET);
    if(status < 0) { goto done; }
    status = staPlatformSPItransmit(ssd1315_display_pin_spi, pdata, datasize);
done:
    return status;
} // end of staOLEDsendData


static  void  staOLEDsetCursor(short  x, short y)
{
    oled_dev.curr_x = x;
    oled_dev.curr_y = y;
} // end of staOLEDsetCursor


static  void  staDisplayDevFill(uint8_t pixel_on)
{
    uint8_t  setvalue = 0;
    setvalue = (pixel_on == 0) ? 0x00: 0xFF;
    XMEMSET(&gmon_ssd1315_framebuf[0], setvalue, sizeof(char) * OLED_SSD1315_SCREEN_FRAMEBUF_NBYTES);
} // end of staDisplayDevFill


gMonStatus staDisplayRefreshScreen(void)
{
    gMonStatus status = GMON_RESP_SKIP;
    uint8_t idx = 0;
    stationSysEnterCritical();
    for (idx = 0; idx < 8; idx++) {
        status  = staOLEDsendCmd(0xB0 + idx); // this command potiions "page start address" from PAGE0 ~ PAGE7 in GDDRAM
        status |= staOLEDsendCmd(0x00);
        status |= staOLEDsendCmd(0x10);
        if(status < 0) { break; }
        staOLEDsendData(&gmon_ssd1315_framebuf[(GMON_CFG_OLED_SSD1315_SCREEN_WIDTH * idx)] , (size_t)GMON_CFG_OLED_SSD1315_SCREEN_WIDTH);
        if(status < 0) { break; }
    }
    stationSysExitCritical();
    return status;
} // end of staDisplayRefreshScreen




static gMonStatus staOLEDcmdInit(void)
{
    uint8_t idx = 0; gMonStatus status = GMON_RESP_OK;
    status  = staDisplaySetGPIOpin(ssd1315_display_pin_rst, GMON_PLATFORM_PIN_RESET);
    if(status < 0) { goto done; }
    stationSysDelayUs(10000);  // wait for at least 3 us after reset assertion
    status  = staDisplaySetGPIOpin(ssd1315_display_pin_rst, GMON_PLATFORM_PIN_SET);
    if(status < 0) { goto done; }
    for(idx=0; idx<10; idx++) { stationSysDelayUs(20000); } // wait for 1 ms after reset de-assertion
    status = staOLEDsendCmd(0xAE); // display OFF, go to sleep mode to modify configuration
    if(status < 0) { goto done; }
    // set memory addressing mode, LSB 2 bit is for specifying address mode ...
    // 2'b00 : Horizontal Addressing Mode, 2'b01 : Vertical Addressing Mode, 2'b10 : Page Addressing Mode
    status = staOLEDsendCmd(0x20); // <-- Horizontal Addressing Mode is selected at here
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0x10); // set low column address, TODO: recheck if it's necessary to set it at here.
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0xB0); // set Page Start Address (in this case, start address is set to PAGE 0)
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0xC8); // set COM output scan direction, TODO: figure out difference from 0xC0
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0x00); // set low column address (in this case, starting column is set to COL0)
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0x10); // set high column address TODO: figure out its usage and necessary.
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0x40); // set Display Start Line, LSB 6 bits are determined which RAM row is mapped to COM0
                                   // , which is in PAGE0 (in this case, (GDD)RAM row 0 is mapped to COM0)
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0x81); // set contrast control for BANK0
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0xFF); // dummy byte, TODO: figure out the usage
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0xA1); // set segment re-map (column address 127, COL127 is mapped to SEG0)
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0xA6); // set normal display, bit 1 of RAM data means "pixel ON" (otherwise bit 0 means pixel ON)
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0xA8); // set multiplex ratio
    if(status < 0) { goto done; }
#if    (GMON_CFG_OLED_SSD1315_SCREEN_HEIGHT == 32)
    status = staOLEDsendCmd(0x1F); // set high column address to row 31
#elif  (GMON_CFG_OLED_SSD1315_SCREEN_HEIGHT == 64)
    status = staOLEDsendCmd(0x3F); // set high column address to row 63
#else
    #error "ONLY 32 or 64 rows of height is supported. Recheck your configuration."
#endif // end of GMON_CFG_OLED_SSD1315_SCREEN_HEIGHT
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0xA4); // display outputs according to (GDD)RAM content.
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0xD3); // double-byte command: set display offset to COM0 (mapped to RAM row 0)
    status = staOLEDsendCmd(0x00);
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0xD5); // double-byte command: set display clock divide ratio / Oscillator Frequency to internal DCLK
    status = staOLEDsendCmd(0xF0); // in this case, do not divide DCLK freq. (LSB 4 bits = 0x0), maximize Oscillator Frequency (MSB 4 bits = 0xf)
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0xD9); // set pre-charged period
    status = staOLEDsendCmd(0x22); // phase 1 period : 2 DCLK, phase 2 period : 2 DCLK, TODO: figure out the usage
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0xDA); // set COM pins hardware configuration, see Table 10-3 of SSD1306 spec for detail
#if    (GMON_CFG_OLED_SSD1315_SCREEN_HEIGHT == 32)
    status = staOLEDsendCmd(0x02);
#elif  (GMON_CFG_OLED_SSD1315_SCREEN_HEIGHT == 64)
    status = staOLEDsendCmd(0x12); // bit 5 = 0, disable COM left/right remap, bit 4 = 1
#else
    #error "ONLY 32 or 64 rows of height is supported. Recheck your configuration."
#endif // end of GMON_CFG_OLED_SSD1315_SCREEN_HEIGHT
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0xDB); // set V_comh deselect level (DBh)
    status = staOLEDsendCmd(0x20);
    if(status < 0) { goto done; }
    status = staOLEDsendCmd(0x8D);
    status = staOLEDsendCmd(0x14);
    status = staOLEDsendCmd(0xAF); // display ON, go to normal mode
    if(status < 0) { goto done; }
    staDisplayDevFill(0x0);
    status = staDisplayRefreshScreen();
done:
    return status;
} // end of staOLEDcmdInit


gMonStatus  staDisplayDevInit(void)
{
    gMonStatus status = GMON_RESP_OK;
    ssd1315_display_pin_spi = NULL;
    ssd1315_display_pin_rst = NULL;
    ssd1315_display_pin_dc  = NULL;
    status = staOutDevPlatformInitDisplay(GMON_PLATFORM_DISPLAY_SPI, &ssd1315_display_pin_spi);
    XASSERT(ssd1315_display_pin_spi != NULL);
    // Note: some OLED devices may not have RST pin / DC pin
    ssd1315_display_pin_rst = staPlatformiGetDisplayRstPin();
    ssd1315_display_pin_dc  = staPlatformiGetDisplayDataCmdPin();
    if(ssd1315_display_pin_rst != NULL) {
        status = staPlatformPinSetDirection(ssd1315_display_pin_rst, GMON_PLATFORM_PIN_DIRECTION_OUT);
        if(status != GMON_RESP_OK) { goto done; }
    }
    if(ssd1315_display_pin_dc != NULL) {
        status = staPlatformPinSetDirection(ssd1315_display_pin_dc, GMON_PLATFORM_PIN_DIRECTION_OUT);
        if(status != GMON_RESP_OK) { goto done; }
    }
    status = staOLEDcmdInit();
    if(status != GMON_RESP_OK) { goto done; }
    oled_dev.inverted = 0;
    oled_dev.initialized = 1;
    oled_dev.screen_width  = GMON_CFG_OLED_SSD1315_SCREEN_WIDTH ;
    oled_dev.screen_height = GMON_CFG_OLED_SSD1315_SCREEN_HEIGHT;
done:
    return status;
} // end of staDisplayDevInit


gMonStatus  staDisplayDevDeInit(void)
{
    gMonStatus status = GMON_RESP_OK;
    status = staOutDevPlatformDeinitDisplay(ssd1315_display_pin_spi);
    ssd1315_display_pin_spi = NULL;
    return status;
} // end of staDisplayDevDeInit


unsigned short  staDisplayDevGetScreenWidth(void)
{
    return oled_dev.screen_width;
} // end of staDisplayDevGetScreenWidth


unsigned short  staDisplayDevGetScreenHeight(void)
{
    return oled_dev.screen_height;
} // end of staDisplayDevGetScreenHeight



static gMonStatus  staDiplayDrawPixel(uint16_t x, uint16_t y, uint8_t color)
{
    gMonStatus status = GMON_RESP_OK;
    if((GMON_CFG_OLED_SSD1315_SCREEN_WIDTH <= x) || (GMON_CFG_OLED_SSD1315_SCREEN_HEIGHT <= y)) {
        status = GMON_RESP_ERRMEM;
    } else {
        if(color != 0x0) {
            gmon_ssd1315_framebuf[x + (y >> 3) * GMON_CFG_OLED_SSD1315_SCREEN_WIDTH] |=  (1 << (y%8));
        } else {
            gmon_ssd1315_framebuf[x + (y >> 3) * GMON_CFG_OLED_SSD1315_SCREEN_WIDTH] &= ~(1 << (y%8));
        }
    }
    return status;
} // end of staDiplayDrawPixel


static  gMonStatus   staDiplayDevPrintChar(char chr, uint16_t start_x, uint16_t start_y, gmonPrintFont_t *font)
{
    uint16_t idx = 0, jdx = 0, rowpattern;
    uint8_t  color = 0x1;
    gMonStatus status = GMON_RESP_OK;

    if(chr < 0x20 || font == NULL) {
        status = GMON_RESP_ERRARGS; goto done;
    } else if ((start_x > font->width) || (start_y > font->height)) {
        status = GMON_RESP_ERRARGS; goto done;
    }

    for(idx = 0; idx < (font->height - start_y); idx++) {
        rowpattern = font->bitmap[(chr - 0x20) * font->height + idx];
        rowpattern <<= start_x;
        for(jdx = 0; jdx < (font->width - start_x); jdx++) {
            if(GMON_CFG_OLED_SSD1315_SCREEN_WIDTH <= (oled_dev.curr_x + jdx + 1)) {
                break;
            }
            color = (rowpattern & 0x8000) ? 0xff: 0x0;
            status = staDiplayDrawPixel(oled_dev.curr_x + jdx, oled_dev.curr_y + idx, color);
            if(status < 0) { goto done; }
            rowpattern <<= 1;
        } // end of for loop jdx
        if(GMON_CFG_OLED_SSD1315_SCREEN_HEIGHT <= (oled_dev.curr_y + idx)) {
            status = GMON_RESP_SKIP; break;
        }
    } // end of for loop idx
    oled_dev.curr_x += jdx; // font->width
done:
    return status;
} // end of staDiplayDevPrintChar


gMonStatus  staDiplayDevPrintString(gmonPrintInfo_t *printinfo)
{
    if(printinfo == NULL || printinfo->str.data == NULL || printinfo->str.len <= 0) {
        return GMON_RESP_ERRARGS;
    }
    uint16_t  font_width = 0;
    uint16_t  num_chr_skip = 0;
    short     curr_posx = 0;
    uint16_t  idx = 0;
    gMonStatus status = GMON_RESP_OK;

    curr_posx  = printinfo->posx;
    font_width = printinfo->font->width;
    // if position x is negative integer, skip number of characters from the beginning of the given text
    if(curr_posx < 0) {
        curr_posx  = curr_posx * -1;
        num_chr_skip = curr_posx / font_width; // #chars to skip at the beginning.
        curr_posx -= num_chr_skip * font_width; // 0 < curr_posx < font_width
        // the first character may be partially printed
        XASSERT((0 <= curr_posx) && (curr_posx < font_width));
        staOLEDsetCursor(0, printinfo->posy);
    } else {
        staOLEDsetCursor(curr_posx, printinfo->posy);
        curr_posx  = 0;
    }
    for(idx = num_chr_skip; idx < printinfo->str.len; idx++) {
        status = staDiplayDevPrintChar(printinfo->str.data[idx] , curr_posx, 0, printinfo->font);
        if(status != GMON_RESP_OK) { break; }
        if(curr_posx != 0) { curr_posx  = 0; }
    } // end of for loop
    return status;
} // end of staDiplayDevPrintString


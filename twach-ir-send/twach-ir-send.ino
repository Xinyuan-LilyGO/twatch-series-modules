#include <FT5206.h>
#include <TFT_eSPI.h>
#include "board_def.h"
#include <lvgl.h>
#include "axp20x.h"
#include <Ticker.h>
#include <pcf8563.h>
#include <soc/rtc.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <IRremote.h>

#define IR_SEND     25

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
AXP20X_Class axp;
IRsend irsend(IR_SEND);

static lv_obj_t *celc, *text, *gContainer, *imgbtn1, *imgbtn2, *imgbtn3;
static char buff[256];
static int level = 24;

LV_IMG_DECLARE(imgAdd);
LV_IMG_DECLARE(imgBg);
LV_IMG_DECLARE(imgLess);
LV_IMG_DECLARE(imgOff);
LV_FONT_DECLARE(FontGeometr);
LV_FONT_DECLARE(imgCelcius);
LV_FONT_DECLARE(imgCool);
LV_FONT_DECLARE(imgSleep);


//OFF
const unsigned int  airOFF[211] = {9050, 4400, 600, 1550, 650, 1550, 600, 450, 600, 400, 650, 400, 650, 400, 650, 1550, 600, 1550, 650, 1550, 600, 1550, 600, 1550, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 1550, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 1550, 600, 1550, 600, 1550, 650, 1550, 600, 1550, 650, 400, 650, 400, 650, 400, 700}; // NEC C3E90000
// unsigned int  data = 0xC3E90000;

const unsigned int  airLevel30[211] = {9000, 4400, 600, 1550, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 1550, 600, 1550, 600, 1550, 650, 1550, 600, 1550, 650, 400, 650, 1550, 600, 1550, 650, 400, 600, 1600, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 550, 450, 650, 400, 650, 1550, 600, 450, 600, 1550, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 600, 450, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 400, 650, 400, 700, 350, 700, 350, 650, 400, 650, 400, 700, 350, 650, 400, 650, 400, 650, 1500, 650, 400, 650, 1550, 650, 1500, 650, 400, 650, 1550, 650, 400, 650}; // NEC C3ED0000
// unsigned int  data = 0xC3ED0000;

const unsigned int  airLevel29[211] = {9000, 4400, 650, 1550, 600, 1550, 650, 400, 650, 400, 600, 450, 600, 450, 600, 1550, 650, 1550, 600, 1550, 650, 1550, 600, 1550, 600, 1600, 600, 400, 650, 1550, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 1550, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 1550, 600, 450, 600, 1550, 600, 450, 600}; // NEC C3F50000
// unsigned int  data = 0xC3F50000;

const unsigned int  airLevel28[211] = {9050, 4400, 650, 1550, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 1550, 600, 1550, 600, 1600, 600, 1550, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 1600, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 1550, 650, 400, 650}; // NEC C3E50000
// unsigned int  data = 0xC3E50000;

const unsigned int  airLevel27[211] = {9000, 4450, 600, 1550, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 1550, 600, 1550, 600, 1600, 600, 1550, 600, 1550, 650, 1550, 600, 1550, 650, 400, 650, 400, 650, 1550, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 1550, 600, 450, 600, 1550, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 1600, 600, 400, 650, 400, 650, 400, 650, 400, 650, 1550, 600, 450, 600}; // NEC C3F90000
// unsigned int  data = 0xC3F90000;

const unsigned int  airLevel26[211] = {9000, 4450, 600, 1550, 600, 1600, 600, 450, 600, 400, 650, 400, 650, 400, 650, 1550, 600, 1550, 650, 1550, 600, 1550, 600, 1550, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 1550, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 1600, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 600, 1600, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 1550, 600, 1550, 600, 1550, 650, 400, 650, 400, 650}; // NEC C3E90000
// unsigned int  data = 0xC3E90000;

const unsigned int  airLevel25[211] = {9000, 4400, 650, 1550, 600, 1550, 650, 400, 650, 400, 650, 400, 600, 450, 600, 1600, 600, 1550, 600, 1550, 650, 1550, 600, 1550, 650, 1550, 600, 450, 600, 450, 600, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 1550, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 1550, 650, 1550, 600, 450, 600, 400, 650}; // NEC C3F10000
// unsigned int  data = 0xC3F10000;

const unsigned int  airLevel24[211] = {8950, 4400, 650, 1550, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 1550, 600, 1550, 600, 1550, 650, 1550, 600, 1550, 650, 400, 650, 400, 650, 400, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 1550, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 1550, 600, 450, 600, 1550, 650, 400, 600, 450, 600}; // NEC C3E10000
// unsigned int  data = 0xC3E10000;

const unsigned int  airLevel23[211] = {8950, 4450, 600, 1550, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 1550, 650, 1550, 600, 1550, 650, 1550, 600, 1550, 600, 1600, 600, 1550, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 600, 450, 650, 400, 650, 1550, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 600, 450, 600, 1600, 600, 400, 650, 400, 650}; // NEC C3FE0000
// unsigned int  data = 0xC3FE0000;

const unsigned int  airLevel22[211] = {9000, 4450, 600, 1550, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 600, 1600, 600, 1550, 600, 1550, 650, 1550, 600, 1550, 650, 400, 650, 1550, 600, 1550, 600, 1600, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 1550, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 1550, 600, 1550, 650, 400, 600, 450, 600, 450, 600}; // NEC C3EE0000
// unsigned int  data = 0xC3EE0000;

const unsigned int  airLevel21[211] = {8900, 4400, 650, 1550, 600, 1550, 650, 400, 650, 400, 650, 400, 600, 450, 600, 1550, 650, 1550, 600, 1550, 650, 1550, 600, 1550, 650, 1550, 600, 450, 600, 1550, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 600, 450, 650, 1550, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 550, 500, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 1550, 650, 400, 650, 400, 600, 450, 600}; // NEC C3F60000
// unsigned int  data = 0xC3F60000;

const unsigned int  airLevel20[211] = {9050, 4400, 650, 1550, 600, 1550, 650, 400, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 1550, 600, 1550, 650, 1550, 600, 1550, 600, 450, 600, 450, 600, 1550, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 1550, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600}; // NEC C3E60000
// unsigned int  data = 0xC3E60000;

const unsigned int  airLevel19[211] = {9050, 4400, 600, 1600, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 1550, 600, 1550, 600, 1550, 650, 1550, 600, 1550, 650, 1550, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600}; // NEC C3FA0000
// unsigned int  data = 0xC3FA0000;

const unsigned int  airLevel18[211] = {9000, 4450, 600, 1550, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 600, 1600, 600, 1550, 600, 1550, 650, 1550, 600, 1550, 650, 400, 650, 1550, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 1600, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 1550, 600, 450, 600, 1550, 650, 1550, 600, 1550, 650, 1550, 600, 1550, 600}; // NEC C3EA0000
// unsigned int  data = 0xC3EA0000;

const unsigned int  airLevel17[211] = {9000, 4400, 650, 1550, 600, 1550, 650, 400, 650, 400, 650, 400, 600, 450, 600, 1550, 650, 1550, 600, 1550, 650, 1550, 600, 1550, 650, 1550, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 600, 1600, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 1550, 600, 450, 600, 450, 600, 1550, 600, 1550, 650, 1550, 600, 1550, 650}; // NEC C3F20000
// unsigned int  data = 0xC3F20000;

const unsigned int  airLevel16[211] = {9000, 4450, 600, 1550, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 1550, 650, 1550, 600, 1550, 650, 1550, 600, 450, 600, 450, 600, 450, 600, 1550, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 600, 1600, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 400, 650, 400, 650, 400, 650, 400, 600, 450, 600, 450, 600, 450, 600, 1550, 650, 1550, 600, 450, 600, 1550, 600, 450, 600, 1550, 650, 1550, 600, 1550, 650}; // NEC C3E20000
// unsigned int  data = 0xC3E20000;



const unsigned int *airArray[] = {
    airLevel16,
    airLevel17,
    airLevel18,
    airLevel19,
    airLevel20,
    airLevel21,
    airLevel22,
    airLevel23,
    airLevel24,
    airLevel25,
    airLevel26,
    airLevel27,
    airLevel28,
    airLevel29,
    airLevel30
};

void ir_air_off()
{
    irsend.sendRaw(airOFF, 211, 38);
}

void ir_send_to_air(int level)
{
    irsend.sendRaw(airArray[level - sizeof(airArray) / sizeof(airArray[0]) - 1], 211, 38);
}

void ex_disp_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_array)
{
    uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1) * 2;
    tft->setAddrWindow(x1, y1, x2, y2);
    tft->pushColors((uint8_t *)color_array, size);
    lv_flush_ready();
}

static lv_res_t lv_btm_release_action ( lv_obj_t *obj)
{
    static bool onoff = false;
    if (obj == imgbtn1) {
        level = level + 1 > 30 ? 30 : level + 1;
        snprintf(buff, sizeof(buff), "%d", level);
        lv_label_set_text(text, buff);
        ir_send_to_air(level);
    } else if (obj == imgbtn2) {
        level = level - 1 < 16 ? 16 : level - 1;
        snprintf(buff, sizeof(buff), "%d", level);
        lv_label_set_text(text, buff);
        ir_send_to_air(level);
    } else if (obj == imgbtn3) {
        const void *img = onoff ? &imgSleep : &imgCool;
        lv_imgbtn_set_src(imgbtn3, LV_BTN_STATE_REL, img);
        lv_imgbtn_set_src(imgbtn3, LV_BTN_STATE_PR, img);
        lv_imgbtn_set_src(imgbtn3, LV_BTN_STATE_TGL_REL, img);
        lv_imgbtn_set_src(imgbtn3, LV_BTN_STATE_TGL_PR, img);
        if (onoff) {
            ir_air_off();
        } else {
            ir_send_to_air(24);
        }
        onoff = !onoff;
    }
}


static void remote_create(void)
{
    static lv_style_t style1;
    lv_style_copy(&style1, &lv_style_plain);
    style1.text.font = &FontGeometr;
    style1.text.color = LV_COLOR_WHITE;

    gContainer = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_set_size(gContainer,  LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style(gContainer, &lv_style_transp_fit);

    lv_obj_t *wp = lv_img_create(gContainer, NULL);
    lv_obj_set_protect(wp, LV_PROTECT_PARENT);          /*Don't let to move the wallpaper by the layout */
    lv_img_set_src(wp, &imgBg);
    lv_obj_set_size(wp, LV_HOR_RES, LV_VER_RES);

    text = lv_label_create(gContainer, NULL);
    lv_label_set_text(text, "24");
    lv_label_set_style(text, &style1);
    lv_obj_align(text, gContainer, LV_ALIGN_IN_TOP_MID, 0, 0);

    celc = lv_img_create(gContainer, NULL);
    lv_img_set_src(celc, &imgCelcius);
    lv_obj_align(celc, text, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);

    imgbtn1 = lv_imgbtn_create(gContainer, NULL);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_REL, &imgAdd);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_PR, &imgOff);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_TGL_REL, &imgAdd);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_TGL_PR, &imgOff);
    lv_imgbtn_set_style(imgbtn1, LV_BTN_STATE_PR, &lv_style_plain);        /*Use the darker style in the pressed state*/
    lv_imgbtn_set_style(imgbtn1, LV_BTN_STATE_TGL_PR, &lv_style_plain);
    lv_imgbtn_set_toggle(imgbtn1, true);
    lv_obj_align(imgbtn1, NULL, LV_ALIGN_IN_RIGHT_MID, -10, 50);


    static lv_style_t style2;
    lv_style_copy(&style2, &lv_style_plain);
    style2.text.font = &lv_font_dejavu_20;
    style2.text.color = LV_COLOR_WHITE;

    imgbtn2 = lv_imgbtn_create(gContainer, NULL);
    lv_imgbtn_set_src(imgbtn2, LV_BTN_STATE_REL, &imgLess);
    lv_imgbtn_set_src(imgbtn2, LV_BTN_STATE_PR, &imgOff);
    lv_imgbtn_set_src(imgbtn2, LV_BTN_STATE_TGL_REL, &imgLess);
    lv_imgbtn_set_src(imgbtn2, LV_BTN_STATE_TGL_PR, &imgOff);
    lv_imgbtn_set_style(imgbtn2, LV_BTN_STATE_PR, &lv_style_plain);        /*Use the darker style in the pressed state*/
    lv_imgbtn_set_style(imgbtn2, LV_BTN_STATE_TGL_PR, &lv_style_plain);
    lv_imgbtn_set_toggle(imgbtn2, true);
    lv_obj_align(imgbtn2, NULL, LV_ALIGN_IN_LEFT_MID, 10, 50);


    imgbtn3 = lv_imgbtn_create(gContainer, NULL);
    lv_imgbtn_set_src(imgbtn3, LV_BTN_STATE_REL, &imgSleep);
    lv_imgbtn_set_src(imgbtn3, LV_BTN_STATE_PR, &imgSleep);
    lv_imgbtn_set_src(imgbtn3, LV_BTN_STATE_TGL_REL, &imgSleep);
    lv_imgbtn_set_src(imgbtn3, LV_BTN_STATE_TGL_PR, &imgSleep);
    lv_imgbtn_set_style(imgbtn3, LV_BTN_STATE_PR, &lv_style_plain);        /*Use the darker style in the pressed state*/
    lv_imgbtn_set_style(imgbtn3, LV_BTN_STATE_TGL_PR, &lv_style_plain);
    lv_obj_align(imgbtn3, NULL, LV_ALIGN_CENTER, 0, 50);

    lv_imgbtn_set_action(imgbtn1, LV_BTN_ACTION_CLICK, lv_btm_release_action);
    lv_imgbtn_set_action(imgbtn2, LV_BTN_ACTION_CLICK, lv_btm_release_action);
    lv_imgbtn_set_action(imgbtn3, LV_BTN_ACTION_CLICK, lv_btm_release_action);
}


void setup()
{
    Serial.begin(115200);
    tft = new TFT_eSPI(LV_HOR_RES, LV_VER_RES);
    tft->init();
    tft->setRotation(0);

    pinMode(TP_INT, INPUT);
    Wire.begin(I2C_SDA, I2C_SCL);
    tp = new FT5206_Class();
    if (! tp->begin(Wire)) {
        Serial.println("Couldn't start FT5206 touchscreen controller");
    } else {
        Serial.println("Capacitive touchscreen started");
    }

    lv_init();

    /*Initialize the display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.disp_flush = ex_disp_flush; /*Used in buffered mode (LV_VDB_SIZE != 0  in lv_conf.h)*/
    lv_disp_drv_register(&disp_drv);

    /*Initialize the touch pad*/
    lv_indev_drv_t indev_drv;
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read =  [] (lv_indev_data_t *data) -> bool {
        static TP_Point p;
        data->state = tp->touched() ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
        if (data->state == LV_INDEV_STATE_PR)
        {
            p = tp->getPoint();
            p.x = map(p.x, 0, 320, 0, 240);
            p.y = map(p.y, 0, 320, 0, 240);
        }
        /*Set the coordinates (if released use the last pressed coordinates)*/
        data->point.x = p.x;
        data->point.y = p.y;
        return false; /*Return false because no moare to be read*/
    };
    lv_indev_drv_register(&indev_drv);

    lvTicker1.attach_ms(20, [] {
        lv_tick_inc(20);
    });

    lvTicker2.attach_ms(5, [] {
        lv_task_handler();
    });

    Wire1.begin(SEN_SDA, SEN_SCL);
    axp.begin(Wire1);
    axp.setPowerOutPut(AXP202_LDO2, AXP202_ON);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, 1);

    irsend.enableIROut(38);

    remote_create();
}



void loop()
{
    delay(100000);
}
#include <FT5206.h>
#include <TFT_eSPI.h>
#include "board_def.h"
#include <lvgl.h>
#include "axp20x.h"
#include <Ticker.h>

TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
AXP20X_Class axp;
lv_obj_t *label;

void ex_disp_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_array)
{
    uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1) * 2;
    tft->setAddrWindow(x1, y1, x2, y2);
    tft->pushColors((uint8_t *)color_array, size);
    lv_flush_ready();
}

void setup()
{
    Serial.begin(115200);
    Serial.println(F("Cap Touch Paint!"));

    tft = new TFT_eSPI(LV_HOR_RES, LV_VER_RES);
    tft->init();
    tft->setRotation(0);

    pinMode(TP_INT, INPUT);
    Wire.begin(I2C_SDA, I2C_SCL);
    tp = new FT5206_Class();
    if (! tp->begin(Wire)) {
        Serial.println("Couldn't start FT5206 touchscreen controller");
        while (1);
    } else {
        Serial.println("Capacitive touchscreen started");
    }

    lv_init();

    /*Initialize the display*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.disp_flush = ex_disp_flush; /*Used in buffered mode (LV_VDB_SIZE != 0  in lv_conf.h)*/
    lv_disp_drv_register(&disp_drv);

    lvTicker1.attach_ms(20, [] {
        lv_tick_inc(20);
    });


    label = lv_label_create(lv_scr_act(), NULL);
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label, "Tp test");


    Wire1.begin(SEN_SDA, SEN_SCL);
    axp.begin(Wire1);
    axp.setPowerOutPut(AXP202_LDO2, AXP202_ON);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, 1);

}

char buff[256];
uint64_t timermap = 0;

void loop()
{
    if (millis() - timermap > 5) {
        lv_task_handler();
        timermap = millis();
    }
    if (tp->touched()) {
        TP_Point  p = tp->getPoint();
        p.x = map(p.x, 0, 320, 0, 240);
        p.y = map(p.y, 0, 320, 0, 240);
        snprintf(buff, sizeof(buff), "x:%d y:%d", p.x, p.y);
        lv_label_set_text(label, buff);
        lv_obj_align(label, NULL, LV_ALIGN_CENTER, 0, 0);
        Serial.println(buff);
    }
}
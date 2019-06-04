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
#include <Adafruit_MPR121.h>

#ifndef _BV
#define _BV(bit) (1 << (bit))
#endif

TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
AXP20X_Class axp;
Adafruit_MPR121 cap;

lv_obj_t *btn, *text, *gContainer;

uint16_t lasttouched = 0;
uint16_t currtouched = 0;
char buff[256];

LV_IMG_DECLARE(img_press);
LV_IMG_DECLARE(img_release);


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

    if (!cap.begin(MPR121_I2CADDR_DEFAULT, &Wire1)) {
        Serial.println("MPR121 not found, check wiring?");
        while (1);
    }

    gContainer = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_set_size(gContainer,  LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style(gContainer, &lv_style_transp_fit);

    btn = lv_img_create(gContainer, NULL);
    lv_img_set_src(btn, &img_release);
    lv_obj_align(btn, NULL, LV_ALIGN_CENTER, 0, 0);

    text = lv_label_create(gContainer, NULL);
    lv_label_set_text(text, "Button is released");
    lv_obj_align(text, btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
}


void loop()
{
    // Get the currently touched pads
    currtouched = cap.touched();

    for (uint8_t i = 0; i < 12; i++) {
        // it if *is* touched and *wasnt* touched before, alert!
        if ((currtouched & _BV(i)) && !(lasttouched & _BV(i)) ) {
            lv_img_set_src(btn, &img_press);
            lv_obj_align(btn, NULL, LV_ALIGN_CENTER, 0, 0);
            snprintf(buff, sizeof(buff), "Cap %d button is pressed", i);
            lv_label_set_text(text, buff);
            lv_obj_align(text, btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
            Serial.print(i); Serial.println(" touched");
        }
        // if it *was* touched and now *isnt*, alert!
        if (!(currtouched & _BV(i)) && (lasttouched & _BV(i)) ) {
            lv_img_set_src(btn, &img_release);
            lv_obj_align(btn, NULL, LV_ALIGN_CENTER, 0, 0);
            snprintf(buff, sizeof(buff), "Cap %d button is released", i);
            lv_label_set_text(text, buff);
            lv_obj_align(text, btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
            Serial.print(i); Serial.println(" released");
        }
    }

    // reset our state
    lasttouched = currtouched;

    // comment out this line for detailed data from the sensor!
    return;
}
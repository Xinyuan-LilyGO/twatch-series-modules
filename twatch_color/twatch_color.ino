#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "board_def.h"
#include <lvgl.h>
#include "axp20x.h"
#include <FT5206.h>
#include <Ticker.h>
#include "Adafruit_TCS34725.h"

TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
AXP20X_Class axp;

LV_IMG_DECLARE(img_err);

lv_obj_t *gContainer = NULL;

uint8_t gammatable[256];

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

void ex_disp_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color_array)
{
    uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1) * 2;
    tft->setAddrWindow(x1, y1, x2, y2);
    tft->pushColors((uint8_t *)color_array, size);
    lv_flush_ready();
}

void lv_dirver_init()
{
    tft = new TFT_eSPI(LV_HOR_RES, LV_VER_RES);
    tft->init();
    tft->setRotation(0);

    pinMode(TP_INT, INPUT);
    Wire1.begin(I2C_SDA, I2C_SCL);
    tp = new FT5206_Class();
    if (! tp->begin(Wire1)) {
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
}

void setup()
{
    Serial.begin(115200);

    lv_dirver_init();

    Wire.begin(SEN_SDA, SEN_SCL);
    axp.begin(Wire);
    axp.setPowerOutPut(AXP202_LDO2, AXP202_ON);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, 1);

    gContainer = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_set_size(gContainer,  LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style(gContainer, &lv_style_transp_fit);

    lv_obj_t *err = NULL;
    bool ret = false;
    do {
        ret = tcs.begin();
        if (!ret && !err) {
            lv_obj_t *img = lv_img_create(gContainer, NULL);
            lv_img_set_src(img, &img_err);
            lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);
            err = lv_label_create(gContainer, NULL);
            lv_label_set_text(err, "Could not find sensor");
            lv_obj_align(err, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        }else
        Serial.println("No TCS34725 found ... check your connections");
        delay(1000);
    } while (!ret);

    if (err) {
        lv_obj_clean(gContainer);
    }
    lvTicker2.detach();

    for (int i = 0; i < 256; i++) {
        float x = i;
        x /= 255;
        x = pow(x, 2.5);
        x *= 255;

        if (1) {
            gammatable[i] = 255 - x;
        } else {
            gammatable[i] = x;
        }
    }
}


void loop()
{
    uint16_t clear, red, green, blue;
    tcs.getRawData(&red, &green, &blue, &clear);
    uint32_t sum = clear;
    float r, g, b;
    r = red; r /= sum;
    g = green; g /= sum;
    b = blue; b /= sum;
    r *= 256; g *= 256; b *= 256;
    Serial.print("\t");
    Serial.print((int)r, HEX); Serial.print((int)g, HEX); Serial.print((int)b, HEX);
    Serial.println();

    tft->fillScreen(tft->color565(255 - gammatable[(int)r], 255 - gammatable[(int)g], 255 - gammatable[(int)b]));
    delay(1000);
}







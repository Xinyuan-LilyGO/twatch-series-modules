#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "board_def.h"
#include <lvgl.h>
#include "axp20x.h"
#include <FT5206.h>
#include <Ticker.h>
#include <Button2.h>


TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
AXP20X_Class axp;

LV_IMG_DECLARE(hello1);
LV_IMG_DECLARE(hello2);
LV_IMG_DECLARE(hello3);
LV_IMG_DECLARE(hello4);
LV_IMG_DECLARE(hello5);
LV_IMG_DECLARE(hello6);
LV_IMG_DECLARE(hello7);
LV_IMG_DECLARE(hello8);


lv_obj_t *gContainer = NULL;


#include "AudioGeneratorMP3.h"
#include "AudioFileSourcePROGMEM.h"
#include "AudioFileSourceID3.h"
#include "AudioOutputI2S.h"
#include "pika.h"
AudioFileSourcePROGMEM *file;
AudioGeneratorMP3 *mp3;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

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

    pinMode(TP_INT, INPUT);
    Wire1.begin(I2C_SDA, I2C_SCL);
    tp = new FT5206_Class();
    if (! tp->begin(Wire1)) {
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
}


lv_task_t *taskHandle = nullptr;
lv_obj_t *imgbtn1;

void taskfunc(void *t)
{
    const int len = 8;
    const void *data[len] = {
        &hello1,
        &hello2,
        &hello3,
        &hello4,
        &hello5,
        &hello6,
        &hello7,
        &hello8,
    };
    static int i = 0;
    ledcWriteTone(1, 1000);
    const void *src = data[i++];
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_REL, src);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_PR, src);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_TGL_REL, src);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_TGL_PR, src);

    if (i >= len) {
        i = 0;
        lv_task_del(taskHandle);
        taskHandle = nullptr;
        // ledcWriteTone(0, 0);
        ledcWriteTone(1, 0);
        lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_REL, &hello1);
        lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_PR, &hello1);
        lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_TGL_REL, &hello1);
        lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_TGL_PR, &hello1);
    }
}


lv_res_t lv_btm_action1 ( lv_obj_t *obj)
{
    Serial.println("LV_BTN_ACTION_CLICK");

    if (taskHandle != nullptr)return 0 ;
    if (file->isOpen()) return 0;
    file->open(pika, sizeof(pika));
    mp3->begin(id3, out);
    taskHandle = lv_task_create(taskfunc, 100, LV_TASK_PRIO_MID, NULL);
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
    axp.setLDO3Mode(1);
    axp.setPowerOutPut(AXP202_LDO3, AXP202_ON);

    gContainer = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_set_size(gContainer,  LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style(gContainer, &lv_style_transp_fit);


    ledcSetup(1, 1000, 16);
    ledcAttachPin(33, 1);

    static lv_style_t style_pr;
    lv_style_copy(&style_pr, &lv_style_plain);
    style_pr.image.color = LV_COLOR_BLACK;
    style_pr.image.intense = LV_OPA_50;
    style_pr.text.color = LV_COLOR_HEX3(0xaaa);

    imgbtn1 = lv_imgbtn_create(gContainer, NULL);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_REL, &hello1);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_PR, &hello1);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_TGL_REL, &hello1);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_TGL_PR, &hello1);
    lv_imgbtn_set_toggle(imgbtn1, true);
    lv_obj_align(imgbtn1, NULL, LV_ALIGN_CENTER, 0, 0);

    lv_imgbtn_set_action(imgbtn1, LV_BTN_ACTION_CLICK, lv_btm_action1);
    file = new AudioFileSourcePROGMEM(pika, sizeof(pika));
    id3 = new AudioFileSourceID3(file);
    out = new AudioOutputI2S(0, 1);
    mp3 = new AudioGeneratorMP3();
    mp3->begin(id3, out);
}

void loop()
{
    if (mp3->isRunning()) {
        if (!mp3->loop()) mp3->stop();
    } else {
        delay(1000);
    }
}







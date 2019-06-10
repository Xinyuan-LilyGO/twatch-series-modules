#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "board_def.h"
#include <lvgl.h>
#include "axp20x.h"
#include <FT5206.h>
#include <Ticker.h>

#define MOTOR_L                     25
#define MOTOR_R                     26

TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
AXP20X_Class axp;

LV_IMG_DECLARE(img_backleft);
LV_IMG_DECLARE(img_backright);
LV_IMG_DECLARE(img_robot);
LV_IMG_DECLARE(img_stop);


lv_obj_t *gContainer = NULL;
lv_obj_t *btn1, *btn2, * btn3;

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


lv_obj_t *create_imgbtn(lv_obj_t *par, const void *src)
{
    static lv_style_t style_pr;
    lv_style_copy(&style_pr, &lv_style_plain);
    style_pr.image.color = LV_COLOR_BLACK;
    style_pr.image.intense = LV_OPA_50;
    style_pr.text.color = LV_COLOR_HEX3(0xaaa);

    lv_obj_t *btn = lv_imgbtn_create(par, NULL);
    lv_imgbtn_set_src(btn, LV_BTN_STATE_REL, src);
    lv_imgbtn_set_src(btn, LV_BTN_STATE_PR, src);
    lv_imgbtn_set_src(btn, LV_BTN_STATE_TGL_REL, src);
    lv_imgbtn_set_src(btn, LV_BTN_STATE_TGL_PR, src);
    lv_imgbtn_set_style(btn, LV_BTN_STATE_PR, &style_pr);        /*Use the darker style in the pressed state*/
    lv_imgbtn_set_style(btn, LV_BTN_STATE_TGL_PR, &style_pr);
    lv_imgbtn_set_toggle(btn, true);
    return btn;
}

static lv_res_t btnm_action(lv_obj_t *btnm)
{
    if (btn1 == btnm) {
        ledcWrite(1, 1638);
        ledcWrite(2, 1638);
    } else if (btn2 == btnm) {
        ledcWrite(1, 6000);
        ledcWrite(2, 6000);
    } else if (btn3 == btnm) {
        ledcWrite(1, 0);
        ledcWrite(2, 0);
    }
    return LV_RES_OK;
}

void setup()
{
    Serial.begin(115200);

    ledcSetup(1, 50, 16);
    ledcAttachPin(MOTOR_L, 1);   

    ledcSetup(2, 50, 16); 
    ledcAttachPin(MOTOR_R, 2);  

    lv_dirver_init();

    Wire1.begin(SEN_SDA, SEN_SCL);
    axp.begin(Wire1);
    axp.setPowerOutPut(AXP202_LDO2, AXP202_ON);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, 1);

    gContainer = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_set_size(gContainer,  LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style(gContainer, &lv_style_transp_fit);

    lv_obj_t *img = lv_img_create(gContainer, NULL);
    lv_img_set_src(img, &img_robot);
    lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);

    btn1 = create_imgbtn(gContainer, &img_backleft);
    lv_obj_align(btn1, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

    btn2 = create_imgbtn(gContainer, &img_backright);
    lv_obj_align(btn2, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);

    btn3 = create_imgbtn(gContainer, &img_stop);
    lv_obj_align(btn3, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

    lv_imgbtn_set_action(btn1, LV_BTN_ACTION_PR, btnm_action);
    lv_imgbtn_set_action(btn2, LV_BTN_ACTION_PR, btnm_action);
    lv_imgbtn_set_action(btn3, LV_BTN_ACTION_PR, btnm_action);
}


void loop()
{
}
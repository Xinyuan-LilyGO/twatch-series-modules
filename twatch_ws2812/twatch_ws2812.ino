#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "board_def.h"
#include <lvgl.h>
#include "axp20x.h"
#include <FT5206.h>
#include <Ticker.h>
#include <Button2.h>
#include <WS2812FX.h>


#define LED_PIN     25                       // 0 = GPIO0, 2=GPIO2
#define LED_COUNT   4

#define DEFAULT_COLOR 0xFF5900
#define DEFAULT_BRIGHTNESS 255
#define DEFAULT_SPEED 1000
#define DEFAULT_MODE FX_MODE_STATIC

TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
AXP20X_Class axp;
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

LV_IMG_DECLARE(imgBlue);
LV_IMG_DECLARE(imgGreen);
LV_IMG_DECLARE(imgRed);
LV_IMG_DECLARE(imgRainbow);
LV_IMG_DECLARE(imgBg);
LV_IMG_DECLARE(imgOff);

lv_obj_t *gContainer = NULL;
lv_obj_t *imgbtn0, *imgbtn1, *imgbtn2, *imgbtn3;


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
        // while (1);
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


lv_res_t lv_btm_action ( lv_obj_t *obj)
{
    if (obj == imgbtn0) {
        ws2812fx.setMode(FX_MODE_RAINBOW_CYCLE);
    } else if (obj == imgbtn1) {
        ws2812fx.setMode(DEFAULT_MODE);
        ws2812fx.setColor(BLUE);
    } else if (obj == imgbtn2) {
        ws2812fx.setMode(DEFAULT_MODE);
        ws2812fx.setColor(GREEN);
    } else if (obj == imgbtn3) {
        ws2812fx.setMode(DEFAULT_MODE);
        ws2812fx.setColor(RED);
    }
    return LV_RES_OK;
}

static lv_res_t slider_action(lv_obj_t *slider)
{
    printf("New slider value: %d\n", lv_slider_get_value(slider));
    ws2812fx.setBrightness(lv_slider_get_value(slider));
    return LV_RES_OK;
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

    lv_obj_t *wp = lv_img_create(gContainer, NULL);
    lv_obj_set_protect(wp, LV_PROTECT_PARENT);          /*Don't let to move the wallpaper by the layout */
    lv_img_set_src(wp, &imgBg);
    lv_obj_set_size(wp, LV_HOR_RES, LV_VER_RES);

    ws2812fx.init();
    ws2812fx.setMode(DEFAULT_MODE);
    ws2812fx.setColor(DEFAULT_COLOR);
    ws2812fx.setSpeed(DEFAULT_SPEED);
    ws2812fx.setBrightness(DEFAULT_BRIGHTNESS);
    ws2812fx.start();

    static lv_style_t style_pr;
    lv_style_copy(&style_pr, &lv_style_plain);
    style_pr.image.color = LV_COLOR_BLACK;
    style_pr.image.intense = LV_OPA_50;
    style_pr.text.color = LV_COLOR_HEX3(0xaaa);

    imgbtn0 = lv_imgbtn_create(gContainer, NULL);
    lv_imgbtn_set_src(imgbtn0, LV_BTN_STATE_REL, &imgRainbow);
    lv_imgbtn_set_src(imgbtn0, LV_BTN_STATE_PR, &imgRainbow);
    lv_imgbtn_set_src(imgbtn0, LV_BTN_STATE_TGL_REL, &imgRainbow);
    lv_imgbtn_set_src(imgbtn0, LV_BTN_STATE_TGL_PR, &imgRainbow);
    lv_imgbtn_set_style(imgbtn0, LV_BTN_STATE_PR, &style_pr);        /*Use the darker style in the pressed state*/
    lv_imgbtn_set_style(imgbtn0, LV_BTN_STATE_TGL_PR, &style_pr);
    lv_imgbtn_set_toggle(imgbtn0, true);
    lv_obj_align(imgbtn0, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
    lv_imgbtn_set_action(imgbtn0, LV_BTN_ACTION_CLICK, lv_btm_action);

    imgbtn1 = lv_imgbtn_create(gContainer, NULL);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_REL, &imgBlue);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_PR, &imgBlue);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_TGL_REL, &imgBlue);
    lv_imgbtn_set_src(imgbtn1, LV_BTN_STATE_TGL_PR, &imgBlue);
    lv_imgbtn_set_style(imgbtn1, LV_BTN_STATE_PR, &style_pr);        /*Use the darker style in the pressed state*/
    lv_imgbtn_set_style(imgbtn1, LV_BTN_STATE_TGL_PR, &style_pr);
    lv_imgbtn_set_toggle(imgbtn1, true);
    lv_obj_align(imgbtn1, NULL, LV_ALIGN_IN_LEFT_MID, 0, 0);
    lv_imgbtn_set_action(imgbtn1, LV_BTN_ACTION_CLICK, lv_btm_action);

    imgbtn2 = lv_imgbtn_create(gContainer, NULL);
    lv_imgbtn_set_src(imgbtn2, LV_BTN_STATE_REL, &imgGreen);
    lv_imgbtn_set_src(imgbtn2, LV_BTN_STATE_PR, &imgGreen);
    lv_imgbtn_set_src(imgbtn2, LV_BTN_STATE_TGL_REL, &imgGreen);
    lv_imgbtn_set_src(imgbtn2, LV_BTN_STATE_TGL_PR, &imgGreen);
    lv_imgbtn_set_style(imgbtn2, LV_BTN_STATE_PR, &style_pr);        /*Use the darker style in the pressed state*/
    lv_imgbtn_set_style(imgbtn2, LV_BTN_STATE_TGL_PR, &style_pr);
    lv_imgbtn_set_toggle(imgbtn2, true);
    lv_obj_align(imgbtn2, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_imgbtn_set_action(imgbtn2, LV_BTN_ACTION_CLICK, lv_btm_action);

    imgbtn3 = lv_imgbtn_create(gContainer, NULL);
    lv_imgbtn_set_src(imgbtn3, LV_BTN_STATE_REL, &imgRed);
    lv_imgbtn_set_src(imgbtn3, LV_BTN_STATE_PR, &imgRed);
    lv_imgbtn_set_src(imgbtn3, LV_BTN_STATE_TGL_REL, &imgRed);
    lv_imgbtn_set_src(imgbtn3, LV_BTN_STATE_TGL_PR, &imgRed);
    lv_imgbtn_set_style(imgbtn3, LV_BTN_STATE_PR, &style_pr);        /*Use the darker style in the pressed state*/
    lv_imgbtn_set_style(imgbtn3, LV_BTN_STATE_TGL_PR, &style_pr);
    lv_imgbtn_set_toggle(imgbtn3, true);
    lv_obj_align(imgbtn3, NULL, LV_ALIGN_IN_RIGHT_MID, 0, 0);
    lv_imgbtn_set_action(imgbtn3, LV_BTN_ACTION_CLICK, lv_btm_action);


    /*Create a bar, an indicator and a knob style*/
    static lv_style_t style_bg;
    static lv_style_t style_indic;
    static lv_style_t style_knob;

    lv_style_copy(&style_bg, &lv_style_pretty);
    style_bg.body.main_color =  LV_COLOR_BLACK;
    style_bg.body.grad_color =  LV_COLOR_GRAY;
    style_bg.body.radius = LV_RADIUS_CIRCLE;
    style_bg.body.border.color = LV_COLOR_WHITE;

    lv_style_copy(&style_indic, &lv_style_pretty);
    style_indic.body.grad_color =  LV_COLOR_GREEN;
    style_indic.body.main_color =  LV_COLOR_LIME;
    style_indic.body.radius = LV_RADIUS_CIRCLE;
    style_indic.body.shadow.width = 10;
    style_indic.body.shadow.color = LV_COLOR_LIME;
    style_indic.body.padding.hor = 3;
    style_indic.body.padding.ver = 3;

    lv_style_copy(&style_knob, &lv_style_pretty);
    style_knob.body.radius = LV_RADIUS_CIRCLE;
    style_knob.body.opa = LV_OPA_70;
    style_knob.body.padding.ver = 10 ;

    /*Create a second slider*/
    lv_obj_t *slider2 = lv_slider_create(gContainer, NULL);
    lv_slider_set_style(slider2, LV_SLIDER_STYLE_BG, &style_bg);
    lv_slider_set_style(slider2, LV_SLIDER_STYLE_INDIC, &style_indic);
    lv_slider_set_style(slider2, LV_SLIDER_STYLE_KNOB, &style_knob);
    lv_obj_align(slider2, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -40);
    lv_bar_set_value(slider2, 255);
    lv_bar_set_range(slider2, 0, 255);
    lv_slider_set_action(slider2, slider_action);
}

void loop()
{
    ws2812fx.service();
}







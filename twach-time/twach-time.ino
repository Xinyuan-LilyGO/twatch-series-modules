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

WiFiMulti wifimulti;
TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
AXP20X_Class axp;
lv_obj_t *label;
PCF8563_Class rtc;
lv_obj_t *gContainer = NULL;
lv_obj_t *img0, *img1, *img2, *img3;
bool axp_irq = false;
LV_IMG_DECLARE(img_n0);
LV_IMG_DECLARE(img_n1);
LV_IMG_DECLARE(img_n2);
LV_IMG_DECLARE(img_n3);
LV_IMG_DECLARE(img_n4);
LV_IMG_DECLARE(img_n5);
LV_IMG_DECLARE(img_n6);
LV_IMG_DECLARE(img_n7);
LV_IMG_DECLARE(img_n8);
LV_IMG_DECLARE(img_n9);

const char *nums[] = {
    (const char *) &img_n0,
    (const char *) &img_n1,
    (const char *) &img_n2,
    (const char *) &img_n3,
    (const char *) &img_n4,
    (const char *) &img_n5,
    (const char *) &img_n6,
    (const char *) &img_n7,
    (const char *) &img_n8,
    (const char *) &img_n9,
};

bool sync = false;
static lv_res_t btn_click_action(lv_obj_t *btn)
{

    printf("Button is released\n");
    sync = true;

    /* The button is released.
     * Make something here */

    return LV_RES_OK; /*Return OK if the button is not deleted*/
}

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

    wifimulti.addAP("Xiaomi", "12345678");

    Wire1.begin(SEN_SDA, SEN_SCL);
    axp.begin(Wire1);
    axp.setPowerOutPut(AXP202_LDO2, AXP202_ON);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, 1);


    axp.enableIRQ(AXP202_ALL_IRQ, AXP202_OFF);
    axp.adc1Enable(0xFF, AXP202_OFF);
    axp.adc2Enable(0xFF, AXP202_OFF);
    axp.enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_CHARGING_FINISHED_IRQ, AXP202_ON);
    axp.clearIRQ();

    pinMode(AXP202_INT, INPUT_PULLUP);
    attachInterrupt(AXP202_INT, [] {
        axp_irq = true;
    }, FALLING);
    rtc.begin(Wire1);

    gContainer = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_set_size(gContainer,  LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style(gContainer, &lv_style_transp_fit);

    lv_obj_t *info = NULL;

    img0 = lv_img_create(gContainer, NULL);
    lv_img_set_src(img0, nums[0]);
    lv_obj_align(img0, NULL, LV_ALIGN_IN_TOP_LEFT, 25, 30);

    img1 = lv_img_create(gContainer, NULL);
    lv_img_set_src(img1, nums[0]);
    lv_obj_align(img1, img0, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    img2 = lv_img_create(gContainer, NULL);
    lv_img_set_src(img2, nums[0]);
    lv_obj_align(img2, img1, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    img3 = lv_img_create(gContainer, NULL);
    lv_img_set_src(img3, nums[0]);
    lv_obj_align(img3, img2, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

    lv_obj_t *btn1 = lv_btn_create(gContainer, NULL);
    lv_cont_set_fit(btn1, true, true);
    lv_obj_align(btn1, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -30);
    lv_btn_set_action(btn1, LV_BTN_ACTION_CLICK, btn_click_action);

    lv_obj_t *label = lv_label_create(btn1, NULL);
    lv_label_set_text(label, "Sync Time");
    lv_obj_align(label, btn1, LV_ALIGN_CENTER, 0, 0);
}

uint8_t prevh, prevm;

void loop()
{
    RTC_Date dt = rtc.getDateTime();

    Serial.println(rtc.formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S));

    if (dt.hour != prevh) {
        prevh = dt.hour ;
        if (prevh / 10) {
            lv_img_set_src(img0, nums[prevh / 10 ]);
            lv_obj_align(img0, NULL, LV_ALIGN_IN_TOP_LEFT, 25, 30);
        } else {
            lv_img_set_src(img0, nums[0]);
            lv_obj_align(img0, NULL, LV_ALIGN_IN_TOP_LEFT, 25, 30);
        }
        lv_img_set_src(img1, nums[ prevh % 10]);
        lv_obj_align(img1, img0, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    }
    if (dt.minute != prevm) {
        prevm = dt.minute;
        if (prevm / 10) {
            lv_img_set_src(img2, nums[prevm / 10 ]);
            lv_obj_align(img2, img1, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
        } else {
            lv_img_set_src(img2, nums[0]);
            lv_obj_align(img2, img1, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
        }
        lv_img_set_src(img3, nums[ prevm % 10]);
        lv_obj_align(img3, img2, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    }

    if (wifimulti.run() == WL_CONNECTED) {
        if (sync) {
            configTzTime("CST-8", "pool.ntp.org");
            struct tm timeinfo;
            if (!getLocalTime(&timeinfo)) {
                Serial.println("Failed to obtain time");
            } else {
                sync = false;
                Serial.println(&timeinfo, "SYNC PASS : %A, %B %d %Y %H:%M:%S");
                rtc.setDateTime(timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
                WiFi.mode(WIFI_OFF);
            }
        }
    }


    if (axp_irq) {
        axp_irq = false;
        axp.readIRQ();
        if (axp.isPEKShortPressIRQ()) {
            digitalWrite(TFT_BL, !digitalRead(TFT_BL));
        }
        axp.clearIRQ();
    }
    delay(1000);
}
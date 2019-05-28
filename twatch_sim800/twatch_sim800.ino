#include <Arduino.h>
#include <Wire.h>
#include <sim800.h>
#include <TFT_eSPI.h>
#include "board_def.h"
#include <lvgl.h>
#include "axp20x.h"
#include <FT5206.h>
#include <Ticker.h>
#include <gprs.h>

#define UART_TX                     33
#define UART_RX                     34
#define SIMCARD_RST                 14
#define SIMCARD_PWKEY               15
#define SIMCARD_BOOST_CTRL          4
#define UART_BANUD_RATE             115200

TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
AXP20X_Class axp;

HardwareSerial hwSerial(1);
GPRS gprs(hwSerial, SIMCARD_PWKEY, SIMCARD_RST, SIMCARD_BOOST_CTRL);

LV_IMG_DECLARE(img_scall);
LV_IMG_DECLARE(img_ecall);
LV_IMG_DECLARE(img_nosim);
LV_IMG_DECLARE(img_restart);
LV_IMG_DECLARE(img_check);

void lv_call_menu();
void lv_incomming();
void calling();

Ticker callTicker;

lv_obj_t *keyboard = NULL;
lv_obj_t *phonenum = NULL;
lv_obj_t *gContainer = NULL;
lv_obj_t *gObjecter = NULL;
lv_obj_t *ecallBtn = NULL;
lv_obj_t *scallBtn = NULL;;
char phoneNum[64];
char timeBuff[64];
uint32_t calltime = 0;
bool isCalling = false;
bool callout = false;
bool inCall = false;


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

lv_res_t reboot_atcion(lv_obj_t *obj)
{
    esp_restart();
}

void reboot_sim800()
{
    lv_obj_clean(gContainer);
    lv_obj_t *img = lv_img_create(gContainer, NULL);
    lv_img_set_src(img, &img_nosim);
    lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, -30);

    lv_obj_t *label = lv_label_create(gContainer, NULL);
    lv_label_set_text(label, "No found the sim card");
    lv_obj_align(label, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


    lv_obj_t *reboot = lv_imgbtn_create(gContainer, NULL);
    lv_imgbtn_set_src(reboot, LV_BTN_STATE_REL, &img_restart);
    lv_imgbtn_set_src(reboot, LV_BTN_STATE_PR, &img_restart);
    lv_imgbtn_set_src(reboot, LV_BTN_STATE_TGL_REL, &img_restart);
    lv_imgbtn_set_src(reboot, LV_BTN_STATE_TGL_PR, &img_restart);
    lv_obj_align(reboot, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_imgbtn_set_action(reboot, LV_BTN_ACTION_PR, reboot_atcion);
}



void setup()
{
    Serial.begin(115200);

    lv_dirver_init();

    gContainer = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_set_size(gContainer,  LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style(gContainer, &lv_style_transp_fit);

    Wire1.begin(SEN_SDA, SEN_SCL);
    axp.begin(Wire1);
    axp.setPowerOutPut(AXP202_LDO2, AXP202_ON);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, 1);

    hwSerial.begin(UART_BANUD_RATE, SERIAL_8N1, UART_RX, UART_TX);
    Serial.println("Setup Complete!");

    lv_obj_t *img = lv_img_create(gContainer, NULL);
    lv_img_set_src(img, &img_check);
    lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *label = lv_label_create(gContainer, NULL);
    lv_label_set_text(label, "Begin sim card");
    lv_obj_align(label, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


    gprs.preInit();
    delay(1000);
    if (0 != gprs.init()) {
        reboot_sim800();
        while (1);
    }

    hwSerial.print("AT+CHFA=1\r\n");
    delay(2);
    hwSerial.print("AT+CRSL=100\r\n");
    delay(2);
    hwSerial.print("AT+CLVL=100\r\n");
    delay(2);
    hwSerial.print("AT+CLIP=1\r\n");
    delay(2);

    Serial.println("Init success");


    lv_call_menu();
}

bool inCalling = false;
uint64_t startTimer = 0;

void loop()
{
    if (hwSerial.available()) {
        const char *s = hwSerial.readStringUntil('\n').c_str();
        if (strstr(s, "OK" ) != NULL) {
            Serial.println("SIM OK");
        } else if (strstr(s, "+CPIN: NOT READY") != NULL) {
            Serial.println("SIM +CPIN: NOT READY");
        } else if (strstr(s, "+CPIN: READY") != NULL) {
            Serial.println("SIM +CPIN: READY");
        } else if (strstr(s, "+CLIP:") != NULL) {
            Serial.printf("SIM %s\n", s);
            if (!inCalling) {
                if (sscanf(s, "%*[^\"]\"%[^\"]", phoneNum) > 0) {
                    inCalling = true;
                    lv_incomming();
                }
            }
        } else if (strstr(s, "RING") != NULL) {
            Serial.println("SIM RING");
        } else if (strstr(s, "Call Ready") != NULL) {
            Serial.println("SIM Call Ready");
        } else if (strstr(s, "SMS Ready") != NULL) {
            Serial.println("SIM SMS Ready");
        } else if (strstr(s, "NO CARRIER") != NULL) {  
            Serial.println("SIM NO CARRIER");
            if (inCalling) {
                inCalling = false;
                lv_call_menu();
            }
        } else if (strstr(s, "NO DIALTONE") != NULL) {
            Serial.println("SIM NO DIALTONE");
            if(inCall){
                inCall = false;
                lv_call_menu();
            }
        } else if (strstr(s, "BUSY") != NULL) {
            Serial.println("SIM BUSY");
            if(inCall){
                inCall = false;
                lv_call_menu();
            }
        } else {
            Serial.print(s);
        }
        Serial.println("==========");
    }


    if (Serial.available()) {
        String r = Serial.readString();
        hwSerial.write(r.c_str());
    }

    if (callout) {
        callout = false;
        gprs.callUp(phoneNum);
        calling();
    }
}



static lv_res_t call_btnm_action(lv_obj_t *btnm, const char *txt)
{
    lv_kb_ext_t *ext = (lv_kb_ext_t *)lv_obj_get_ext_attr(keyboard);
    if (strcmp(txt, SYMBOL_CALL) == 0) {
        const char *num = lv_ta_get_text(ext->ta);
        printf("call %s\n", num);
        strcpy(phoneNum, num);
        callout = true;
    }  else if (strcmp(txt, "Del") == 0)
        lv_ta_del_char(ext->ta);
    else {
        lv_ta_add_text(ext->ta, txt);
    }
    return LV_RES_OK;
}

void lv_call_menu()
{
    static const char *btnm_map[] = {"1", "2", "3", "4", "5", "\n",
                                     "6", "7", "8", "9", "0", "\n",
                                     SYMBOL_CALL, "Del", ""
                                    };
    lv_obj_clean(gContainer);

    phonenum = lv_ta_create(gContainer, NULL);
    lv_obj_set_height(phonenum, 35);
    lv_ta_set_one_line(phonenum, true);
    lv_ta_set_pwd_mode(phonenum, false);
    lv_ta_set_text(phonenum, "");
    lv_obj_align(phonenum, gContainer, LV_ALIGN_IN_TOP_MID, 0, 20);

    keyboard = lv_kb_create(gContainer, NULL);
    lv_kb_set_map(keyboard, btnm_map);
    lv_obj_set_height(keyboard, 150);
    lv_obj_align(keyboard, gContainer, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_kb_set_ta(keyboard, phonenum);
    lv_btnm_set_action(keyboard, call_btnm_action);
}



lv_res_t incommingCallBtnCallback (lv_obj_t *obj)
{
    if (obj == ecallBtn) {
        Serial.println("ecallBtn pressed");
        hwSerial.print("ATH\r\n");
        inCalling = false;
        if (isCalling) {
            isCalling = false;
        }
        lv_call_menu();
    } else if (obj == scallBtn) {
        Serial.println("scallBtn pressed");
        hwSerial.print("ATA\r\n");
    }
}

lv_obj_t *commingNum = NULL;
lv_obj_t *callingTimer = NULL;

void lv_incomming()
{
    lv_obj_clean(gContainer);

    commingNum = lv_label_create(gContainer, NULL);
    lv_label_set_text(commingNum, phoneNum);
    lv_obj_align(commingNum, NULL, LV_ALIGN_IN_TOP_MID, 0, 100);

    static lv_style_t style_pr;
    lv_style_copy(&style_pr, &lv_style_plain);
    style_pr.image.color = LV_COLOR_BLACK;
    style_pr.image.intense = LV_OPA_50;
    style_pr.text.color = LV_COLOR_HEX3(0xaaa);

    scallBtn = lv_imgbtn_create(gContainer, NULL);
    lv_imgbtn_set_src(scallBtn, LV_BTN_STATE_REL, &img_scall);
    lv_imgbtn_set_src(scallBtn, LV_BTN_STATE_PR, &img_scall);
    lv_imgbtn_set_src(scallBtn, LV_BTN_STATE_TGL_REL, &img_scall);
    lv_imgbtn_set_src(scallBtn, LV_BTN_STATE_TGL_PR, &img_scall);
    lv_imgbtn_set_style(scallBtn, LV_BTN_STATE_PR, &style_pr);        /*Use the darker style in the pressed state*/
    lv_imgbtn_set_style(scallBtn, LV_BTN_STATE_TGL_PR, &style_pr);
    lv_imgbtn_set_toggle(scallBtn, true);
    lv_obj_align(scallBtn, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 50, -50);


    ecallBtn = lv_imgbtn_create(gContainer, NULL);
    lv_imgbtn_set_src(ecallBtn, LV_BTN_STATE_REL, &img_ecall);
    lv_imgbtn_set_src(ecallBtn, LV_BTN_STATE_PR, &img_ecall);
    lv_imgbtn_set_src(ecallBtn, LV_BTN_STATE_TGL_REL, &img_ecall);
    lv_imgbtn_set_src(ecallBtn, LV_BTN_STATE_TGL_PR, &img_ecall);
    lv_imgbtn_set_style(ecallBtn, LV_BTN_STATE_PR, &style_pr);        /*Use the darker style in the pressed state*/
    lv_imgbtn_set_style(ecallBtn, LV_BTN_STATE_TGL_PR, &style_pr);
    lv_imgbtn_set_toggle(ecallBtn, true);
    lv_obj_align(ecallBtn, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -50, -50);

    lv_imgbtn_set_action(ecallBtn, LV_BTN_ACTION_PR, incommingCallBtnCallback);
    lv_imgbtn_set_action(scallBtn, LV_BTN_ACTION_PR, incommingCallBtnCallback);
}


void calling()
{
    inCall = true;
    isCalling = true;
    lv_obj_clean(gContainer);

    commingNum = lv_label_create(gContainer, NULL);
    lv_label_set_text(commingNum, phoneNum);
    lv_obj_align(commingNum, NULL, LV_ALIGN_IN_TOP_MID, 0, 80);

    static lv_style_t style_pr;
    lv_style_copy(&style_pr, &lv_style_plain);

    style_pr.image.color = LV_COLOR_BLACK;
    style_pr.image.intense = LV_OPA_50;
    style_pr.text.color = LV_COLOR_HEX3(0xaaa);

    ecallBtn = lv_imgbtn_create(gContainer, NULL);
    lv_imgbtn_set_src(ecallBtn, LV_BTN_STATE_REL, &img_ecall);
    lv_imgbtn_set_src(ecallBtn, LV_BTN_STATE_PR, &img_ecall);
    lv_imgbtn_set_src(ecallBtn, LV_BTN_STATE_TGL_REL, &img_ecall);
    lv_imgbtn_set_src(ecallBtn, LV_BTN_STATE_TGL_PR, &img_ecall);
    lv_imgbtn_set_style(ecallBtn, LV_BTN_STATE_PR, &style_pr);        /*Use the darker style in the pressed state*/
    lv_imgbtn_set_style(ecallBtn, LV_BTN_STATE_TGL_PR, &style_pr);
    lv_imgbtn_set_toggle(ecallBtn, true);
    lv_obj_align(ecallBtn, NULL, LV_ALIGN_CENTER, 0, 80);

    lv_imgbtn_set_action(ecallBtn, LV_BTN_ACTION_PR, incommingCallBtnCallback);
}
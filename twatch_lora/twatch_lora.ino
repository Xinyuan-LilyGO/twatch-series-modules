#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "board_def.h"
#include <lvgl.h>
#include "axp20x.h"
#include <FT5206.h>
#include <Ticker.h>
#include <Button2.h>
#include <LoRa.h>

TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
Ticker btnTicker;
AXP20X_Class axp;
Button2 btn(36);
SPIClass sSPI(HSPI);

uint32_t state = 0, prev_state = 0;
LV_IMG_DECLARE(img_check);

lv_obj_t *gContainer = NULL;
lv_obj_t *ta1 = NULL;
static String recv = "";

void createWin();


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

static lv_res_t lv_btm_action ( lv_obj_t *obj)
{
    uint32_t n = lv_obj_get_free_num(obj);
    state = n;
    if (!ta1) {
        createWin();
    }
}

static void createGui()
{
    lv_obj_clean(gContainer);

    ta1 = NULL;
    lv_obj_t *label ;
    /*Create a normal button*/
    lv_obj_t *btn1 = lv_btn_create(gContainer, NULL);
    // lv_cont_set_fit(btn1, true, true); /*Enable resizing horizontally and vertically*/
    lv_obj_set_size(btn1, 120, 65);
    lv_obj_align(btn1, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
    lv_obj_set_free_num(btn1, 1);   /*Set a unique number for the button*/

    lv_btn_set_action(btn1, LV_BTN_ACTION_CLICK, lv_btm_action);

    /*Add a label to the button*/
    label = lv_label_create(btn1, NULL);
    lv_label_set_text(label, "Sender");

    /*Copy the button and set toggled state. (The release action is copied too)*/
    lv_obj_t *btn2 = lv_btn_create(gContainer, btn1);
    lv_obj_align(btn2, btn1, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_free_num(btn2, 2);               /*Set a unique number for the button*/

    /*Add a label to the toggled button*/
    label = lv_label_create(btn2, NULL);
    lv_label_set_text(label, "Receiver");

}

void createWin()
{
    lv_obj_clean(gContainer);
    ta1 = lv_ta_create(gContainer, NULL);
    lv_obj_set_size(ta1, LV_HOR_RES, LV_VER_RES);
    lv_obj_align(ta1, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_ta_set_style(ta1, LV_TA_STYLE_BG, &lv_style_transp_fit);                    /*Apply the scroll bar style*/
    lv_ta_set_cursor_type(ta1, LV_CURSOR_NONE);
    lv_ta_set_text(ta1, "");    /*Set an initial text*/
    lv_ta_set_max_length(ta1, 128);
}

void add_message(const char *txt)
{
    if (!txt || !ta1)return;
    if (lv_txt_get_encoded_length(lv_ta_get_text(ta1)) >= lv_ta_get_max_length(ta1)) {
        lv_ta_set_text(ta1, "");
    }
    lv_ta_add_text(ta1, txt);
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

    lv_obj_t *img = lv_img_create(gContainer, NULL);
    lv_img_set_src(img, &img_check);
    lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *label = lv_label_create(gContainer, NULL);
    lv_label_set_text(label, "Begin Lora Module");
    lv_obj_align(label, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


    //! Lora power enable
    axp.setLDO3Mode(1);
    axp.setPowerOutPut(AXP202_LDO3, AXP202_ON);

    sSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
    LoRa.setSPI(sSPI);
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);

    while (1) {
        if (!LoRa.begin(BAND))
            Serial.println("LORA Begin FAIL");
        else {
            Serial.println("LORA Begin PASS");
            break;
        }
        delay(1000);
    }

    createGui();

    btn.setPressedHandler([](Button2 & b) {
        state = 0;
        createGui();
    });

    btnTicker.attach_ms(30, []() {
        btn.loop();
    });

}

char buf[256];

void lora_sender()
{
    static uint32_t sendCount = 0;
    LoRa.beginPacket();
    LoRa.print("lora: ");
    LoRa.print(sendCount);
    LoRa.endPacket();
    ++sendCount;
    snprintf(buf, sizeof(buf), "Send %lu\n", sendCount);
    Serial.println(buf);
    add_message(buf);
}

void lora_receiver()
{
    if (LoRa.parsePacket()) {
        recv = "";
        while (LoRa.available()) {
            recv += (char)LoRa.read();
        }
        snprintf(buf, sizeof(buf), "Received:%s - rssi:%d\n", recv.c_str(), LoRa.packetRssi());
        Serial.println(buf);
        add_message(buf);
    }
}


void loop()
{
    switch (state) {
    case 1:
        lora_sender();
        delay(1000);
        break;
    case 2:
        lora_receiver();
        break;
    default:
        break;
    }
}







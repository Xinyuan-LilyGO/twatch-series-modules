#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "board_def.h"
#include <lvgl.h>
#include "axp20x.h"
#include <FT5206.h>
#include <Ticker.h>
#include <Button2.h>
#include <s7xg.h>

TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
Ticker btnTicker;
AXP20X_Class axp;
S7XG_Class s7xg;
Button2 btn(36);

uint32_t state = 0, prev_state = 0;
bool isInit = false;
int16_t w = 0;
LV_IMG_DECLARE(img_check);

lv_obj_t *gContainer = NULL;
lv_obj_t *ta1 = NULL;
void createWin();
void stop_prev();

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
    // Serial.printf("Numn:%u\n", n);
    isInit = false;
    state = n;
    if (!ta1) {
        createWin();
    }
}

static void  createGui()
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

    /*Copy the button and set inactive state.*/
    lv_obj_t *btn3 = lv_btn_create(gContainer, btn1);
    lv_obj_align(btn3, btn2, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_free_num(btn3, 3);               /*Set a unique number for the button*/

    /*Add a label to the inactive button*/
    label = lv_label_create(btn3, NULL);
    lv_label_set_text(label, "GPS");
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

    axp.setLDO4Voltage(AXP202_LDO4_1800MV);
    axp.setPowerOutPut(AXP202_LDO4, AXP202_ON);
    Serial1.begin(115200, SERIAL_8N1, UART_RX, UART_TX );
    s7xg.begin(Serial1);
    s7xg.reset();
    delay(1000);

    gContainer = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_set_size(gContainer,  LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style(gContainer, &lv_style_transp_fit);

    lv_obj_t *img = lv_img_create(gContainer, NULL);
    lv_img_set_src(img, &img_check);
    lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *label = lv_label_create(gContainer, NULL);
    lv_label_set_text(label, "Begin S7xG");
    lv_obj_align(label, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


    while (1) {
        Serial1.print("sip get_hw_model");
        String model = "";
        if (Serial1.available()) {
            model = Serial1.readStringUntil('\r');
        }
        Serial.println(model);
        model.replace("\r","");model.replace("\n","");model.replace(">","");model.replace(" ","");
        if ( model == "S76G" || model == "S78G") {
            break;
        }
        delay(1000);
    }
    Serial.println(s7xg.getVersion());


    createGui();


    btn.setPressedHandler([](Button2 & b) {
        isInit = false;
        state = 0;
        stop_prev();
        createGui();
    });

    btnTicker.attach_ms(30, []() {
        btn.loop();
    });

}

void stop_prev()
{
    switch (prev_state) {
    case 1:
    case 2:
        Serial.println("Stop Lora");
        s7xg.loraPingPongStop();
        break;
    case 3:
        Serial.println("Stop GPS");
        s7xg.gpsStop();
        break;
    default:
        break;
    }
    prev_state = state;
}

void lora_sender()
{
    if (!isInit) {
        isInit = true;
        stop_prev();
        s7xg.loraSetPingPongSender();
        Serial.println("Start  lora_sender");
    }
    String str = s7xg.getMessage();
    if (str != "") {
        add_message(str.c_str());
    }
}

void lora_receiver()
{
    if (!isInit) {
        isInit = true;
        stop_prev();
        s7xg.loraSetPingPongReceiver();
        Serial.println("Start  lora_receiver");
    }
    String str = s7xg.getMessage();
    if (str != "") {
        add_message(str.c_str());
    }
}

void s7xg_gps()
{
    if (!isInit) {
        isInit = true;
        stop_prev();
        s7xg.gpsReset();
        s7xg.gpsSetLevelShift(true);
        s7xg.gpsSetStart();
        s7xg.gpsSetSystem(0);
        s7xg.gpsSetPositioningCycle(1000);
        s7xg.gpsSetPortUplink(20);
        s7xg.gpsSetFormatUplink(1);
        s7xg.gpsSetMode(1);
        Serial.println("Start  s7xg_gps");
    }
    s7xg.gpsGet();
    while (Serial1.available()) {
        String str = Serial1.readStringUntil('\r');
        add_message(str.c_str());
    }
}


void loop()
{
    switch (state) {
    case 1:
        lora_sender();
        break;
    case 2:
        lora_receiver();
        break;
    case 3:
        s7xg_gps();
        break;
    default:
        break;
    }
    delay(1000);
}







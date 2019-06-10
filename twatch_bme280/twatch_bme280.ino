#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "board_def.h"
#include <lvgl.h>
#include "axp20x.h"
#include <FT5206.h>
#include <Ticker.h>
#include <Adafruit_BME280.h>

TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
AXP20X_Class axp;

LV_IMG_DECLARE(img_bg);
LV_IMG_DECLARE(img_err);

lv_obj_t *gContainer = NULL;
lv_obj_t *pressure = NULL;
lv_obj_t *temperature = NULL;
lv_obj_t *humidity = NULL;

#define BEM280_ADDRESS 0X77
Adafruit_BME280 bme;

static void bme280_task(void *param);

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
        ret = bme.begin(BEM280_ADDRESS);
        if (!ret && !err) {
            lv_obj_t *img = lv_img_create(gContainer, NULL);
            lv_img_set_src(img, &img_err);
            lv_obj_align(img, NULL, LV_ALIGN_CENTER, 0, 0);
            err = lv_label_create(gContainer, NULL);
            lv_label_set_text(err, "Could not find BME280");
            lv_obj_align(err, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        }
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        delay(1000);
    } while (!ret);

    if (err) {
        lv_obj_clean(gContainer);
    }

    lv_obj_t *mountain = lv_img_create(gContainer, NULL);
    lv_img_set_src(mountain, &img_bg);
    lv_obj_align(mountain, NULL, LV_ALIGN_CENTER, 0, 0);

    pressure = lv_label_create(gContainer, NULL);
    temperature = lv_label_create(gContainer, NULL);
    humidity = lv_label_create(gContainer, NULL);

    lv_task_create(bme280_task, 1000, LV_TASK_PRIO_LOW, NULL);
}

static void bme280_task(void *param)
{
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f hPa", bme.readPressure() / 100.0F);
    lv_label_set_text(pressure, buffer);
    lv_obj_align(pressure, NULL, LV_ALIGN_IN_BOTTOM_MID, 40, -15);

    snprintf(buffer, sizeof(buffer), "%.2f *C", bme.readTemperature());
    lv_label_set_text(temperature, buffer);
    lv_obj_align(temperature, NULL, LV_ALIGN_IN_TOP_MID, 30, 15);

    snprintf(buffer, sizeof(buffer), "%.2f %%", bme.readHumidity());
    lv_label_set_text(humidity, buffer);
    lv_obj_align(humidity, NULL, LV_ALIGN_CENTER, 30, 0);
}

void loop()
{
    delay(10000);
}







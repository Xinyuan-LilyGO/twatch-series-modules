#include <Arduino.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "board_def.h"
#include <lvgl.h>
#include "axp20x.h"
#include <FT5206.h>
#include <Ticker.h>
#include <MAX30105.h>
#include "heartRate.h"

TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
AXP20X_Class axp;

LV_IMG_DECLARE(img_heart);
LV_IMG_DECLARE(img_err);
lv_obj_t *gContainer = NULL;
lv_obj_t *heart = NULL;
lv_obj_t *ir = NULL;
lv_obj_t *bpm = NULL;
lv_obj_t *avg = NULL;

MAX30105 particleSensor;
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;
int y;

static void heartrate_task(void *param);
void heart_anim_stop();
void heart_anim_start();

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

    tft->fillScreen(TFT_WHITE);

    gContainer = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_set_size(gContainer,  LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style(gContainer, &lv_style_transp_fit);

    lv_obj_t *err = NULL;
    bool ret = false;
    do {
        ret = particleSensor.begin();
        if (!ret && !err) {
            tft->setTextFont(2);
            tft->setTextDatum(MC_DATUM);
            tft->setTextColor(TFT_BLACK,TFT_WHITE);
            tft->drawString("Could not find sensor", tft->width() / 2, tft->height() / 2);
            Serial.println("Could not find a valid heartrate sensor, check wiring!");
        }
        delay(1000);
    } while (!ret);

    if (err) {
        lv_obj_clean(gContainer);
    }

    heart = lv_img_create(gContainer, NULL);
    lv_img_set_src(heart, &img_heart);
    lv_obj_align(heart, NULL, LV_ALIGN_CENTER, 0, -10);

    y = lv_obj_get_y(heart) + 10;

    bpm = lv_label_create(gContainer, NULL);
    lv_label_set_text(bpm, "Place your");
    lv_obj_align(bpm, NULL, LV_ALIGN_CENTER, 0, y);

    avg = lv_label_create(gContainer, NULL);
    lv_label_set_text(avg, "finger on the sensor");
    lv_obj_align(avg, bpm, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    particleSensor.setup(); //Configure sensor. Use 6.4mA for LED drive
    particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
    particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}

char buffer[256];
uint64_t startMillis = 0;
bool anim_start = false;

void loop()
{
    long irValue = particleSensor.getIR();

    if (checkForBeat(irValue) == true) {
        //We sensed a beat!
        long delta = millis() - lastBeat;
        lastBeat = millis();
        beatsPerMinute = 60 / (delta / 1000.0);
        if (beatsPerMinute < 255 && beatsPerMinute > 20) {
            rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
            rateSpot %= RATE_SIZE; //Wrap variable
            //Take average of readings
            beatAvg = 0;
            for (byte x = 0 ; x < RATE_SIZE ; x++)
                beatAvg += rates[x];
            beatAvg /= RATE_SIZE;
        }
    }
    if (irValue < 50000) {
        beatsPerMinute =  0;
        beatAvg  = 0;
        if (anim_start) {
            anim_start = false;
            heart_anim_stop();
            lv_label_set_text(bpm, "Place your");
            lv_label_set_text(avg, "finger on the sensor");
            lv_obj_align(bpm, NULL, LV_ALIGN_CENTER, 0, y);
            lv_obj_align(avg, bpm, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
        }
        Serial.println(" No finger?");
    } else {
        if (!anim_start) {
            anim_start = true;
            heart_anim_start();
        }
        Serial.printf("IR:%u - bpm:%.2f avg:%d\n", irValue, beatsPerMinute, beatAvg);
        snprintf(buffer, sizeof(buffer), "BPM:%.2f", beatAvg);
        lv_label_set_text(bpm, buffer);
        lv_obj_align(bpm, NULL, LV_ALIGN_CENTER, 0, y);

        snprintf(buffer, sizeof(buffer), "AVG BPM:%d", beatAvg);
        lv_label_set_text(avg, buffer);
        lv_obj_align(avg, bpm, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    }

    if (millis() - startMillis > 5) {
        startMillis = millis();
        lv_task_handler();
    }
}


void heart_anim_start()
{
    static lv_anim_t a;
    a.var = heart;
    a.start = lv_obj_get_y(heart);
    a.end = lv_obj_get_y(heart) - 10;
    a.fp = (lv_anim_fp_t)lv_obj_set_y;
    a.path = lv_anim_path_linear;
    a.end_cb = NULL;
    a.act_time = -1000;                         /*Negative number to set a delay*/
    a.time = 400;                               /*Animate in 400 ms*/
    a.playback = 1;                             /*Make the animation backward too when it's ready*/
    a.playback_pause = 0;                       /*Wait before playback*/
    a.repeat = 1;                               /*Repeat the animation*/
    a.repeat_pause = 100;                       /*Wait before repeat*/
    lv_anim_create(&a);
}

void heart_anim_stop()
{
    lv_anim_del(heart, NULL);
}


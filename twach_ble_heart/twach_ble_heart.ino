#include "BLEDevice.h"
#include <Button2.h>
#include <Wire.h>
#include <TFT_eSPI.h>
#include "board_def.h"
#include <lvgl.h>
#include "axp20x.h"
#include <FT5206.h>
#include <Ticker.h>

#define HEART_RATE_SERVICE_UUID                 "180D"
#define HEART_RATE_CHARACTERISTIC_UUID          "2A37"

#define BATTERY_SERVICE_UUID                    "180F"
#define BATTERY_CHARACTERISTIC_UUID             "2A19"
#define BLE_ADDRESS                             "F5:55:F4:88:44:B0"


static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;
static BLERemoteDescriptor *pRemoteSensorDescriptor ;
static BLERemoteCharacteristic *pRemoteSensorCharacteristic;

uint8_t ledstatus;
TFT_eSPI *tft = nullptr;
FT5206_Class *tp = nullptr;
BLEScan *pBLEScan = nullptr;
Ticker lvTicker1;
Ticker lvTicker2;
AXP20X_Class axp;
// Button2 btn(0);
// Ticker btnTicker;

LV_IMG_DECLARE(imgBg);
LV_IMG_DECLARE(img_heart);
LV_IMG_DECLARE(img_err);
LV_IMG_DECLARE(imgLoudly);
LV_IMG_DECLARE(imgSmiling);
LV_IMG_DECLARE(imRolling);

lv_obj_t *gContainer = NULL;
lv_obj_t *heart = NULL;
uint64_t startMillis = 0;
lv_obj_t *text = NULL;
lv_obj_t *preload = NULL;
int bpm = 0;
int battery = 0;

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

static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
    bpm = pData[1];
    // for (int i = 0; i < length; i++ ) {
    //     Serial.print(pData[i]); Serial.print(" ");
    // }
    Serial.println();
}

class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
    }

    void onDisconnect(BLEClient *pclient)
    {
        connected = false;
        Serial.println("onDisconnect");
    }
};

bool connectToServer()
{
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient  *pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");


    BLERemoteService *pRemoteSensorService = pClient->getService(HEART_RATE_SERVICE_UUID);
    if (pRemoteSensorService == nullptr) {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(HEART_RATE_SERVICE_UUID);
    } else {
        Serial.print(" - Found our service");
        Serial.println(HEART_RATE_SERVICE_UUID);
        pRemoteSensorCharacteristic = pRemoteSensorService->getCharacteristic(HEART_RATE_CHARACTERISTIC_UUID);
        if (pRemoteSensorCharacteristic->canNotify())
            pRemoteSensorCharacteristic->registerForNotify(notifyCallback);

        pRemoteSensorDescriptor =  pRemoteSensorCharacteristic->getDescriptor(BLEUUID("2902"));
        if (pRemoteSensorDescriptor != nullptr) {
            Serial.println(" - Found our pRemoteSensorDescriptor");
            pRemoteSensorDescriptor->writeValue(1);
        }
    }
    connected = true;
}

void create_heart_ui()
{
    lv_obj_clean(gContainer);

    lv_obj_t *wp = lv_img_create(gContainer, NULL);
    lv_obj_set_protect(wp, LV_PROTECT_PARENT);          /*Don't let to move the wallpaper by the layout */
    lv_img_set_src(wp, &imgBg);
    lv_obj_set_size(wp, LV_HOR_RES, LV_VER_RES);

    heart = lv_img_create(gContainer, NULL);
    lv_img_set_src(heart, &img_heart);
    lv_obj_align(heart, NULL, LV_ALIGN_CENTER, 0, -10);

    text = lv_label_create(gContainer, NULL);
    lv_label_set_text(text, "BPM:0");
    lv_obj_align(text, heart, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    static lv_style_t style1;
    lv_style_copy(&style1, &lv_style_plain);
    style1.text.color = LV_COLOR_WHITE;
    lv_label_set_style(text, &style1);
}

void create_preload()
{
    lv_obj_clean(gContainer);

    lv_obj_t *wp = lv_img_create(gContainer, NULL);
    lv_obj_set_protect(wp, LV_PROTECT_PARENT);          /*Don't let to move the wallpaper by the layout */
    lv_img_set_src(wp, &imgBg);
    lv_obj_set_size(wp, LV_HOR_RES, LV_VER_RES);

    /*Create a style for the Preloader*/
    static lv_style_t style;
    lv_style_copy(&style, &lv_style_plain);
    style.line.width = 10;                         /*10 px thick arc*/
    style.line.color = LV_COLOR_GREEN;       /*Blueish arc color*/

    style.body.border.color = LV_COLOR_HEX3(0xBBB); /*Gray background color*/
    style.body.border.width = 10;
    style.body.padding.hor = 0;

    /*Create a Preloader object*/
    preload = lv_preload_create(gContainer, NULL);
    lv_obj_set_size(preload, 100, 100);
    lv_obj_align(preload, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_preload_set_style(preload, LV_PRELOAD_STYLE_MAIN, &style);


    text = lv_label_create(gContainer, NULL);
    lv_label_set_text(text, "Scanning");
    lv_obj_align(text, preload, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
}

void scanCompleteCB(BLEScanResults result)
{
    Serial.printf(">> Dump scan results:");
    for (int i = 0; i < result.getCount(); i++) {
        Serial.printf( "- %s\n", result.getDevice(i).toString().c_str());
        if (result.getDevice(i).getAddress().equals(BLEAddress(BLE_ADDRESS))) {
            pBLEScan->stop();
            myDevice = new BLEAdvertisedDevice(result.getDevice(i));
            doConnect = true;
            doScan = true;
            return;
        }
    }
    pBLEScan->start(5, scanCompleteCB);
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

    create_preload();

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, scanCompleteCB);
    Serial.println("Start loop");

    // btn.setClickHandler([](Button2 & b) {
    //     if (connected) {
    //         ledstatus = !ledstatus;
    //         pRemoteCharacteristic->writeValue(ledstatus);
    //     }
    // });

    // btnTicker.attach_ms(30, []() {
    //     btn.loop();
    // });

} // End of setup.


// This is the Arduino main loop function.
void loop()
{
    if (doConnect == true) {
        lv_label_set_text(text, "Connect...");
        lv_obj_align(text, preload, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        if (connectToServer()) {
            create_heart_ui();
            Serial.println("We are now connected to the BLE Server.");
        } else {
            Serial.println("We have failed to connect to the server; there is nothin more we will do.");
        }
        doConnect = false;
    }
    if (connected) {
        char buff[64];
        snprintf(buff, sizeof(buff), "BPM:%d", bpm);
        lv_label_set_text(text, buff);
        lv_obj_align(text, heart, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    } else if (doScan) {
        Serial.println("Keep Scan Device");
        create_preload();
        doScan = false;
        pBLEScan->start(5, scanCompleteCB);
    }
    if (millis() - startMillis > 5) {
        startMillis = millis();
        lv_task_handler();
    }
}

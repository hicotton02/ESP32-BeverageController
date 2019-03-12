#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <Adafruit_ADS1015.h>  //https://github.com/soligen2010/Adafruit_ADS1X15
#include <TimeLib.h>
#include "Nextion.h"
#include <PID_v1.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AWS_IOT.h>
#include <WiFi.h>
#include <HardwareSerial.h>
#include <WebServer.h> //https://github.com/zhouhan0126/DNSServer---esp32
#include <DNSServer.h> //https://github.com/zhouhan0126/DNSServer---esp32
#include <WiFiManager.h>   //https://github.com/zhouhan0126/DNSServer---esp32
#include "time.h"

//pinout
#define USB_TX 1
#define USB_RX 3
#define TEMPA_BUS 4 //Onewire1
#define TEMPC_BUS 5 //onewire2
#define ELEMENT2_SSR_RELAY 13 //Boiler 
#define ELEMENT1_SSR_RELAY 14 //HLT or Still
#define PUMP1_RELAY 15 //Main pump or recirculating pump for still
#define DEBUG_TX 16
#define DEBUG_RX 17
#define ELEMENT1_CONTACTOR_RELAY 18 //HLT or Still Boiler Element 2
#define ERROR_RELAY 19 //Error LED
#define SDA 21
#define SCL 22
#define TEMPB_BUS 23 //Onewire3
#define ELEMENT2_CONTACTOR_RELAY 25 //Brew Boiler or Still Boiler Element 1
#define DISPLAY_TX 26
#define DISPLAY_RX 27
#define PUMP2_RELAY 33 //Herms Pump


/*
   WiFi working
   External Ip call working
   barometric pressure call working
   OneWire Working
   Ammeter not working

   TempA = HLT/Column
   TempB = Mlt/Recirculating Water
   TempC = Boiler

*/
uint64_t chipId;
typedef struct
{
  char ssid[26] = "xxxx";
  char pass[26] = "xxxxxxxxx";
  unsigned int hermsEnabled = 1;
  unsigned int enableDistillPump = 1;
  char timeZone[4] = "-8";
} CONFIG_T;

CONFIG_T configuration;
enum machineStates
{
  HOME,
  AUTO_BREW,
  MANUAL_BREW,
  AUTO_DISTILL,
  MANUAL_DISTILL,
  SETTINGS
};
enum machineStates state;
//Temperature Variables

#define TEMP_PRECISION 9
OneWire oneWireA(TEMPA_BUS);
OneWire oneWireB(TEMPB_BUS);
OneWire oneWireC(TEMPC_BUS);
DallasTemperature sensorA(&oneWireA);
DallasTemperature sensorB(&oneWireB);
DallasTemperature sensorC(&oneWireC);
int actualSensorBTemp = 0;
int actualSensorCTemp = 0;
int actualSensorATemp = 0;
int targetMltTemp = 0;
int targetBoilTemp = 0;
int targetHltTemp = 0;
int targetStrikeTemp = 0;

//Ammeter
char ampDisplayLeg1[5];
char ampDisplayLeg2[5];
Adafruit_ADS1015 adsLeg1(0x48);
Adafruit_ADS1015 adsLeg2(0x49);
float leg1;
float leg2;
int maxValueLeg1 = 0;
int minValueLeg1 = 0;
int maxValueLeg2 = 0;
int minValueLeg2 = 0;
float multiplier = 0.036;

//PIDs
double element1Input, element1Setpoint, element1Output, element2Input, element2Setpoint, element2Output;
double kp = 2, ki = 5, kd = 1;
PID element1Pid(&element1Input, &element1Output, &element1Setpoint, kp, ki, kd, DIRECT);
PID element2Pid(&element2Input, &element2Output, &element2Setpoint, kp, ki, kd, DIRECT);
int pidWindowSize = 1000;

#define BAUD_RATE 115200
TaskHandle_t NTPTaskHandle;
TaskHandle_t UpdateDisplayTaskHandle;
TaskHandle_t ProcessDisplayTaskHandle;
TaskHandle_t BrewTaskHandle;
TaskHandle_t DistillTaskHandle;
TaskHandle_t AmmeterTaskHandle;
TaskHandle_t OneWireTaskHandle;
TaskHandle_t WiFiServerTaskHandle;
TaskHandle_t RemoteReceiveTaskHandle;
TaskHandle_t RemoteSendTaskHandle;
TaskHandle_t wiFiTaskHandle;
TickType_t oneWireDelay = pdMS_TO_TICKS(750); // 0.75 seconds
TickType_t wifiTaskDelay = pdMS_TO_TICKS(3600000); //1 hour
TickType_t wifiConnectDelay = pdMS_TO_TICKS(500); //0.5 seconds
TickType_t adsTaskDelay = pdMS_TO_TICKS(50); // 0.75 seconds

//WiFi
bool awsEnable = true;
AWS_IOT awsClient;
char awsHostAddress[] = "xxxxxxxxxxxxxx.amazonaws.com";
char externalIpServiceUrl[] = "api.ipify.org";
char ntpServerName[] = "us.pool.ntp.org";
char wwoServerUrl[] = "http://api.worldweatheronline.com/premium/v1/weather.ashx?key=";
char awsClientId[] = "BDCDev01";
char awsSubscribeTopic[] = "$aws/things/ESP32BrewController/shadow/update/accepted"; //Change ESP32BrewController to the Chip ID
char publishTopic[] = "$aws/things/ESP32BrewController/shadow/update/";
char wwoKey[] = "d54be7730f504424bbc231440192901";
float currentPressure = 0; //in mB
int timeZone;
struct tm timeinfo;
int timeZoneOffsetS;
WiFiServer server(80);
char localIp[16];
char externalIp[16];
char rcvdPayload[512];
char msgReceived = 0;
int serviceNum = 0;
char ser[20], serTag[20], serVal[20];


//Distilling Variables
static int maxCondenserCoolantTempF = 145;
double boilerPowerPercentage = 1.0; //1.0 = 100%; 0.25 = 25%
unsigned long lastDistillDutyCycle = 0;
static int distillDutyCycle = 1000; //1 second
float vaporAbv;
float boilerAbv;
int distillDutyTime = 1000;
bool stillRunning = false;
bool autoDistill = false;

//Brewing Varriables
bool boilElementStatus = false;
bool hltElementStatus = false;
bool mainPumpStatus = false;
bool hermsPumpStatus = false;
bool boilTimerOn = false;
bool mashTimerOn = false;
bool brewSessionEnabled = false;
bool autoBrew = false;
bool brewTimerStarted = false;
int autoBrewMashSeconds = 0;
int autoBrewBoilSeconds = 0;
int boilTimeSeconds = 0;

bool contactor1Status = false;
bool contactor2Status = false;
bool waitingOnUserResponse = false;
bool goToNextStep = false;

//Timing Variables
unsigned long hltPidWindowStartTime;
unsigned long boilPidWindowStartTime;

unsigned int rtcTime[7] = {0};
unsigned int targetTimeInMinutes = 0;
unsigned int targetTimeInSeconds = 0;

//Display Variables
static bool displayEnabled = true;
int pageDisplayed = 0;
bool manualHltButtonUpPressed = false;
bool manualHltButtonDownPressed = false;
bool manualMltButtonUpPressed = false;
bool manualMltButtonDownPressed = false;
bool manualBoilButtonUpPressed = false;
bool manualBoilButtonDownPressed = false;
bool manualTimeButtonUpPressed = false;
bool manualTimeButtonDownPressed = false;
NexRtc rtc;
//Lock Page
NexPage lockPage = NexPage(0, 0, "Lock");

//Select Page
NexPage selectPage = NexPage(1, 0, "Select");
NexButton autoBrewButton = NexButton(1, 1, "bAutoBrew");
NexButton manualBrewButton = NexButton(1, 2, "bManualBrew");
NexButton autoDistillButton = NexButton(1, 3, "bAutoDistill");
NexButton manualDistillButton = NexButton(1, 4, "bManualDistill");
NexButton settingsButton = NexButton(1, 5, "bSettings");

//BrewAuto Page
NexPage autoBrewPage = NexPage(2, 0, "BrewAuto");
NexDSButton autoBrewHLTButton = NexDSButton(2, 7, "btHlt");
NexDSButton autoBrewBoilButton = NexDSButton(2, 8, "btBoil");
NexDSButton autoBrewMainPumpButton = NexDSButton(2, 42, "btMain");
NexDSButton autoBrewHermsPumpButton = NexDSButton(2, 41, "btHerms");
NexButton autoBrewNextButton = NexButton(2, 1, "bNext");
NexText autoBrewHltActual = NexText(2, 29, "aHlt");
NexText autoBrewHltTarget = NexText(2, 26, "tHlt");
NexText autoBrewMltActual = NexText(2, 30, "aMlt");
NexText autoBrewMltTarget = NexText(2, 38, "tMlt");
NexText autoBrewBoilActual = NexText(2, 31, "aBoil");
NexText autoBrewBoilTarget = NexText(2, 27, "tBoil");
NexText autoBrewIpAddress = NexText(2, 18, "BrewAuto.tIp");
NexText autoBrewSsid = NexText(2, 20, "BrewAuto.tSsid");
NexText autoBrewAmpsLeg1 = NexText(2, 41, "tLeg1Amps");
NexText autoBrewAmpsLeg2 = NexText(2, 43, "tLeg2Amps");
NexNumber autoBrewHours = NexNumber(2, 12, "nHour");
NexNumber autoBrewMinutes = NexNumber(2, 15, "nMin");
NexNumber autoBrewSeconds = NexNumber(2, 16, "nSec");
NexButton autoBrewHomeButton = NexButton(2, 33, "bHome");
NexVariable autoBrewStatusMessage = NexVariable(2, 34, "BrewAuto.vStatusMsg");

//BrewManual Page
NexPage manualBrewPage = NexPage(3, 0, "BrewManual");
NexDSButton manualBrewHLTButton = NexDSButton(3, 1, "btHlt");
NexDSButton manualBrewBoilButton = NexDSButton(3, 2, "btBoil");
NexDSButton manualBrewPumpButton = NexDSButton(3, 6, "btMain");
NexDSButton manualBrewHermsButton = NexDSButton(3, 7, "btHerms");
NexButton manualBrewResetButton = NexButton(3, 20, "bReset");
NexButton manualBrewHltUpButton = NexButton(3, 27, "btHltUp");
NexButton manualBrewHltDownButton = NexButton(3, 29, "btHltDown");
NexButton manualBrewMltUpButton = NexButton(3, 35, "btMltUp");
NexButton manualBrewMltDownButton = NexButton(3, 36, "btMltDown");
NexButton manualBrewBoilUpButton = NexButton(3, 28, "btBoilUp");
NexButton manualBrewBoilDownButton = NexButton(3, 30, "btBoilDown");
NexButton manualBrewTimeUpButton = NexButton(3, 39, "btTimeUp");
NexButton manualBrewTimeDownButton = NexButton(3, 40, "btTimeDown");
NexText manualBrewHltActual = NexText(3, 31, "aHlt");
NexText manualBrewHltTarget = NexText(3, 24, "tHlt");
NexText manualBrewMltActual = NexText(3, 32, "aMlt");
NexText manualBrewMltTarget = NexText(3, 26, "tMlt");
NexText manualBrewBoilActual = NexText(3, 33, "aBoil");
NexText manualBrewBoilTarget = NexText(3, 25, "tBoil");
NexText manualBrewIpAddress = NexText(3, 15, "BrewManual.tIp");
NexText manualBrewSsid = NexText(3, 16, "BrewManual.tSsid");
NexText manualBrewAmpsLeg1 = NexText(3, 45, "tLeg1Amps");
NexText manualBrewAmpsLeg2 = NexText(3, 47, "tLeg2Amps");
NexNumber manualBrewHours = NexNumber(3, 8, "nHour");
NexNumber manualBrewMinutes = NexNumber(3, 11, "nMin");
NexNumber manualBrewSeconds = NexNumber(3, 12, "nSec");
NexButton manualBrewHomeButton = NexButton(3, 37, "bHome");
NexButton manualBrewStartButton = NexButton(3, 38, "bStart");
NexVariable manualBrewStatusMessage = NexVariable(3, 45, "BrewManual.vStatusMsg");

//DistillAuto Page
NexPage autoDistillPage = NexPage(4, 0, "DistillAuto");
NexButton autoDistillHomeButton = NexButton(4, 13, "bHome");
NexText autoDistillIpAddress = NexText(4, 8, "DistillAuto.tIp");
NexText autoDistillSsid = NexText(4, 10, "DistillAuto.tSsid");
NexText autoDistillAmpsLeg1 = NexText(4, 23, "tLeg1Amps");
NexText autoDistillAmpsLeg2 = NexText(4, 25, "tLeg2Amps");
NexText autoDistillColumnActual = NexText(4, 14, "aColumn");
NexText autoDistillBoilActual = NexText(4, 15, "aBoil");
NexText autoDistillAbv = NexText(4, 24, "tABV");
NexVariable autoDistillStatusMessage = NexVariable(4, 22, "DistillAuto.vStatusMsg");

//DistillManual Page
NexPage manualDistillPage = NexPage(5, 0, "DistillManual");
NexDSButton manualDistillBoilButton = NexDSButton(5, 11, "btBoil");
NexDSButton manualDistillPumpButton = NexDSButton(5, 22, "btPump");
NexText manualDistillColumnActual = NexText(5, 12, "aColumn");
NexText manualDistillBoilActual = NexText(5, 13, "aBoil");
NexText manualDistillCoolantActual = NexText(5, 27, "aCoolant");
NexSlider manualDistillSlider = NexSlider(5, 16, "hPP");
NexNumber manualDistillPowerPercentage = NexNumber(5, 19, "nPowerPercent");
NexText manualDistillIpAddress = NexText(5, 7, "DistillManual.tIp");
NexText manualDistillSsid = NexText(5, 9, "DistillManual.tSsid");
NexText manualDistillAmpsLeg1 = NexText(5, 10, "tLeg1Amps");
NexText manualDistillAmpsLeg2 = NexText(5, 29, "tLeg2Amps");
NexText manualDistillAbv = NexText(5, 25, "tABV");
NexButton manualDistillHomeButton = NexButton(5, 21, "bHome");
NexVariable manualDistillStatusMessage = NexVariable(5, 24, "DistillManual.vStatusMsg");

//Settings Page
NexPage settingsPage = NexPage(6, 0, "Settings");
NexButton settingsSaveButton = NexButton(6, 1, "bSave");
NexButton settingsCancelButton = NexButton(6, 2, "bCancel");
NexText settingsSsid = NexText(6, 4, "tSsid");
NexText settingsPass = NexText(6, 5, "tPass");
NexCheckbox settingsHerms = NexCheckbox(6, 8, "cHerms");
NexCheckbox settingsStillPump = NexCheckbox(6, 10, "cStillPump");
NexText settingsTimeZone = NexText(6, 11, "tTimeZone");

NexTouch *nex_listen_list[] = {
  &settingsPage,
  &manualDistillPage,
  &autoDistillPage,
  &manualBrewPage,
  &autoBrewPage,
  &selectPage,
  &lockPage,
  &autoBrewMainPumpButton,
  &autoBrewHermsPumpButton,
  &autoDistillButton,
  &manualDistillButton,
  &settingsButton,
  &autoBrewButton,
  &manualBrewButton,
  &autoBrewHLTButton,
  &autoBrewBoilButton,
  &autoBrewNextButton,
  &manualBrewHLTButton,
  &manualBrewBoilButton,
  &manualBrewPumpButton,
  &manualBrewHermsButton,
  &manualBrewResetButton,
  &manualBrewStartButton,
  &manualDistillBoilButton,
  &manualDistillPumpButton,
  &manualDistillSlider,
  &manualDistillHomeButton,
  &settingsSaveButton,
  &settingsCancelButton,
  &autoBrewHomeButton,
  &manualBrewHomeButton,
  &autoDistillHomeButton,
  &manualDistillHomeButton,
  &manualBrewHltUpButton,
  &manualBrewHltDownButton,
  &manualBrewMltUpButton,
  &manualBrewMltDownButton,
  &manualBrewBoilUpButton,
  &manualBrewBoilDownButton,
  &manualBrewTimeUpButton,
  &manualBrewTimeDownButton,
  NULL
};

void setup() {
  // put your setup code here, to run once:
  chipId = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
  pinMode(ELEMENT1_CONTACTOR_RELAY, OUTPUT);
  pinMode(ELEMENT2_CONTACTOR_RELAY, OUTPUT);
  pinMode(PUMP1_RELAY, OUTPUT);
  pinMode(PUMP2_RELAY, OUTPUT);
  pinMode(ERROR_RELAY, OUTPUT);
  pinMode(ELEMENT1_SSR_RELAY, OUTPUT);
  pinMode(ELEMENT2_SSR_RELAY, OUTPUT);
  digitalWrite(ELEMENT1_CONTACTOR_RELAY, LOW);
  digitalWrite(ELEMENT2_CONTACTOR_RELAY, LOW);
  digitalWrite(PUMP1_RELAY, LOW);
  digitalWrite(PUMP2_RELAY, LOW);
  digitalWrite(ERROR_RELAY, LOW);
  digitalWrite(ELEMENT1_SSR_RELAY, LOW);
  digitalWrite(ELEMENT2_SSR_RELAY, LOW);

  Serial.begin(BAUD_RATE); //Programming
  Serial1.begin(BAUD_RATE, SERIAL_8N1, DISPLAY_RX, DISPLAY_TX);
  Serial2.begin(BAUD_RATE, SERIAL_8N1, DEBUG_RX, DEBUG_TX);

  if (displayEnabled) {
    nexSetup();
  }
  pidSetup();
  tempProbesSetup();
  adsSetup();
  xTaskCreate(
    OneWireTask,                  /* pvTaskCode */
    "OneWireWorkload",            /* pcName */
    1000,                   /* usStackDepth */
    NULL,                   /* pvParameters */
    7,                      /* uxPriority */
    &OneWireTaskHandle);                 /* pxCreatedTask */


  xTaskCreate(
    WiFiTask,                  /* pvTaskCode */
    "WiFIWorkload",            /* pcName */
    2500,                   /* usStackDepth */
    NULL,                   /* pvParameters */
    6,                      /* uxPriority */
    &wiFiTaskHandle);                 /* pxCreatedTask */

  xTaskCreate(
    AmmeterTask,                  /* pvTaskCode */
    "AmmeterWorkload",            /* pcName */
    2000,                   /* usStackDepth */
    NULL,                   /* pvParameters */
    1,                      /* uxPriority */
    &AmmeterTaskHandle);                 /* pxCreatedTask */

  xTaskCreatePinnedToCore(
    ProcessDisplayTask,                  /* pvTaskCode */
    "ProcessDisplayWorkload",            /* pcName */
    2000,                   /* usStackDepth */
    NULL,                   /* pvParameters */
    9,                      /* uxPriority */
    &ProcessDisplayTaskHandle,                 /* pxCreatedTask */
    0);
  xTaskCreatePinnedToCore(
    UpdateDisplayTask,                  /* pvTaskCode */
    "UpdateDisplayWorkload",            /* pcName */
    1000,                   /* usStackDepth */
    NULL,                   /* pvParameters */
    8,                      /* uxPriority */
    &UpdateDisplayTaskHandle,                 /* pxCreatedTask */
    1);
  xTaskCreate(
    DistillTask,                  /* pvTaskCode */
    "DistillWorkload",            /* pcName */
    1000,                   /* usStackDepth */
    NULL,                   /* pvParameters */
    5,                      /* uxPriority */
    &DistillTaskHandle);                 /* pxCreatedTask */
  xTaskCreate(
    BrewTask,                  /* pvTaskCode */
    "BrewWorkload",            /* pcName */
    1000,                   /* usStackDepth */
    NULL,                   /* pvParameters */
    4,                      /* uxPriority */
    &BrewTaskHandle);                 /* pxCreatedTask */
  xTaskCreate(
    RemoteReceiveTask,                  /* pvTaskCode */
    "RemoteWorkload",            /* pcName */
    2000,                   /* usStackDepth */
    NULL,                   /* pvParameters */
    3,                      /* uxPriority */
    &RemoteReceiveTaskHandle);                 /* pxCreatedTask */
  xTaskCreate(
    RemoteSendTask,                  /* pvTaskCode */
    "RemoteWorkload",            /* pcName */
    2000,                   /* usStackDepth */
    NULL,                   /* pvParameters */
    2,                      /* uxPriority */
    &RemoteSendTaskHandle);                 /* pxCreatedTask */
  wifiSetup();
  ConnectToAWS();
}

void loop() {
  //unused

}

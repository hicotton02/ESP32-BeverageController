void UpdateDisplayTask(void * parameter) {
  while (1) {
    //    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    //    Serial2.print( "Update Display Task Memory: " );
    //    Serial2.print(uxHighWaterMark );
    //    Serial2.print( " ");
    //    Serial2.println( esp_get_free_heap_size() );
    UpdateDisplay();
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
void ProcessDisplayTask(void * parameter) {
  while (1) {
    nexLoop(nex_listen_list);
    pageDisplayed = getPageDisplayed(); //this needs to go somewhere else
    //Serial2.print("Page Index = ");
    //Serial2.println(pageDisplayed);
    vTaskDelay(10);
  }
}
void nexSetup() {
  Serial2.println("Setting up Display");
  nexInit();
  autoDistillButton.attachPop(ButtonAutoDistillRelease, &autoDistillButton);
  manualDistillButton.attachPop(ButtonManualDistillRelease, &manualDistillButton);
  settingsButton.attachPop(ButtonSettingRelease, &settingsButton);
  autoBrewButton.attachPop(ButtonAutoBrewRelease, &autoBrewButton);
  manualBrewButton.attachPop(ButtonManualBrewRelease, &manualBrewButton);
  autoBrewHLTButton.attachPop(ButtonAutoBrewHLTRelease, &autoBrewHLTButton);
  autoBrewBoilButton.attachPop(ButtonAutoBrewBoilRelease, &autoBrewBoilButton);
  autoBrewNextButton.attachPop(ButtonAutoBrewNextRelease, &autoBrewNextButton);
  manualBrewHLTButton.attachPop(ButtonManualBrewHltRelease, &manualBrewHLTButton);
  manualBrewBoilButton.attachPop(ButtonManualBrewBoilRelease, &manualBrewBoilButton);
  manualBrewPumpButton.attachPop(ButtonManualBrewPumpRelease, &manualBrewPumpButton);
  manualBrewHermsButton.attachPop(ButtonManualBrewHermsRelease, &manualBrewHermsButton);
  manualBrewResetButton.attachPop(ButtonManualBrewResetRelease, &manualBrewResetButton);
  manualBrewStartButton.attachPop(ButtonManualBrewStartRelease, &manualBrewStartButton);
  manualDistillBoilButton.attachPop(ButtonManualDistillBoilRelease, &manualDistillBoilButton);
  manualDistillPumpButton.attachPop(ButtonManualDistillPumpRelease, &manualDistillPumpButton);
  manualDistillSlider.attachPop(SliderManualDistillRelease, &manualDistillSlider);
  manualDistillHomeButton.attachPop(HomeButtonRelease, &manualDistillHomeButton);
  settingsSaveButton.attachPop(ButtonSettingsSaveRelease, &settingsSaveButton);
  settingsCancelButton.attachPop(ButtonSettingsCancelRelease, &settingsCancelButton);
  autoBrewHomeButton.attachPop(HomeButtonRelease, &autoBrewHomeButton);
  manualBrewHomeButton.attachPop(HomeButtonRelease, &manualBrewHomeButton);
  autoDistillHomeButton.attachPop(HomeButtonRelease, &autoDistillHomeButton);
  manualBrewHomeButton.attachPop(HomeButtonRelease, &manualBrewHomeButton);
  manualBrewHltUpButton.attachPop(ButtonBrewHltUpRelease, &manualBrewHltUpButton);
  manualBrewHltUpButton.attachPush(ButtonBrewHltUpPress, &manualBrewHltUpButton);
  manualBrewHltDownButton.attachPop(ButtonBrewHltDownRelease, &manualBrewHltDownButton);
  manualBrewHltDownButton.attachPush(ButtonBrewHltDownPress, &manualBrewHltDownButton);
  manualBrewMltUpButton.attachPop(ButtonBrewMltUpRelease, &manualBrewMltUpButton);
  manualBrewMltUpButton.attachPush(ButtonBrewMltUpPress, &manualBrewMltUpButton);
  manualBrewMltDownButton.attachPop(ButtonBrewMltDownRelease, &manualBrewMltDownButton);
  manualBrewMltDownButton.attachPush(ButtonBrewMltDownPress, &manualBrewMltDownButton);
  manualBrewBoilUpButton.attachPop(ButtonBrewBoilUpRelease, &manualBrewBoilUpButton);
  manualBrewBoilUpButton.attachPush(ButtonBrewBoilUpPress, &manualBrewBoilUpButton);
  manualBrewBoilDownButton.attachPush(ButtonBrewBoilDownPress, &manualBrewBoilDownButton);
  manualBrewBoilDownButton.attachPop(ButtonBrewBoilDownRelease, &manualBrewBoilDownButton);
  manualBrewTimeDownButton.attachPop(ButtonBrewTimeDownRelease, &manualBrewTimeDownButton);
  manualBrewTimeDownButton.attachPush(ButtonBrewTimeDownPress, &manualBrewTimeDownButton);
  manualBrewTimeUpButton.attachPop(ButtonBrewTimeUpRelease, &manualBrewTimeUpButton);
  manualBrewTimeUpButton.attachPush(ButtonBrewTimeUpPress, &manualBrewTimeUpButton);
  SetDisplayRTCTime();
  UpdateDisplaySsid();
  UpdateDisplayIPAddress();
}
void UpdateDisplay() {
  UpdateDisplayAmps();
  if (stillRunning) {
    if (autoDistill) {
      //Auto Distill
    }
    else {
      //Manual Distill
      char abvText[6];
      char aBoilTemp[6];
      char aColumnTemp[6];
      char aCoolantTemp[6];
      dtostrf(actualSensorATemp, 3, 1, aColumnTemp);
      dtostrf(vaporAbv, 2, 1, abvText);
      dtostrf(actualSensorCTemp, 3, 1, aBoilTemp);
      dtostrf(actualSensorBTemp, 3, 1, aCoolantTemp);
      manualDistillColumnActual.setText(aColumnTemp);
      manualDistillBoilActual.setText(aBoilTemp);
      manualDistillCoolantActual.setText(aCoolantTemp);
      manualDistillAbv.setText(abvText);
    }
  }
  else if (brewSessionEnabled) {
    if (autoBrew) {
      //Auto Brew

    }
    else {
      //Manual Brew

    }
  }
  else {
    //non-updatable page
  }
}
void UpdateDisplayIPAddress() {
  manualDistillIpAddress.setText(localIp);
  manualBrewIpAddress.setText(localIp);
}
void UpdateDisplayAmps() {
  dtostrf(leg1, 4, 1, ampDisplayLeg1);
  dtostrf(leg2, 4, 1, ampDisplayLeg2);
  switch (pageDisplayed) {
    case 2:
      autoBrewAmpsLeg1.setText(ampDisplayLeg1);
      autoBrewAmpsLeg2.setText(ampDisplayLeg2);
      break;
    case 3:
      manualBrewAmpsLeg1.setText(ampDisplayLeg1);
      manualBrewAmpsLeg2.setText(ampDisplayLeg2);
      break;
    case 4:
      autoDistillAmpsLeg1.setText(ampDisplayLeg1);
      autoDistillAmpsLeg2.setText(ampDisplayLeg2);
      break;
    case 5:
      manualDistillAmpsLeg1.setText(ampDisplayLeg1);
      manualDistillAmpsLeg2.setText(ampDisplayLeg2);
      break;
    default:
      break;
  }
}
void UpdateDisplaySsid() {
  autoDistillSsid.setText(configuration.ssid);
  autoBrewSsid.setText(configuration.ssid);
  manualDistillSsid.setText(configuration.ssid);
  manualBrewSsid.setText(configuration.ssid);
}
void GetTime() {
  rtc.read_rtc_time(rtcTime, 7);
}
void SetDisplayRTCTime() {
  if (timeinfo.tm_year > 0) {
    rtcTime[0] = timeinfo.tm_year;
    rtcTime[1] = timeinfo.tm_mon;
    rtcTime[2] = timeinfo.tm_yday;
    rtcTime[3] = timeinfo.tm_hour;
    rtcTime[4] = timeinfo.tm_min;
    rtcTime[5] = timeinfo.tm_sec;
    rtc.write_rtc_time(rtcTime);
  }
}
void HomeButtonRelease(void *ptr) {
  Serial2.println("In Home Routine");
  brewSessionEnabled = false;
  autoBrew = false;
  stillRunning = false;
  autoDistill = false;
  pageDisplayed = 1;
}
void UpdateTimeOnManualBrewPage() {
  if (targetTimeInMinutes < 60)
  {
    manualBrewMinutes.setValue(targetTimeInMinutes);
  }
  else
  {
    uint8_t hours = targetTimeInMinutes / 60;
    manualBrewHours.setValue(hours);
    manualBrewMinutes.setValue(targetTimeInMinutes % 60);
  }
}
void ButtonBrewHltUpRelease(void *ptr) {
  manualHltButtonUpPressed = false;
}
void ButtonBrewHltUpPress(void *ptr) {
  manualHltButtonUpPressed = true;
}
void ButtonBrewHltDownRelease(void *ptr) {
  manualHltButtonDownPressed = false;
}
void ButtonBrewHltDownPress(void *ptr) {
  manualHltButtonDownPressed = true;
}
void ButtonBrewMltUpRelease(void *ptr) {
  manualMltButtonUpPressed = false;
}
void ButtonBrewMltUpPress(void *ptr) {
  manualMltButtonUpPressed = true;
}
void ButtonBrewMltDownRelease(void *ptr) {
  manualMltButtonDownPressed = false;
}
void ButtonBrewMltDownPress(void *ptr) {
  manualMltButtonDownPressed = true;
}
void ButtonBrewBoilUpRelease(void *ptr) {
  manualBoilButtonUpPressed = false;
}
void ButtonBrewBoilUpPress(void *ptr) {
  manualBoilButtonUpPressed = true;
}
void ButtonBrewBoilDownRelease(void *ptr) {
  manualBoilButtonDownPressed = false;
}
void ButtonBrewBoilDownPress(void *ptr) {
  manualBoilButtonDownPressed = true;
}
void ButtonBrewTimeUpRelease(void *ptr) {
  manualTimeButtonUpPressed = false;
}
void ButtonBrewTimeUpPress(void *ptr) {
  manualTimeButtonUpPressed = true;
}
void ButtonBrewTimeDownRelease(void *ptr) {
  manualTimeButtonDownPressed = false;
}
void ButtonBrewTimeDownPress(void *ptr) {
  manualTimeButtonDownPressed = true;
}
void ButtonSettingRelease(void *ptr) {
  //settingsPage.show();
  state = SETTINGS;
}

void ButtonManualDistillRelease(void *ptr) {
  EnableSession(false, false);
  Serial2.println("Starting Distilling Session from Manual Button");
}

void ButtonAutoDistillRelease(void *ptr) {
  EnableSession(false, true);
}

void ButtonManualBrewRelease(void *ptr) {
  EnableSession(true, false);
}

void ButtonAutoBrewRelease(void *ptr)
{
  EnableSession(true, true);
}
void ButtonAutoBrewHLTRelease(void *ptr)
{

  //override and toggle Hlt Element
  hltElementStatus = !hltElementStatus;
  autoBrewHLTButton.setValue(hltElementStatus);

  Serial2.print("Auto Brew Hlt Element changed to: ");
  Serial2.println(hltElementStatus);

}
void ButtonAutoBrewBoilRelease(void *ptr)
{
  //override and toggle Boil Element
  boilElementStatus = !boilElementStatus;
  autoBrewBoilButton.setValue(boilElementStatus);

  Serial2.print("Auto Brew Boil Element changed to: ");
  Serial2.println(boilElementStatus);

}
void ButtonAutoBrewNextRelease(void *ptr)
{
  waitingOnUserResponse = false;
  goToNextStep = true;
}
void ButtonManualBrewHltRelease(void *ptr)
{
  //Toggle Hlt Element
  hltElementStatus = !hltElementStatus;
  manualBrewHLTButton.setValue(hltElementStatus);
}
void ButtonManualBrewBoilRelease(void *ptr)
{
  //Toggle Boiler Element
  Serial2.println("Toggling Boiler Element from Display");
  boilElementStatus = !boilElementStatus;
  manualBrewBoilButton.setValue(boilElementStatus);
}
void ButtonManualBrewPumpRelease(void *ptr)
{
  //Toggle Main Pump
  Serial2.println("Toggling Pump Element from Display");
  mainPumpStatus = !mainPumpStatus;
  manualBrewPumpButton.setValue(mainPumpStatus);
}
void ButtonManualBrewHermsRelease(void *ptr)
{
  //Toggle Herms Pump
  hermsPumpStatus = !hermsPumpStatus;
  manualBrewHermsButton.setValue(hermsPumpStatus);
}
void ButtonManualBrewResetRelease(void *ptr)
{
  // Reset page counter to 0s
  targetTimeInMinutes = 0;
}
void ButtonManualBrewStartRelease(void *ptr)
{
  //start counter
  brewTimerStarted = true;
}
void ButtonManualDistillBoilRelease(void *ptr)
{
  //Toggle Boiler Element
  Serial2.println("Toggling Boiler Element from Display");
  boilElementStatus = !boilElementStatus;
  manualDistillBoilButton.setValue(boilElementStatus);
}
void SliderManualDistillRelease(void *ptr)
{
  //Set Power % for boiler
  unsigned int value;
  manualDistillSlider.getValue(&value);
  boilerPowerPercentage = value / 100.0;
  distillDutyTime = distillDutyCycle * boilerPowerPercentage;
}
void ButtonManualDistillPumpRelease(void *ptr)
{
  //Toggle Recirculating Pump
  Serial2.println("Toggling Pump Element from Display");
  mainPumpStatus = !mainPumpStatus;
  manualDistillPumpButton.setValue(mainPumpStatus);
}
void ButtonSettingsSaveRelease(void *ptr)
{

  settingsStillPump.getValue(&configuration.enableDistillPump);
  settingsHerms.getValue(&configuration.hermsEnabled);
  settingsSsid.getText(configuration.ssid, sizeof(configuration.ssid));
  settingsPass.getText(configuration.pass, sizeof(configuration.pass));
  settingsTimeZone.getText(configuration.timeZone, sizeof(configuration.timeZone));

  //wifiSetup();
}
void ButtonSettingsCancelRelease(void *ptr)
{

}

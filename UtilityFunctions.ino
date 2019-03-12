void append(char* s, char c) {
  int len = strlen(s);
  s[len] = c;
  s[len + 1] = '\0';
}

void adsSetup() {
  Serial2.println("Setting up ADS");
  Wire.begin(SDA, SCL);
  Wire.setClock(2000000);
  adsLeg1.setGain(GAIN_TWO);
  //adsLeg1.begin();
  adsLeg1.setSPS(ADS1015_DR_3300SPS);
  //adsLeg1.startContinuous_Differential_0_1();
  adsLeg2.setGain(GAIN_TWO);
  //adsLeg2.begin();
  adsLeg2.setSPS(ADS1015_DR_3300SPS);
  //adsLeg2.startContinuous_Differential_0_1();

}
void AmmeterTask(void * parameter) {
  long stopTime = millis() + 500;
  while (1) {
    if (millis() < stopTime) {
      int readValue = adsLeg1.readADC_Differential_0_1(); // read
      //Serial2.print("Leg1: ");
      //Serial2.println(readValue);
      if (readValue > maxValueLeg1) maxValueLeg1 = readValue; // update max
      if (readValue < minValueLeg1) minValueLeg1 = readValue; // update min
      readValue = adsLeg2.readADC_Differential_0_1(); // read
      //Serial2.print("Leg2: ");
      //Serial2.println(readValue);
      if (readValue > maxValueLeg2) maxValueLeg2 = readValue; // update max
      if (readValue < minValueLeg2) minValueLeg2 = readValue; // update min
    }
    else    {
      leg1 = ((maxValueLeg1 - minValueLeg1) * multiplier);
//      Serial2.print("Leg1 Min Value: ");
//      Serial2.println(minValueLeg1);
//      Serial2.print("Leg1 Max Value: ");
//      Serial2.println(maxValueLeg1);
//      Serial2.print("Leg1 Calculated: ");
//      Serial2.println(leg1);
      maxValueLeg1 = -4096;
      minValueLeg1 = 0;
      leg2 = ((maxValueLeg2 - minValueLeg2) * multiplier);
//      Serial2.print("Leg2 Min Value: ");
//      Serial2.println(minValueLeg2);
//      Serial2.print("Leg2 Max Value: ");
//      Serial2.println(maxValueLeg2);
//      Serial2.print("Leg2 Calculated: ");
//      Serial2.println(leg2);
      maxValueLeg2 = -4096;
      minValueLeg2 = 0;
      stopTime = millis() + 500;
      UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
      //      Serial2.print(xPortGetCoreID());
      //      Serial2.print( "Ammeter Task Memory: " );
      //      Serial2.print(uxHighWaterMark );
      //      Serial2.print( " ");
      //      Serial2.println( esp_get_free_heap_size() );
      vTaskDelay(10);
    }
  }
}
void CalculateABV() {
  vaporAbv = TtoVaporABV((float)actualSensorBTemp, currentPressure * 100); //Converting mB to hPa
  //Serial2.println(vaporAbv);
  boilerAbv = TtoLiquidABV((float)actualSensorATemp, currentPressure * 100); //Converting mB to hPa
  //Serial2.println(boilerAbv);
}
void pidSetup() {
  Serial2.println("Setting up PIDs");

  element1Pid.SetOutputLimits(0, pidWindowSize);
  element2Pid.SetOutputLimits(0, pidWindowSize);

  element1Pid.SetMode(AUTOMATIC);
  element2Pid.SetMode(AUTOMATIC);

}

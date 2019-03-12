void OneWireTask(void * parameter) {
  for (;;)
  {
    //Serial2.println("Requesting Temperatures");
    sensorC.requestTemperatures();
    sensorB.requestTemperatures();
    sensorA.requestTemperatures();

    vTaskDelay(oneWireDelay);
//    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL ); 
//    Serial2.print(xPortGetCoreID());
//    Serial2.print( "OneWire Task Memory: " ); 
//    Serial2.print(uxHighWaterMark ); 
//    Serial2.print( " "); 
//    Serial2.println( esp_get_free_heap_size() );
//    Serial2.println("Updating Temperatures");
    if ( sensorC.getTempFByIndex(0) > 0 && sensorC.getTempFByIndex(0) < 212)  actualSensorCTemp = sensorC.getTempFByIndex(0);
    if ( sensorA.getTempFByIndex(0) > 0 && sensorA.getTempFByIndex(0) < 212)  actualSensorATemp = sensorA.getTempFByIndex(0);
    if ( sensorB.getTempFByIndex(0) > 0 && sensorB.getTempFByIndex(0) < 212)  actualSensorBTemp = sensorB.getTempFByIndex(0);
//    Serial2.print("SensorA: ");
//    Serial2.println(actualSensorATemp);
//    Serial2.print("SensorB: ");
//    Serial2.println(actualSensorBTemp);
//    Serial2.print("SensorC: ");
//    Serial2.println(actualSensorCTemp);
    CalculateABV();
  }
  vTaskDelete(NULL);
}
void tempProbesSetup()
{

  Serial2.println("Setting up onewire devices");

  pinMode(TEMPC_BUS, INPUT);
  pinMode(TEMPB_BUS, INPUT);
  pinMode(TEMPA_BUS, INPUT);

  sensorC.begin();
  sensorC.setResolution(TEMP_PRECISION);
  sensorC.setWaitForConversion(false);
  sensorA.begin();
  sensorA.setResolution(TEMP_PRECISION);
  sensorA.setWaitForConversion(false);
  sensorB.begin();
  sensorB.setResolution(TEMP_PRECISION);
  sensorB.setWaitForConversion(false);
}

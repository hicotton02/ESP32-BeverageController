void RemoteReceiveTask(void * parameter) {
  /*
     This task should handle any AWS IoT requests
  */
  while (1) {
    if (msgReceived == 1)
    {
      msgReceived = 0;
      Serial2.print("Received Message:");
      Serial2.println(rcvdPayload);
      const size_t capacity = 5 * JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + 210;
      DynamicJsonBuffer jsonBuffer(capacity);
      JsonObject& root = jsonBuffer.parseObject(rcvdPayload);

      JsonObject& state_desired = root["state"]["desired"];
      const char* state_desired_function = state_desired["function"]; // "still"
      int state_desired_power = state_desired["power"]; // 100
      const char* state_desired_mode = state_desired["mode"];
      Serial2.print("State Desired: ");
      Serial2.println(state_desired_function);
      Serial2.print("Power Desired: ");
      Serial2.println(state_desired_power);
      Serial2.print("Mode Desired: ");
      Serial2.println(state_desired_mode);

      if (strcmp("still", state_desired_function) == 0) {
        boilerPowerPercentage = state_desired_power / 100.0;
        if (strcmp("manual", state_desired_mode) == 0) {
          EnableSession(false, false);
          boilElementStatus = true;
          Serial2.println("Changing to Distilling Manual Mode");
        }
        else if (strcmp("auto", state_desired_mode) == 0) {
          EnableSession(false, true);
          boilElementStatus = true;
          Serial2.println("Changing to Distilling Auto Mode");
        }
      }
      else {
        Serial2.println("Not Implemented");
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
void RemoteSendTask(void * parameter) {
  while (1) {
    const size_t capacity = 2 * JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(6);
    DynamicJsonBuffer jsonBuffer(capacity);
    JsonObject& root = jsonBuffer.createObject();
    JsonObject& state = root.createNestedObject("state");
    JsonObject& state_reported = state.createNestedObject("reported");
    if (stillRunning) {
      state_reported["function"] = "still";
      state_reported["power"] = boilerPowerPercentage * 100;
      if (autoDistill) {
        state_reported["state"] = "auto";
      }
      else {
        state_reported["state"] = "manual";
      }
    }
    else if (brewSessionEnabled) {
      state_reported["function"] = "brew";
      if (autoBrew) {
        state_reported["state"] = "auto";
      }
      else {
        state_reported["state"] = "manual";
      }
    }

    state_reported["TempA"] = actualSensorATemp;
    state_reported["TempB"] = actualSensorBTemp;
    state_reported["TempC"] = actualSensorCTemp;
    char output[120];
    root.printTo(output);
    awsClient.publish(publishTopic, output);
    vTaskDelay(pdMS_TO_TICKS(20000));
  }
}
void mySubCallBackHandler (char *topicName, int payloadLen, char *payLoad) {
  if (msgReceived == 0)
  {
    Serial2.println(topicName);
    strncpy(rcvdPayload, payLoad, payloadLen);
    rcvdPayload[payloadLen] = 0;
    msgReceived = 1;
  }
}

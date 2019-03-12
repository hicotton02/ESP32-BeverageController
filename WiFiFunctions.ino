void WiFiTask(void * parameter) {
  /*
     This task should ensure that the wifi is connected and control all subsequent calls that require wifi to be connected.

     1. Get External IP address
     2. Call out to NTP server for correct time
     3. Call out to weather API to get correct barometric pressure for ABV calculation (Experimental, may need to add bmp280 to circuit)
     4. Connect to AWS Server

  */
  while (1) {
    GetWeatherData();
    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    Serial2.print(xPortGetCoreID());
    Serial2.print( "WiFi Task Memory: " );
    Serial2.print(uxHighWaterMark );
    Serial2.print( " ");
    Serial2.println( esp_get_free_heap_size() );
    vTaskDelay(wifiTaskDelay);
  }
}
void WiFiServerTask(void * parameter) {
  /*
     This task should handle any server tasks that have been queued. this task may not be needed.
  */
}

void GetExternalIP() {
  WiFiClient client;
  if (!client.connect("api.ipify.org", 80)) {
    Serial2.println("Failed to connect with 'api.ipify.org' !");
  }
  else {
    int timeout = millis() + 5000;
    client.print("GET /?format=json HTTP/1.1\r\nHost: api.ipify.org\r\n\r\n");
    // Check HTTP status
    char status[32] = {0};
    client.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
      Serial2.print(F("Unexpected response: "));
      Serial2.println(status);
      return;
    }// Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!client.find(endOfHeaders)) {
      Serial2.println(F("Invalid response"));
      return;
    }
    const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_ARRAY_SIZE(2) + 43;
    DynamicJsonBuffer jsonBuffer(capacity);

    // Parse JSON object
    JsonObject& root = jsonBuffer.parseObject(client);
    if (!root.success()) {
      Serial2.println(F("Parsing failed!"));
      return;
    }

    // Extract values
    strlcpy(externalIp, root["ip"] | "0.0.0.0", 16);
    Serial2.print("External IP: ");
    Serial2.println(externalIp);
    // Disconnect
    client.stop();
  }
}

void GetWeatherData() {
  WiFiClient client;
  char fullQueryString[170];
  sprintf(fullQueryString, "GET /premium/v1/weather.ashx?key=d54be7730f504424bbc231440192901&q=%s&format=json&num_of_days=1 HTTP/1.1\r\nHost: api.worldweatheronline.com\r\n\r\n", externalIp);
  if (!client.connect("api.worldweatheronline.com", 80)) {
    Serial2.println("Failed to connect with 'api.worldweatheronline.com' !");
  }
  else {
    int timeout = millis() + 5000;
    client.print(fullQueryString);
    // Check HTTP status
    char status[32] = {0};
    client.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
      Serial2.print(F("Unexpected response: "));
      Serial2.println(status);
      return;
    }// Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!client.find(endOfHeaders)) {
      Serial2.println(F("Invalid response"));
      return;
    }
    const size_t capacity = 4 * JSON_ARRAY_SIZE(1) + 3 * JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(18) + 500;
    DynamicJsonBuffer jsonBuffer(capacity);

    // Parse JSON object
    JsonObject& root = jsonBuffer.parseObject(client);
    if (!root.success()) {
      Serial2.println(F("Parsing failed!"));
      return;
    }
    JsonObject& data_current_condition = root["data"]["current_condition"][0];

    // Extract values
    currentPressure = (int)data_current_condition["pressure"];
    Serial2.print("Barometric Pressure: ");
    Serial2.println(currentPressure);
    // Disconnect
    client.stop();
  }
}

void NTPTask(void * parameter) {
  //Not sure if needed
}
void GetNTPTime() {
  int timeZoneOffsetH = 0;
  sscanf(configuration.timeZone, "%d", &timeZoneOffsetH);
  timeZoneOffsetS = timeZoneOffsetH * 60 * 60;
  configTime(timeZoneOffsetS, 3600, ntpServerName);
  if (!getLocalTime(&timeinfo))
  {
    Serial2.println("Failed to get time");
    return;
  }
  Serial2.println(&timeinfo, "%I:%M:%S");
  if (displayEnabled) {
    SetDisplayRTCTime();
  }
}
void ConnectToAWS() {
  Serial2.println("Connecting to AWS");
  if (awsClient.connect(awsHostAddress, awsClientId) == 0)
  {
    Serial2.println("Connected to AWS");
    vTaskDelay(pdMS_TO_TICKS(1000));

    if (0 == awsClient.subscribe(awsSubscribeTopic, mySubCallBackHandler))
    {
      Serial2.println("Subscribe Successful");
    }
    else
    {
      Serial2.println("Subscribe Failed, Check the Thing Name and Certificates");
      while (1);
    }
  }
  else
  {
    Serial2.println("AWS connection failed, Check the HOST Address");
    while (1);
  }

  vTaskDelay(pdMS_TO_TICKS(2000));

}
void wifiSetup() {
  Serial2.println("Setting up WiFi");
  if (configuration.ssid && configuration.pass) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(configuration.ssid, configuration.pass);
    while (WiFi.status() != WL_CONNECTED) {
      vTaskDelay(wifiConnectDelay);
      Serial2.print(".");
    }
    strncpy(localIp, WiFi.localIP().toString().c_str(), 16);
    Serial2.print("IP Address: ");
    Serial2.println(localIp);

    //Get NTP Time
    GetNTPTime();
    StartOTA();
    GetExternalIP();
  }
}

void StartOTA() {
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial2.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial2.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial2.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial2.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial2.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial2.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial2.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial2.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial2.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial2.println("OTA Ready");
}

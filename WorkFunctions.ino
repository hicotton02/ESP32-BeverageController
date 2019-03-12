void DistillTask(void * parameter) {
  while (1) {
    if (stillRunning) {
      //long currentMillis = millis();
      if (!(contactor1Status || contactor2Status)) {
        digitalWrite(ELEMENT1_CONTACTOR_RELAY, HIGH);
        contactor1Status = true;
        digitalWrite(ELEMENT2_CONTACTOR_RELAY, HIGH);
        contactor2Status = true;
      }
      if (autoDistill) {
        //TODO
      }
      else { //autoDistill
        if (boilElementStatus) {
          if (millis() - lastDistillDutyCycle <= distillDutyTime)
          {
            if (digitalRead(ELEMENT1_SSR_RELAY) == LOW || digitalRead(ELEMENT2_SSR_RELAY) == LOW)
            {
              Serial2.println("Turning SSR relays on");
              digitalWrite(ELEMENT1_SSR_RELAY, HIGH);
              digitalWrite(ELEMENT2_SSR_RELAY, HIGH);
            }
          }
          //else turn off
          else
          {
            if (digitalRead(ELEMENT1_SSR_RELAY) == HIGH || digitalRead(ELEMENT2_SSR_RELAY) == HIGH)
            {
              Serial2.println("Turning SSR relays off");
              digitalWrite(ELEMENT1_SSR_RELAY, LOW);
              digitalWrite(ELEMENT2_SSR_RELAY, LOW);
            }
          }
          //if current time is outside of cycle time, reset.
          if (millis() - lastDistillDutyCycle >= distillDutyCycle)
          {
            lastDistillDutyCycle = millis();
            Serial2.print("Updated Distilling Duty Cycle to: ");
            Serial2.println(lastDistillDutyCycle);
          }
        }
        else {
          if (digitalRead(ELEMENT1_SSR_RELAY) == HIGH || digitalRead(ELEMENT2_SSR_RELAY) == HIGH)
          {
            Serial2.println("Turning SSR relays off");
            digitalWrite(ELEMENT1_SSR_RELAY, LOW);
            digitalWrite(ELEMENT2_SSR_RELAY, LOW);
          }
        }
        if (mainPumpStatus) {
          if (digitalRead(PUMP1_RELAY) == LOW) {
            digitalWrite(PUMP1_RELAY, HIGH);
          }
        }
        else {
          if (digitalRead(PUMP1_RELAY) == HIGH)
            digitalWrite(PUMP1_RELAY, LOW);
        }
      }
      vTaskDelay(100);
    }
    else { //stillRunning
      //go to sleep
      if (contactor1Status || contactor2Status) {
        digitalWrite(ELEMENT1_CONTACTOR_RELAY, LOW);
        contactor1Status = false;
        digitalWrite(ELEMENT2_CONTACTOR_RELAY, LOW);
        contactor2Status = false;
      }
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }
}
void BrewTask(void * parameter) {
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}
void EnableSession(bool isBrew, bool isAuto) {
  if (isBrew) {
    if (isAuto) {
      sendCommand("dp=2");
      brewSessionEnabled = true;
      autoBrew = true;
    }
    else {
      sendCommand("dp=3");
      brewSessionEnabled = true;
      autoBrew = false;
    }
  }
  else {
    if (isAuto) {
      sendCommand("dp=4");
      stillRunning = true;
      autoDistill = true;
    }
    else {
      sendCommand("dp=5");
      stillRunning = true;
      autoDistill = false;
      manualDistillPowerPercentage.setValue(boilerPowerPercentage  * 100);
      manualDistillSlider.setValue(boilerPowerPercentage * 100);
    }
  }
}

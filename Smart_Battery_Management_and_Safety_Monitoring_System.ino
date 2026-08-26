// Smart Battery Management and Safety Monitoring System
#define BLYNK_TEMPLATE_ID "TMPL3I8L0Ypdt"
#define BLYNK_TEMPLATE_NAME "Smart Battery Management and Safety Monitoring"
#define BLYNK_AUTH_TOKEN "IIdgF0-qLNqc1i5auqXAc9VWClq_pTLb"
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
const int MAX_CELLS=4;
const float max_temperature=45.0;
const float clear_temperature=42.0;
float max_voltage=4.20,min_voltage=2.5;
const int POT_PIN=34;
const int RELAY_PIN=25;
const int RELAY_FEEDBACK_PIN=26;
LiquidCrystal_I2C lcd(0x27,16,2);
int currentPage=0;
unsigned long lastPageChange=0,lastLCDUpdate=0;
const unsigned long pageInterval=3000;
const unsigned long relayDebounceTime=2000;
const unsigned long relayRecoveryTime=3000;
const unsigned long failsafeVerificationTime=5000;
const unsigned long shutdownTimeout=15000;
const unsigned long lcdRefreshInterval=250;
String previousRow0="",previousRow1="";
bool lcdFaultActive=false;
String lcdFaultMessage="";
const float unrealisticJump=0.80;
const float noiseLimit=0.05;
const int frozenLimit=3;
enum RELAYState
{
  RELAY_NORMAL,
  RELAY_TRIP,
  RELAY_RECOVERY
};
RELAYState relayState=RELAY_NORMAL;
enum SystemState
{
  STATE_NORMAL,
  STATE_DEGRADED,
  STATE_FAILSAFE,
  STATE_SHUTDOWN
};
SystemState systemState=STATE_NORMAL;
enum FaultSource
{
  FAULT_NONE,
  FAULT_BATTERY,
  FAULT_RELAY,
  FAULT_COMMUNICATION,
  FAULT_ADC
};
enum FaultID
{
  FAULT_ID_NONE,
  FAULT_ID_BATTERY_CELL,
  FAULT_ID_RELAY_MISMATCH,
  FAULT_ID_COMMUNICATION_LOSS,
  FAULT_ID_ADC_FROZEN
};
FaultSource activeFaultSource=FAULT_NONE;
FaultID activeFaultID=FAULT_ID_NONE;
uint8_t activeFaultCell=0;
struct Cell
{
  uint8_t cell_id;
  float voltage;
  float temperature;
  bool fault;
  bool Overvoltage;
  bool Undervoltage;
  bool Overheat;
  bool sensorFrozen;
  bool unrealisticValue;
  bool sensorNoise;
  bool genuineRapidChange;
  float soh;
  float soc;
};
Cell cells[MAX_CELLS];
struct TelemetryEvent
{
  uint32_t eventId;
  unsigned long timestamp;
  float cellVoltage[MAX_CELLS];
  uint8_t weakestCell;
  uint8_t strongestCell;
  bool relayNormal;
  bool faultActive;
  SystemState systemState;
  FaultSource faultSource;
  FaultID faultID;
  int rssi;
};
const int TELEMETRY_QUEUE_SIZE=10;
TelemetryEvent telemetryQueue[TELEMETRY_QUEUE_SIZE];
int queueHead=0;
int queueTail=0;
int queueCount=0;
TelemetryEvent lastTelemetryEvent;
bool telemetryHistoryValid=false;
uint32_t nextEventId=1;
const float CELL_VOLTAGE_EVENT_THRESHOLD=0.05;
const char* WIFI_SSID="Wokwi-GUEST";
const char* WIFI_PASS="";
enum WiFiState
{
  WIFI_DISCONNECTED,
  WIFI_CONNECTING,
  WIFI_CONNECTED
};
WiFiState wifiState=WIFI_DISCONNECTED;
unsigned long wifiAttemptTime=0;
unsigned long lastBlynkAttempt=0;
unsigned long lastQueueReplay=0;
const unsigned long wifiRetryInterval=5000;
const unsigned long blynkRetryInterval=5000;
const unsigned long queueReplayInterval=1000;
bool networkOutageTest=false;
struct BMSAnalysis
{
  uint8_t weakestCell;
  uint8_t strongestCell;
  float minimumVoltage;
  float maximumVoltage;
  float voltageImbalance;
  float previousImbalance;
  float imbalanceChange;
  bool imbalanceIncreasing;
  float averageSOC;
  float adaptiveThreshold;
  bool imbalanceWarning;
};
BMSAnalysis analysis;
int i;
int highest_voltage=0,lowest_voltage=0;
unsigned long faultStartTime=0;
unsigned long relayRecoveryStartTime=0;
unsigned long systemRecoveryStartTime=0;
unsigned long stateTransitionTime=0;
unsigned long failsafeStartTime=0;
unsigned long shutdownStartTime=0;
bool faultTimerActive=false;
bool recoveryVerificationActive=false;
float previousVoltage[MAX_CELLS];
float previousChange[MAX_CELLS];
int frozenCounter[MAX_CELLS];
int rapidChangeCounter[MAX_CELLS];
float previousBaseVoltage=0.0;
int previousADCValue=-1;
int adcFrozenCounter=0;
const int adcFrozenLimit=5;
bool adcFrozenFault=false;
bool adcFreezeTest=false;
bool relayMismatchFault=false;
bool communicationFault=false;
bool relayFeedbackState=true;
int relayMismatchCounter=0;
const int relayMismatchLimit=2;
unsigned long lastCommunicationTime=0;
const unsigned long communicationTimeout=10000;
bool communicationMonitoring=false;
bool faultActive=false;
float riskScore=0.0;
float batteryHealth=100.0;
float faultFrequency=0.0;
unsigned long systemStartTime=0;
int totalFaultCount=0;
int stateTransitionCount=0;
int maintenanceRecommendation=0;
String maintenanceRecommendationText="Battery operating normally";
bool previousFaultActive=false;
SystemState previousLoggedState=STATE_NORMAL;
const unsigned long analyticsInterval=5000;
unsigned long lastAnalyticsUpdate=0;
const unsigned long faultFrequencyWindow=3600000UL;
const int MAX_FAULT_HISTORY=20;
unsigned long faultHistory[MAX_FAULT_HISTORY];
int faultHistoryCount=0;
int faultHistoryHead=0;
void clearFaultData()
{
  activeFaultSource=FAULT_NONE;
  activeFaultID=FAULT_ID_NONE;
  activeFaultCell=0;
  faultActive=false;
}
const char* getSystemStateName(SystemState state)
{
  switch(state)
  {
    case STATE_NORMAL:
      return "NORMAL";//0
    case STATE_DEGRADED:
      return "DEGRADED";//1
    case STATE_FAILSAFE:
      return "FAILSAFE";//2
    case STATE_SHUTDOWN:
      return "SHUTDOWN";//3
    default:
      return "UNKNOWN";
  }
}
int getSystemStateCode(SystemState state)
{
  switch(state)
  {
    case STATE_NORMAL:
      return 0;
    case STATE_DEGRADED:
      return 1;
    case STATE_FAILSAFE:
      return 2;
    case STATE_SHUTDOWN:
      return 3;
    default:
      return 0;
  }
}
const char* getFaultSourceName(FaultSource source)
{
  switch(source)
  {
    case FAULT_NONE:
      return "NONE";
    case FAULT_BATTERY:
      return "BATTERY";
    case FAULT_RELAY:
      return "RELAY";
    case FAULT_COMMUNICATION:
      return "COMMUNICATION";
    case FAULT_ADC:
      return "ADC";
    default:
      return "UNKNOWN";
  }
}
const char* getFaultIDName(FaultID faultID)
{
  switch(faultID)
  {
    case FAULT_ID_NONE:
      return "NONE";
    case FAULT_ID_BATTERY_CELL:
      return "BATTERY_CELL";
    case FAULT_ID_RELAY_MISMATCH:
      return "RELAY_MISMATCH";
    case FAULT_ID_COMMUNICATION_LOSS:
      return "COMMUNICATION_LOSS";
    case FAULT_ID_ADC_FROZEN:
      return "ADC_FROZEN";
    default:
      return "UNKNOWN";
  }
}
void setFault(FaultSource source,FaultID faultID)
{
  activeFaultSource=source;
  activeFaultID=faultID;
  faultActive=true;
}
void recordFaultOccurrence()
{
  unsigned long now=millis();
  if(faultHistoryCount<MAX_FAULT_HISTORY)
  {
    int index=(faultHistoryHead+faultHistoryCount)%MAX_FAULT_HISTORY;
    faultHistory[index]=now;
    faultHistoryCount++;
  }
  else
  {
    faultHistory[faultHistoryHead]=now;
    faultHistoryHead=(faultHistoryHead+1)%MAX_FAULT_HISTORY;
  }
  totalFaultCount++;
  if(Blynk.connected())
  {
    String description="Fault: ";
    description+=getFaultSourceName(activeFaultSource);
    description+=" | ID: ";
    description+=getFaultIDName(activeFaultID);
    if(activeFaultCell>0)
    {
      description+=" | Cell: ";
      description+=String(activeFaultCell);
    }
    Blynk.logEvent("fault_detected",description);
  }
}
void updateFaultFrequency()
{
  unsigned long now=millis();
  int validCount=0;
  for(int j=0;j<faultHistoryCount;j++)
  {
    int index=(faultHistoryHead+j)%MAX_FAULT_HISTORY;

    if(now-faultHistory[index]<=faultFrequencyWindow)
    validCount++;
  }
  faultFrequency=(float)validCount;
}
void logStateTransition( SystemState previousState,SystemState newState, FaultID faultID)
{
  stateTransitionTime=millis();
  stateTransitionCount++;
  previousLoggedState=newState;
  Serial.print("STATE_TRANSITION");
  Serial.print(" | TIME:");
  Serial.print(stateTransitionTime);
  Serial.print(" | PREVIOUS:");
  Serial.print(getSystemStateName(previousState));
  Serial.print(" | NEW:");
  Serial.print(getSystemStateName(newState));
  Serial.print(" | SOURCE:");
  Serial.print(getFaultSourceName(activeFaultSource));
  Serial.print(" | FAULT:");
  Serial.print(getFaultIDName(faultID));
  if(activeFaultCell>0)
  {
    Serial.print(" | CELL:");
    Serial.print(activeFaultCell);
  }
  Serial.println();
  if(Blynk.connected())
  {
    String description="State: ";
    description+=getSystemStateName(previousState);
    description+=" -> ";
    description+=getSystemStateName(newState);
    description+=" | Fault: ";
    description+=getFaultIDName(faultID);
    Blynk.logEvent("state_transition",description);
    Blynk.virtualWrite(V8,getSystemStateCode(newState));
    Blynk.virtualWrite(V21,getSystemStateCode(newState));
    Blynk.virtualWrite(V22,stateTransitionCount);
  }
}
void calculateMaintenanceRecommendation()
{
  if(systemState==STATE_FAILSAFE ||systemState==STATE_SHUTDOWN ||riskScore>=75.0)
  {
    maintenanceRecommendation=3;
    maintenanceRecommendationText= "Immediate attention required";
  }
  else if(riskScore>=50.0 ||faultFrequency>=3.0 ||analysis.imbalanceWarning)
  {
    maintenanceRecommendation=2;
    maintenanceRecommendationText="Maintenance recommended";
  }
  else if( riskScore>=25.0 || analysis.imbalanceIncreasing || analysis.averageSOC<30.0)
  {
    maintenanceRecommendation=1;
    maintenanceRecommendationText= "Monitor battery condition";
  }
  else
  {
    maintenanceRecommendation=0;
    maintenanceRecommendationText= "Battery operating normally";
  }
}
void calculateRiskScore()
{
  float imbalanceRisk=0.0;
  float faultRisk=0.0;
  float socRisk=0.0;
  imbalanceRisk=(analysis.voltageImbalance/0.15)*100.0;
  if(analysis.imbalanceIncreasing)
    imbalanceRisk*=1.25;
  if(imbalanceRisk>100.0)
    imbalanceRisk=100.0;
  if(imbalanceRisk<0.0)
    imbalanceRisk=0.0;
  faultRisk=(faultFrequency/10.0)*100.0;
  if(faultRisk>100.0)
    faultRisk=100.0;
  if(faultRisk<0.0)
    faultRisk=0.0;
socRisk=100.0-analysis.averageSOC;
  if(socRisk>100.0)
    socRisk=100.0;
  if(socRisk<0.0)
    socRisk=0.0;
  riskScore= (imbalanceRisk*0.40)+ (faultRisk*0.30)+ (socRisk*0.30);
  if(systemState==STATE_FAILSAFE)
  {
    if(riskScore<80.0)
      riskScore=80.0;
  }
  if(systemState==STATE_SHUTDOWN)
    riskScore=100.0;
  if(riskScore>100.0)
    riskScore=100.0;
  if(riskScore<0.0)
    riskScore=0.0;
  batteryHealth=100.0-riskScore;
  if(batteryHealth<0.0)
    batteryHealth=0.0;
  if(batteryHealth>100.0)
    batteryHealth=100.0;
  calculateMaintenanceRecommendation();
}
void publishAnalytics()
{
  if(!Blynk.connected())
    return;
  unsigned long currentTime=millis();
  int uptimeMinutes=currentTime/60000UL;
  updateFaultFrequency();
  calculateRiskScore();
  Blynk.virtualWrite(V13,(int)round(riskScore));
  Blynk.virtualWrite(V14,analysis.averageSOC);
  Blynk.virtualWrite(V15,analysis.voltageImbalance*1000.0 );
  Blynk.virtualWrite(V16,faultFrequency);
  Blynk.virtualWrite(V17,uptimeMinutes);
  Blynk.virtualWrite(V18,maintenanceRecommendation);
  Blynk.virtualWrite(  V19,  (int)round(batteryHealth));
  Blynk.virtualWrite( V20, totalFaultCount);
  Blynk.virtualWrite(V21, getSystemStateCode(systemState));
  Blynk.virtualWrite( V22, stateTransitionCount);
  Blynk.virtualWrite( V23,maintenanceRecommendationText);
}
void printAnalytics()
{
  Serial.println();
  Serial.println("BATTERY ANALYTICS");
  Serial.print("Risk Score: ");
  Serial.print(riskScore,1);
  Serial.println(" %");
  Serial.print("Battery Health: ");
  Serial.print(batteryHealth,1);
  Serial.println(" %");
  Serial.print("Average SoC: ");
  Serial.print(analysis.averageSOC,1);
  Serial.println(" %");
  Serial.print("Voltage Imbalance: ");
  Serial.print(analysis.voltageImbalance*1000.0,1);
  Serial.println(" mV");
  Serial.print("Fault Frequency: ");
  Serial.print(faultFrequency,1);
  Serial.println(" faults/hour");
  Serial.print("Total Fault Count: ");
  Serial.println(totalFaultCount);
  Serial.print("Uptime: ");
  Serial.print(millis()/60000UL);
  Serial.println(" minutes");
  Serial.print("Recommendation Code: ");
  Serial.println(maintenanceRecommendation);
  Serial.print("Recommendation: ");
  Serial.println(maintenanceRecommendationText);
  Serial.print("System State: ");
  Serial.println(getSystemStateName(systemState));
  Serial.print("State Transition Count: ");
  Serial.println(stateTransitionCount);
}
bool hasBatteryFault()
{
  for(i=0;i<MAX_CELLS;i++)
  {
    if(cells[i].fault)
      return true;
  }
  return false;
}
bool hasCriticalFault()
{
  return adcFrozenFault|| relayMismatchFault||communicationFault;
}
void updateActiveFault()
{
  bool oldFaultActive=faultActive;
  if(adcFrozenFault)
  {
    activeFaultSource=FAULT_ADC;
    activeFaultID=FAULT_ID_ADC_FROZEN;
    activeFaultCell=0;
    faultActive=true;
  }
  else if(relayMismatchFault)
  {
    activeFaultSource=FAULT_RELAY;
    activeFaultID=FAULT_ID_RELAY_MISMATCH;
    activeFaultCell=0;
    faultActive=true;
  }
  else if(communicationFault)
  {
    activeFaultSource=FAULT_COMMUNICATION;
    activeFaultID=FAULT_ID_COMMUNICATION_LOSS;
    activeFaultCell=0;
    faultActive=true;
  }
  else if(hasBatteryFault())
  {
    activeFaultSource=FAULT_BATTERY;
    activeFaultID=FAULT_ID_BATTERY_CELL;
    activeFaultCell=0;
    for(i=0;i<MAX_CELLS;i++)
    {
      if(cells[i].fault)
      {
        activeFaultCell=cells[i].cell_id;
        break;
      }
    }
    faultActive=true;
  }
  else
  {
    clearFaultData();
  }
  if(faultActive&&!oldFaultActive)
    recordFaultOccurrence();
  previousFaultActive=faultActive;
}
void checkADCFrozen()
{
  int currentADCValue=analogRead(POT_PIN);
  if(adcFreezeTest)
  {
    if(adcFrozenCounter==0)
      previousADCValue=currentADCValue;
    adcFrozenCounter++;
    if(adcFrozenCounter>=adcFrozenLimit)
    {
      adcFrozenFault=true;
      setFault(FAULT_ADC,FAULT_ID_ADC_FROZEN);
    }
  }
  else
  {
    adcFrozenFault=false;
    adcFrozenCounter=0;
    previousADCValue=currentADCValue;
  }
}
void checkRelayMismatch()
{
  relayFeedbackState=digitalRead(RELAY_FEEDBACK_PIN);
  bool expectedRelayState=(digitalRead(RELAY_PIN)==HIGH);
  if(relayFeedbackState!=expectedRelayState)
    relayMismatchCounter++;
  else
    relayMismatchCounter=0;
  if(relayMismatchCounter>=relayMismatchLimit)
  {
    relayMismatchFault=true;
    setFault(FAULT_RELAY,FAULT_ID_RELAY_MISMATCH );
  }
  else
  {
    relayMismatchFault=false;
  }
}
void checkCommunication()
{
  if(!communicationMonitoring)
  {
    communicationFault=false;
    return;
  }
  if(millis()-lastCommunicationTime>=communicationTimeout)
  {
    communicationFault=true;
    setFault(FAULT_COMMUNICATION,FAULT_ID_COMMUNICATION_LOSS);
  }
  else
  {
    communicationFault=false;
  }
}
void clearLCDFault()
{
  if(!hasCriticalFault()&&!hasBatteryFault())
  {
    lcdFaultActive=false;
    lcdFaultMessage="";
    previousRow0="";
    previousRow1="";
    currentPage=0;
    lastPageChange=millis();
    Serial.println("LCD Fault Cleared" );
  }
  else
  {
    Serial.println("Fault still active - cannot clear");
  }
}
void processSerialCommand()
{
  if(Serial.available()>0)
  {
    char command=Serial.read();
    if(command=='H'||command=='h')
    {
      lastCommunicationTime=millis();
      communicationMonitoring=true;
      communicationFault=false;
      Serial.println("Communication Heartbeat Received");
    }
    else if(command=='F'||command=='f')
    {
      adcFreezeTest=!adcFreezeTest;
      if(!adcFreezeTest)
      {
        adcFrozenFault=false;
        adcFrozenCounter=0;
        previousADCValue=analogRead(POT_PIN);
        Serial.println("ADC Freeze Test: DISABLED");
      }
      else
      {
        adcFrozenCounter=0;
        previousADCValue=analogRead(POT_PIN);
        Serial.println("ADC Freeze Test: ENABLED");
      }
    }
    else if(command=='C'||command=='c')
    {
      clearLCDFault();
    }
    else if(command=='R'||command=='r')
    {
      if(systemState==STATE_SHUTDOWN&&!hasCriticalFault()&&!hasBatteryFault())
      {
        SystemState previousState=systemState;
        systemState=STATE_NORMAL;
        recoveryVerificationActive=false;
        failsafeStartTime=0;
        shutdownStartTime=0;
        digitalWrite(RELAY_PIN,HIGH);
        relayState=RELAY_NORMAL;
        clearFaultData();
        logStateTransition(previousState,systemState,FAULT_ID_NONE);
        Serial.println( "System Reset to NORMAL");
      }
      else
      {
        Serial.println("Reset rejected");
      }
    }
    else if(command=='N'||command=='n')
    {
      networkOutageTest=!networkOutageTest;
      if(networkOutageTest)
      {
        WiFi.disconnect();
        wifiState=  WIFI_DISCONNECTED;
        if(Blynk.connected())Blynk.disconnect();
        Serial.println("Network outage test: ENABLED");
      }
      else
      {
        Serial.println("Network outage test: DISABLED");
        startWiFiConnection();
      }
    }
  }
}
void updateSystemState()
{
  bool batteryFault=hasBatteryFault();
  bool criticalFault=hasCriticalFault();
  updateActiveFault();
  if(systemState==STATE_SHUTDOWN)
  {
    digitalWrite(RELAY_PIN,LOW);
    relayState=RELAY_TRIP;
    return;
  }
  if(systemState==STATE_NORMAL)
  {
    if(criticalFault)
    {
      SystemState previousState=systemState;
      systemState=STATE_FAILSAFE;
      digitalWrite(RELAY_PIN,LOW);
      relayState=RELAY_TRIP;
      failsafeStartTime=millis();
      recoveryVerificationActive=false;
      logStateTransition(previousState,systemState,activeFaultID);
    }
    else if(batteryFault)
    {
      SystemState previousState=systemState;
      systemState=STATE_DEGRADED;
      logStateTransition(previousState,systemState,activeFaultID);
    }
  }
  else if(systemState==STATE_DEGRADED)
  {
    if(criticalFault)
    {
      SystemState previousState=systemState;
      systemState=STATE_FAILSAFE;
      digitalWrite(RELAY_PIN,LOW);
      relayState=RELAY_TRIP;
      failsafeStartTime=millis();
      recoveryVerificationActive=false;
      logStateTransition(previousState,systemState,activeFaultID);
    }
    else if(!batteryFault)
    {
      SystemState previousState=systemState;
      systemState=STATE_NORMAL;
      clearFaultData();
      logStateTransition(previousState,systemState,FAULT_ID_NONE);
    }
  }
  else if(systemState==STATE_FAILSAFE)
  {
    if(criticalFault)
    {
      recoveryVerificationActive=false;
      systemRecoveryStartTime=0;
      digitalWrite(RELAY_PIN,LOW);
      relayState=RELAY_TRIP;
      if(failsafeStartTime==0)
        failsafeStartTime=millis();
      if(
        millis()-failsafeStartTime>=shutdownTimeout
      )
      {
        SystemState previousState=systemState;
        systemState=STATE_SHUTDOWN;
        shutdownStartTime=millis();
        logStateTransition(previousState,systemState,activeFaultID);
      }
    }
    else if(!batteryFault)
    {
      if(!recoveryVerificationActive)
      {
        recoveryVerificationActive=true;
        systemRecoveryStartTime=millis();
        digitalWrite(RELAY_PIN,LOW);
        Serial.println("FAILSAFE Recovery Verification Started");
      }
      else if(millis()-systemRecoveryStartTime>=failsafeVerificationTime)
      {
        SystemState previousState=systemState;
        systemState=STATE_NORMAL;
        recoveryVerificationActive=false;
        failsafeStartTime=0;
        systemRecoveryStartTime=0;
       digitalWrite(RELAY_PIN,HIGH);
        relayState=RELAY_NORMAL;
        clearFaultData();
        logStateTransition(previousState,systemState,FAULT_ID_NONE);
        Serial.println("FAILSAFE Recovery Verification Passed");
      }
    }
    else
    {
      recoveryVerificationActive=false;
      systemRecoveryStartTime=0;
      digitalWrite(RELAY_PIN,LOW);
      relayState=RELAY_TRIP;
    }
  }
}
void updateRelayState()
{
  bool faultPresent=false;
  bool safeForRecovery=true;
  for(i=0;i<MAX_CELLS;i++)
  {
    if(
      cells[i].fault||cells[i].sensorFrozen||cells[i].unrealisticValue)
    {
      faultPresent=true;
      safeForRecovery=false;
    }
    if(cells[i].Overheat&&cells[i].temperature>clear_temperature)
      safeForRecovery=false;
    if(cells[i].Overvoltage&&cells[i].voltage>max_voltage-0.10)
      safeForRecovery=false;
    if(cells[i].Undervoltage&&cells[i].voltage<min_voltage+0.10)
      safeForRecovery=false;
  }
  switch(relayState)
  {
    case RELAY_NORMAL:
      if(faultPresent)
      {
        if(!faultTimerActive)
        {
          faultStartTime=millis();
          faultTimerActive=true;
          Serial.println("Relay Fault Timer Started");
        }
        if(millis()-faultStartTime>=relayDebounceTime)
        {
          relayState=RELAY_TRIP;
          digitalWrite(RELAY_PIN,LOW);
          faultTimerActive=false;
          Serial.println("Relay State: TRIPPED");
        }
      }
      else
      {
        faultTimerActive=false;
      }
      break;
    case RELAY_TRIP:
      if(!faultPresent&&safeForRecovery&&systemState!=STATE_FAILSAFE&&systemState!=STATE_SHUTDOWN)
      {
        relayState=RELAY_RECOVERY;
        relayRecoveryStartTime=millis();
        digitalWrite(RELAY_PIN,LOW);
        Serial.println("Relay State: RECOVERY");
      }
      break;
    case RELAY_RECOVERY:
      if(faultPresent||!safeForRecovery)
      {
        relayState=RELAY_TRIP;
        digitalWrite(RELAY_PIN,LOW);
        Serial.println("Relay State: TRIPPED");
      }
      else if(
        millis()-relayRecoveryStartTime>=relayRecoveryTime&&systemState!=STATE_FAILSAFE&&systemState!=STATE_SHUTDOWN)
      {
        relayState=RELAY_NORMAL;
        digitalWrite(RELAY_PIN,HIGH);
        Serial.println("Relay State: NORMAL");
      }
      break;
  }
}
void updateCellVoltages()
{
  int potValue=analogRead(POT_PIN);
  float baseVoltage=min_voltage+((float)potValue/4095.0)*(max_voltage-min_voltage);
  float cellOffsets[MAX_CELLS];
  for(i=0;i<MAX_CELLS;i++)
    cellOffsets[i]=0.0;
  if(MAX_CELLS>=2)
    cellOffsets[1]=-0.03;
  if(MAX_CELLS>=3)
    cellOffsets[2]=0.05;
  if(MAX_CELLS>=4)
    cellOffsets[3]=-0.01;
  for(i=0;i<MAX_CELLS;i++)
  {
    cells[i].voltage=baseVoltage+cellOffsets[i];
    if(cells[i].voltage>max_voltage)cells[i].voltage=max_voltage;
    if(cells[i].voltage<min_voltage)cells[i].voltage=min_voltage;
  }
}
void calculateSOC()
{
  for(i=0;i<MAX_CELLS;i++)
  {
    cells[i].soc=((cells[i].voltage-min_voltage)/(max_voltage-min_voltage))*100.0;
    if(cells[i].soc>100.0)cells[i].soc=100.0;
    if(cells[i].soc<0.0)cells[i].soc=0.0;
  }
}
void findStrongestWeakest()
{
  highest_voltage=0;
  lowest_voltage=0;
  for(i=1;i<MAX_CELLS;i++)
  {
    if(cells[i].voltage>cells[highest_voltage].voltage)highest_voltage=i;
    if(cells[i].voltage<cells[lowest_voltage].voltage)lowest_voltage=i;
  }
  analysis.strongestCell=cells[highest_voltage].cell_id;
  analysis.weakestCell=cells[lowest_voltage].cell_id;
  analysis.maximumVoltage=cells[highest_voltage].voltage;
  analysis.minimumVoltage=cells[lowest_voltage].voltage;
}
void calculateVoltageImbalance()
{
  analysis.voltageImbalance=analysis.maximumVoltage-analysis.minimumVoltage;
}
void monitorImbalanceTrend()
{
  analysis.imbalanceChange=analysis.voltageImbalance-analysis.previousImbalance;
  if(analysis.imbalanceChange>0.001)
  {
    analysis.imbalanceIncreasing=true;
    Serial.println(  "Imbalance Trend: INCREASING");
  }
  else if(analysis.imbalanceChange<-0.001)
  {
    analysis.imbalanceIncreasing=false;
    Serial.println("Imbalance Trend: DECREASING");
  }
  else
  {
    analysis.imbalanceIncreasing=false;
    Serial.println("Imbalance Trend: STABLE");
  }
  analysis.previousImbalance=analysis.voltageImbalance;
}
void calculateAdaptiveThershold()
{
  float total_soc=0.0;
  for(i=0;i<MAX_CELLS;i++)
    total_soc+=cells[i].soc;
  analysis.averageSOC=total_soc/MAX_CELLS;
  if(analysis.averageSOC>80.0)
    analysis.adaptiveThreshold=0.05;
  else if(analysis.averageSOC>50.0)
    analysis.adaptiveThreshold=0.10;
  else
    analysis.adaptiveThreshold=0.15;
  analysis.imbalanceWarning=analysis.voltageImbalance>analysis.adaptiveThreshold;
  if(analysis.imbalanceWarning)
    Serial.println("Imbalance Warning: YES");
  else
    Serial.println("Imbalance Warning: NO");
}
void checkCellFaults()
{
  for(i=0;i<MAX_CELLS;i++)
  {
    cells[i].Overvoltage=false;
    cells[i].Undervoltage=false;
    cells[i].Overheat=false;
    if(
      cells[i].voltage>max_voltage)
    {
      cells[i].fault=true;
      cells[i].Overvoltage=true;
    }
    else if(cells[i].voltage<min_voltage)
    {
      cells[i].fault=true;
      cells[i].Undervoltage=true;
    }
    else
    {
      cells[i].fault=false;
    }
  }
  for(i=0;i<MAX_CELLS;i++)
  {
    cells[i].Overheat=false;
    if(cells[i].temperature>max_temperature)
    {
      cells[i].fault=true;
      cells[i].Overheat=true;
    }
  }
  for(i=0;i<MAX_CELLS;i++)
  {
    cells[i].fault=cells[i].Overvoltage||cells[i].Undervoltage||cells[i].Overheat;
  }
}
void checkSensorAnomalies()
{
  int potValue=analogRead(POT_PIN);
  float currentBaseVoltage=min_voltage+((float)potValue/4095.0)*(max_voltage-min_voltage);
  float baseChange=currentBaseVoltage-previousBaseVoltage;
  for(i=0;i<MAX_CELLS;i++)
  {
    cells[i].sensorFrozen=false;
    cells[i].unrealisticValue=false;
    cells[i].sensorNoise=false;
    cells[i].genuineRapidChange=false;
    float change=cells[i].voltage-previousVoltage[i];
    float absoluteChange=abs(change);
    if(previousVoltage[i]>0.0)
    {
      if(abs(baseChange)>0.01&&absoluteChange<0.001
      )
        frozenCounter[i]++;
      else
        frozenCounter[i]=0;

      if(frozenCounter[i]>=frozenLimit)
      {
        cells[i].sensorFrozen=true;
        cells[i].fault=true;
        Serial.print("Cell ");
        Serial.print(i+1);
        Serial.println(" Sensor Anomaly: FROZEN");
      }
      if(absoluteChange>unrealisticJump)
      {
        cells[i].unrealisticValue=true;
        cells[i].fault=true;
        rapidChangeCounter[i]=0;
        Serial.print("Cell ");
        Serial.print(i+1);
        Serial.println(" Sensor Anomaly: UNREALISTIC JUMP");
      }
      else if(abs(baseChange)>0.01&&absoluteChange>noiseLimit&&absoluteChange<=unrealisticJump)
      {
        rapidChangeCounter[i]++;
        if(
          rapidChangeCounter[i]>=1)
        {
          cells[i].genuineRapidChange=true;
          Serial.print("Cell ");
          Serial.print(i+1);
          Serial.println(" Rapid Change: GENUINE");
        }
      }
      else
      {
        rapidChangeCounter[i]=0;
      }
    }
    if(cells[i].voltage<min_voltage||cells[i].voltage>max_voltage)
    {
      cells[i].unrealisticValue=true;
      cells[i].fault=true;
      Serial.print( "Cell ");
      Serial.print(i+1);
      Serial.println(" Sensor Anomaly: OUT OF RANGE");
    }
    previousChange[i]=change;
    previousVoltage[i]=cells[i].voltage;
  }
  previousBaseVoltage=currentBaseVoltage;
}
const BMSAnalysis& getBMSAnalysis()
{
  return analysis;
}
const Cell* getCellData()
{
  return cells;
}
int getCellCount()
{
  return MAX_CELLS;
}
void updateLCDRow(
  int row,
  String text)
{
  while(text.length()<16)text+=" ";
  if(text.length()>16)
    text=text.substring(0,16);
  if(row==0)
  {
    if(text!=previousRow0)
    {
      lcd.setCursor(0,0);
      lcd.print(text);
      previousRow0=text;
    }
  }
  else
  {
    if(text!=previousRow1)
    {
      lcd.setCursor(0,1);
      lcd.print(text);
      previousRow1=text;
    }
  }
}
void displayBatteryPage()
{
  float averageSOH=0.0;
  for(i=0;i<MAX_CELLS;i++)
    averageSOH+=cells[i].soh;
  averageSOH/=MAX_CELLS;
  String row0="BATTERY STATUS";
  String row1="SOC:";
  row1+=String(analysis.averageSOC,0);
  row1+="% SOH:";
  row1+=String(averageSOH,0);
  row1+="%";
  updateLCDRow(0,row0);
  updateLCDRow(1,row1);
}
void displaySystemPage()
{
  String row0="SYSTEM: ";
  if(relayState==RELAY_NORMAL)
    row0+="NORMAL";
  else if(relayState==RELAY_TRIP)
    row0+="TRIPPED";
  else
    row0+="RECOVERY";
  String row1="CELLS:";
  row1+=String(MAX_CELLS);
  row1+=" FAULT:";
  bool faultPresent=false;
  for(i=0;i<MAX_CELLS;i++)
  {
    if(cells[i].fault)
    {
      faultPresent=true;
      break;
    }
  }
  if(faultPresent)
    row1+="YES";
  else
    row1+="NO";
  updateLCDRow(0,row0);
  updateLCDRow(1,row1);
}
void displayTelemetryPage()
{
  String row0 = "MIN:";
  row0 += String(analysis.minimumVoltage, 2);
  row0 += " MAX:";
  row0 += String(analysis.maximumVoltage, 2);
  String row1 = "IMB:";
  row1 += String(analysis.voltageImbalance * 1000.0, 0);
  row1 += "mV SOC:";
  row1 += String(analysis.averageSOC, 0);
  updateLCDRow(0, row0);
  updateLCDRow(1, row1);
}
void displayFaultPage()
{
  updateLCDRow(0, "!! CRITICAL !!");
  updateLCDRow(1, lcdFaultMessage);
}
void checkLCDCriticalFault()
{
  if (lcdFaultActive) return;
  for (i = 0; i < MAX_CELLS; i++)
  {
    if (cells[i].Overheat)
    {
      lcdFaultActive = true;
      lcdFaultMessage = "CELL " + String(i + 1) + " OVERHEAT";
      previousRow0 = "";
      previousRow1 = "";
      return;
    }
    if (cells[i].Overvoltage)
    {
      lcdFaultActive = true;
      lcdFaultMessage = "CELL " + String(i + 1) + " OVERVOLT";
      previousRow0 = "";
      previousRow1 = "";
      return;
    }
    if (cells[i].Undervoltage)
    {
      lcdFaultActive = true;
      lcdFaultMessage = "CELL " + String(i + 1) + " UNDERVOLT";
      previousRow0 = "";
      previousRow1 = "";
      return;
    }
  if (cells[i].sensorFrozen)
    {
      lcdFaultActive = true;
      lcdFaultMessage = "CELL " + String(i + 1) + " SENSOR";
      previousRow0 = "";
      previousRow1 = "";
      return;
    }
 if (cells[i].unrealisticValue)
    {
      lcdFaultActive = true;
      lcdFaultMessage = "CELL " + String(i + 1) + " INVALID";
      previousRow0 = "";
      previousRow1 = "";
      return;
    }
  }
}
void updateLCD()
{
  unsigned long currentTime = millis();
  if (lcdFaultActive)
  {
    displayFaultPage();
    return;
  }
  if (currentTime - lastPageChange >= pageInterval)
  {
    lastPageChange = currentTime;
    currentPage++;
    if (currentPage >= 3) currentPage = 0;
    previousRow0 = "";
    previousRow1 = "";
  }
  if (currentTime - lastLCDUpdate >= lcdRefreshInterval)
  {
    lastLCDUpdate = currentTime;
    if (currentPage == 0)
      displayBatteryPage();
    else if (currentPage == 1)
      displaySystemPage();
    else
      displayTelemetryPage();
  }
}
void printBMSAnalysis()
{
  const BMSAnalysis& result = getBMSAnalysis();
  Serial.println();
  Serial.println("BATTERY ANALYSIS");
  Serial.print("Strongest Cell: ");
  Serial.println(result.strongestCell);
  Serial.print("Strongest Voltage: ");
  Serial.print(result.maximumVoltage, 3);
  Serial.println(" V");
  Serial.print("Weakest Cell: ");
  Serial.println(result.weakestCell);
  Serial.print("Weakest Voltage: ");
  Serial.print(result.minimumVoltage, 3);
  Serial.println(" V");
  Serial.print("Voltage Imbalance: ");
  Serial.print(result.voltageImbalance * 1000.0, 1);
  Serial.println(" mV");
  Serial.print("Imbalance Change: ");
  Serial.print(result.imbalanceChange * 1000.0, 1);
  Serial.println(" mV");
  Serial.print("Average SoC: ");
  Serial.print(result.averageSOC, 1);
  Serial.println(" %");
  Serial.print("Adaptive Threshold: ");
  Serial.print(result.adaptiveThreshold * 1000.0, 1);
  Serial.println(" mV");
  Serial.print("Imbalance Warning: ");
  if (result.imbalanceWarning)
    Serial.println("YES");
  else
    Serial.println("NO");
  Serial.println();
  Serial.println("CELL DATA");
  for (i = 0; i < MAX_CELLS; i++)
  {
    Serial.print("Cell ");
    Serial.print(cells[i].cell_id);
    Serial.print(" | Voltage: ");
    Serial.print(cells[i].voltage);
    Serial.print(" V | Temperature: ");
    Serial.print(cells[i].temperature);
    Serial.print(" C | SoC: ");
    Serial.print(cells[i].soc);
    Serial.print(" % | Fault: ");
    Serial.println(cells[i].fault);
  }
}
bool isMeaningfulEvent(const TelemetryEvent& currentEvent)
{
  if (!telemetryHistoryValid) return true;
  for (i = 0; i < MAX_CELLS; i++)
  {
    if (abs(currentEvent.cellVoltage[i] - lastTelemetryEvent.cellVoltage[i]) >= CELL_VOLTAGE_EVENT_THRESHOLD)
    {
      Serial.print("Event reason: CELL ");
      Serial.println(i + 1);
      return true;
    }
  }
  if (currentEvent.weakestCell != lastTelemetryEvent.weakestCell)
  {
    Serial.println("Event reason: WEAKEST CELL");
    return true;
  }
  if (currentEvent.strongestCell != lastTelemetryEvent.strongestCell)
  {
    Serial.println("Event reason: STRONGEST CELL");
    return true;
  }
  if (currentEvent.relayNormal != lastTelemetryEvent.relayNormal)
  {
    Serial.println("Event reason: RELAY");
    return true;
  }
  if (currentEvent.faultActive != lastTelemetryEvent.faultActive)
  {
    Serial.println("Event reason: FAULT STATE");
    return true;
  }
  if (currentEvent.systemState != lastTelemetryEvent.systemState)
  {
    Serial.println("Event reason: SYSTEM STATE");
    return true;
  }
  if (currentEvent.faultSource != lastTelemetryEvent.faultSource)
  {
    Serial.println("Event reason: FAULT SOURCE");
    return true;
  }
  if (currentEvent.faultID != lastTelemetryEvent.faultID)
  {
    Serial.println("Event reason: FAULT ID");
    return true;
  }
  return false;
}
TelemetryEvent createTelemetryEvent()
{
  TelemetryEvent event;
  event.eventId = nextEventId++;
  event.timestamp = millis();
  for (i = 0; i < MAX_CELLS; i++)
  event.cellVoltage[i] = cells[i].voltage;
  event.weakestCell = analysis.weakestCell;
  event.strongestCell = analysis.strongestCell;
  event.relayNormal = (relayState == RELAY_NORMAL);
  event.faultActive = faultActive;
  event.systemState = systemState;
  event.faultSource = activeFaultSource;
  event.faultID = activeFaultID;
  event.rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : -127;
  return event;
}
void enqueueTelemetry(const TelemetryEvent& event)
{
  if (queueCount >= TELEMETRY_QUEUE_SIZE)
  {
    Serial.println("Telemetry queue full - new event discarded");
    return;
  }
  telemetryQueue[queueTail] = event;
  queueTail = (queueTail + 1) % TELEMETRY_QUEUE_SIZE;
  queueCount++;
  Serial.print("Telemetry queued | Event: ");
  Serial.print(event.eventId);
  Serial.print(" | Depth: ");
  Serial.println(queueCount);
}
bool peekTelemetry(TelemetryEvent& event)
{
  if (queueCount == 0) return false;
  event = telemetryQueue[queueHead];
  return true;
}
bool dequeueTelemetry(TelemetryEvent& event)
{
  if (queueCount == 0) return false;
  event = telemetryQueue[queueHead];
  queueHead = (queueHead + 1) % TELEMETRY_QUEUE_SIZE;
  queueCount--;
  return true;
}
int getTelemetryQueueDepth()
{
  return queueCount;
}
const char* getWiFiHealthName(int rssi)
{
  if (rssi == -127) return "OFFLINE";
  if (rssi > -60) return "GOOD";
  if (rssi > -75) return "FAIR";
  return "WEAK";
}
bool telemetryLinkAvailable()
{
  return WiFi.status() == WL_CONNECTED && Blynk.connected() && !networkOutageTest;
}
void startWiFiConnection()
{
  if (networkOutageTest) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
  wifiState = WIFI_CONNECTING;
  wifiAttemptTime = millis();
  Serial.println("WiFi state: CONNECTING");
}
void updateWiFiState()
{
  if (networkOutageTest) return;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (wifiState != WIFI_CONNECTED)
    {
      wifiState = WIFI_CONNECTED;
      Serial.print("WiFi state: CONNECTED | RSSI:");
      Serial.println(WiFi.RSSI());
    }
    return;
  }
  if (wifiState == WIFI_CONNECTED)
  {
    wifiState = WIFI_DISCONNECTED;
    if (Blynk.connected())
      Blynk.disconnect();
    Serial.println("WiFi state: LOST");
  }
  if (wifiState != WIFI_CONNECTING || millis() - wifiAttemptTime >= wifiRetryInterval)
    startWiFiConnection();
}
void updateBlynkConnection()
{
  if (networkOutageTest) 
  return;
  if (WiFi.status() != WL_CONNECTED) 
  return;
  if (Blynk.connected()) return;
  if (millis() - lastBlynkAttempt >= blynkRetryInterval)
  {
    lastBlynkAttempt = millis();
    Serial.println("Blynk: connection attempt");
    if (Blynk.connect(5000))
      Serial.println("Blynk: CONNECTED");
    else
      Serial.println("Blynk: not connected");
  }
}
bool sendTelemetryEvent(const TelemetryEvent& event, bool queuedData)
{
  if (!telemetryLinkAvailable()) 
  return false;
  int currentRSSI = WiFi.RSSI();
  Blynk.virtualWrite(V0, event.cellVoltage[0]);
  Blynk.virtualWrite(V1, event.cellVoltage[1]);
  Blynk.virtualWrite(V2, event.cellVoltage[2]);
  Blynk.virtualWrite(V3, event.cellVoltage[3]);
  Blynk.virtualWrite(V4, event.weakestCell);
  Blynk.virtualWrite(V5, event.strongestCell);
  Blynk.virtualWrite(V6, event.relayNormal ? 1 : 0);
  Blynk.virtualWrite(V7, event.faultActive ? 1 : 0);
  Blynk.virtualWrite(V8, getSystemStateCode(event.systemState));
  Blynk.virtualWrite(V9, currentRSSI);
  Blynk.virtualWrite(V10, queueCount);
  Blynk.virtualWrite(V11, queuedData ? 1 : 0);
  Blynk.virtualWrite(V12, event.eventId);
  Blynk.virtualWrite(V13, (int)round(riskScore));
  Blynk.virtualWrite(V14, analysis.averageSOC);
  Blynk.virtualWrite(V15, analysis.voltageImbalance * 1000.0);
  Blynk.virtualWrite(V16, faultFrequency);
  Blynk.virtualWrite(V17, millis() / 60000UL);
  Blynk.virtualWrite(V18, maintenanceRecommendation);
  Blynk.virtualWrite(V19, (int)round(batteryHealth));
  Blynk.virtualWrite(V20, totalFaultCount);
  Blynk.virtualWrite(V21, getSystemStateCode(event.systemState));
  Blynk.virtualWrite(V22, stateTransitionCount);
  Blynk.virtualWrite(V23, maintenanceRecommendationText);
  Serial.print(queuedData ? "Queued event sent | " : "Live event sent | ");
  Serial.print("Event:");
  Serial.print(event.eventId);
  Serial.print(" | RSSI:");
  Serial.print(currentRSSI);
  Serial.print(" | Queue:");
  Serial.println(queueCount);
  return true;
}
void flushOfflineQueue()
{
  if (!telemetryLinkAvailable()) return;
  if (queueCount == 0) return;
  if (millis() - lastQueueReplay < queueReplayInterval) 
  return;
  TelemetryEvent event;
  if (peekTelemetry(event))
  {
    if (sendTelemetryEvent(event, true))
    {
      dequeueTelemetry(event);
      lastQueueReplay = millis();
      if (queueCount == 0)
      {
        Blynk.virtualWrite(V11, 0);
        Serial.println("Offline queue cleared");
      }
    }
  }
}
void processTelemetryEvent()
{
  TelemetryEvent currentEvent = createTelemetryEvent();
  if (!isMeaningfulEvent(currentEvent))
   return;
  lastTelemetryEvent = currentEvent;
  telemetryHistoryValid = true;
  Serial.print("Meaningful event detected | Event: ");
  Serial.println(currentEvent.eventId);
  if (telemetryLinkAvailable())
  {
    if (!sendTelemetryEvent(currentEvent, false))
      enqueueTelemetry(currentEvent);
  }
  else
  {
    enqueueTelemetry(currentEvent);
  }
}
void setup()
{
  Serial.begin(115200);
  Serial.println("Smart Battery Management system");
  Wire.begin(21,22);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("BMS SYSTEM");
  lcd.setCursor(0,1);
  lcd.print("LCD READY");
  pinMode(POT_PIN,INPUT);
  pinMode(RELAY_PIN,OUTPUT);
  pinMode(RELAY_FEEDBACK_PIN,INPUT);
  digitalWrite(RELAY_PIN,HIGH);
  Serial.println("Relay State: NORMAL");
  for(i=0;i<MAX_CELLS;i++)
  {
    cells[i].cell_id=i+1;
    cells[i].voltage=0.0;
    cells[i].temperature=25.0;
    cells[i].fault=false;
    cells[i].Overvoltage=false;
    cells[i].Undervoltage=false;
    cells[i].Overheat=false;
    cells[i].sensorFrozen=false;
    cells[i].unrealisticValue=false;
    cells[i].sensorNoise=false;
    cells[i].genuineRapidChange=false;
    cells[i].soh=100;
    cells[i].soc=100;
    previousVoltage[i]=0.0;
    previousChange[i]=0.0;
    frozenCounter[i]=0;
    rapidChangeCounter[i]=   0;
  }
  analysis.weakestCell=1;
  analysis.strongestCell=1;
  analysis.minimumVoltage=0.0;
  analysis.maximumVoltage=0.0;
  analysis.voltageImbalance=0.0;
  analysis.previousImbalance=0.0;
  analysis.imbalanceChange=0.0;
  analysis.imbalanceIncreasing=false;
  analysis.averageSOC=0.0;
  analysis.adaptiveThreshold=0.0;
  analysis.imbalanceWarning=false;
  systemStartTime=millis();
  lastAnalyticsUpdate=millis();
  previousFaultActive=false;
  previousLoggedState=STATE_NORMAL;
  Serial.print("Number of cells: ");
  Serial.println(MAX_CELLS);
  Serial.println("BMS Engine Initialized");
  Serial.println("State Machine Initialized");
  Serial.println("Telemetry Engine Initialized");
  Serial.println("Analytics Engine Initialized");
  Serial.println("Commands: H=Heartbeat F=ADC Test C=Clear R=Reset N=Network Test");
  lcd.setCursor(0,0);
  lcd.print("    ");
  lcd.setCursor(0,1);
  lcd.print("  ");
  previousRow0="";
  previousRow1="";
  lastPageChange=millis();
  lastLCDUpdate=millis();
  lastCommunicationTime=millis();
  previousADCValue=analogRead(POT_PIN);
  Blynk.config(BLYNK_AUTH_TOKEN);
  startWiFiConnection();
}
void loop()
{
  static unsigned long lastUpdate=0;
  const unsigned long updateInterval=1000;
  processSerialCommand();
  updateWiFiState();
  updateBlynkConnection();
  if(Blynk.connected())
    Blynk.run();
  if(millis()-lastUpdate>=updateInterval)
  {
    lastUpdate=millis();
    Serial.println("Monitoring Battery");
    updateCellVoltages();
    calculateSOC();
    findStrongestWeakest();
    calculateVoltageImbalance();
    monitorImbalanceTrend();
    calculateAdaptiveThershold();
    checkCellFaults();
    checkSensorAnomalies();
    checkADCFrozen();
    checkRelayMismatch();
    checkCommunication();
    updateRelayState();
    updateSystemState();
    updateFaultFrequency();
    calculateRiskScore();
    checkLCDCriticalFault();
    processTelemetryEvent();
    printBMSAnalysis();
    Serial.print("System State: ");
    Serial.println(getSystemStateName(systemState)
    );
    Serial.print("WiFi: ");
    Serial.print(getWiFiHealthName(WiFi.status()==WL_CONNECTED? WiFi.RSSI():-127));
    Serial.print(" | RSSI: ");
    if(WiFi.status()==WL_CONNECTED)
    Serial.println(WiFi.RSSI());
    else
      Serial.println("OFFLINE");
    Serial.print("Offline Queue Depth: ");
    Serial.println(getTelemetryQueueDepth());
printAnalytics();
  }
  if(millis()-lastAnalyticsUpdate>=analyticsInterval)
  {
    lastAnalyticsUpdate=millis();
  publishAnalytics();
  flushOfflineQueue();
  updateLCD();
}
}

#include <LowPower.h>
#include <SoftwareSerial.h>
#include <DHT.h>

#define periodMinutes 20
#define timeout 30000
#define attempts 3

SoftwareSerial gsm(2, 3); //SIM800L Tx & Rx is connected to Arduino #2 & #3
DHT dht(4, DHT22); // Initialize DHT sensor for normal 16mhz Arduino

byte mins, remains = periodMinutes;

float hum;  //Stores humidity value
float temp; //Stores temperature value

void setup()
{
  Serial.begin(9600);
  gsm.begin(9600);
  dht.begin();
}

void loop()
{
  attempt();

  Serial.print(F("Sleeping for "));
  Serial.print(remains);
  Serial.println(F(" minute(s)"));
  Serial.flush();
  
  deepSleep(remains);
}

void attempt() {
  String message = "1:";
  Serial.println(F("Reading sensors..."));
  hum = dht.readHumidity();
  temp = dht.readTemperature();

  message.concat(hum);
  message.concat(";2:");
  message.concat(temp);
  
  for(int i = 1; i <= attempts; i++) {
    Serial.print(F("Attempt #"));
    Serial.println(i);
  
    if (sendInfoToGSM(&message)) {
      Serial.println(F("SUCCESS"));
      break;
    }
    deepSleep(1);
  }
}

void deepSleep(unsigned int minutes) {
  unsigned int sleepCounter = minutes * 15; // 60 / 4
  for (unsigned int i = 0; i < sleepCounter; i++) {
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
  }
}

void waitForResponse(char *buff) {
  unsigned long started = millis();
  memset(buff, 0, sizeof(buff));
  
  while(!gsm.available()) {
    if (millis() - started > timeout) {
      Serial.println(F("TIMED OUT!"));
      break;
    }
  }
  
  if (gsm.available()) {
    strcpy(buff, gsm.readString().c_str());
    Serial.print(*buff);
  }
}

boolean sendInfoToGSM(String *message)
{
  int idx;
  boolean success = false;
  char buff[64];

  Serial.println("Resetting modem..."); 
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  delay(200);
  pinMode(5, INPUT);
  delay(15000);
 
  gsm.println(F("AT"));
  waitForResponse(buff);
  if (strstr(buff, "OK") == NULL) {
    Serial.println(F("Not connected to GSM modem"));
    return false;
  }

  gsm.println(F("AT+CNUM"));
  waitForResponse(buff);
  
  char mynum[16];
  memset(mynum, 0, sizeof(mynum));
  strcpy(mynum, "%2b");
  char *cnum = strstr(buff, "+CNUM:");
  if (cnum != NULL) {
    memcpy(mynum + 3, cnum, 12);
  }
  Serial.println(mynum);
  
  gsm.println(F("AT+CREG?"));
  waitForResponse(buff);
  if (strstr(buff, "+CREG: 0,1") == NULL) {
    Serial.println(F("Not connected to the network"));
    return false;
  }

  gsm.println(F("AT+SAPBR=1,1")); //Connecting to the Internet
  delay(6000);
  waitForResponse(buff);
  gsm.println(F("AT+SAPBR=2,1"));
  waitForResponse(buff);
  gsm.println(F("AT+HTTPINIT"));
  waitForResponse(buff);
  gsm.println(F("AT+HTTPPARA=\"CID\",1"));
  waitForResponse(buff);
  gsm.print(F("AT+HTTPPARA=\"URL\",\"http://18XXXXX.eg3XXXXX.web.hosting-test.net/api/?token=1234567890&source="));
  gsm.print(mynum);
  gsm.print(F("&message="));
  gsm.print(*message); 
  gsm.println("\"");
  waitForResponse(buff);
  
  gsm.print(F("AT+HTTPSSL=0"));
  waitForResponse(buff);
  
  gsm.println(F("AT+HTTPACTION=0"));
  waitForResponse(buff);
  
  if (strstr(buff, "ERROR") == NULL) {
    waitForResponse(buff);
    if (strstr(buff, "200") != NULL) {
      gsm.println(F("AT+HTTPREAD"));
      waitForResponse(buff);
      char *now = strstr(buff, "NOW:");
      if (now != NULL) {
        char minutes[3];
        memset(minutes, 0, sizeof(minutes));
        memcpy(minutes, now + 14, 2);
        Serial.println(minutes);
        mins = atoi(minutes);
        remains = periodMinutes - mins % periodMinutes;
      } 
      success = true;
    }
  }
  
  gsm.println(F("AT+HTTPTERM"));
  waitForResponse(buff);
  gsm.println(F("AT+SAPBR=0,1"));
  delay(6000);
  waitForResponse(buff);

  gsm.println(F("AT+CFUN=0"));
  waitForResponse(buff);
  waitForResponse(buff);
  
  gsm.println(F("AT+CSCLK=2"));
  waitForResponse(buff);
  delay(6000);

  return success;
}

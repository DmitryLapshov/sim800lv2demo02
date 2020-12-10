#include <LowPower.h>
#include <SoftwareSerial.h>
#include <DHT.h>

#define periodMinutes 20
#define timeout 30000
#define attempts 3

SoftwareSerial gsm(2, 3); //SIM800L Tx & Rx is connected to Arduino #2 & #3
DHT dht(4, DHT22); // Initialize DHT sensor for normal 16mhz Arduino

const String apn = "internet";
const String url = "http://18XXXXX.eg3XXXXX.web.hosting-test.net/api/";
const String token = "1234567890";

byte mins, remains;
String buff, mynumber, serverTime;
bool success;
unsigned int sleepCounter;

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

  sleepCounter = remains * 60 / 4;
  for (unsigned int i = 0; i < sleepCounter; i++) {
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
  }
}

void attempt() {
  for(int i = 1; i <= attempts; i++) {
    Serial.print(F("Attempt #"));
    Serial.println(i);
    sendInfoToGSM();
    if (success) {
      Serial.println(F("SUCCESS"));
      break;
    }
    delay(60000);
  }
}

void waitForResponse() {
  if (wait()) {
    buff = gsm.readString();
    Serial.print(buff);
  }
  else {
    buff = "";
  }
}

bool wait()
{
  unsigned long started = millis();
  
  while(!gsm.available()) {
    if (millis() - started > timeout) {
      Serial.println("Timed out!");
      break;
    }
  }

  return gsm.available();
}

void sendInfoToGSM()
{
  int idx;
  success = false;

  Serial.println(F("Reading sensors..."));
  hum = dht.readHumidity();
  temp = dht.readTemperature();

  Serial.println("Resetting modem..."); 
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  delay(500);
  pinMode(5, INPUT);
  delay(20000);
 
  gsm.println(F("AT"));
  waitForResponse();
  if (buff.indexOf("OK") == -1) {
    Serial.println(F("Not connected to GSM modem"));
    return;
  }

  gsm.println(F("AT+CNUM"));
  waitForResponse();
  
  mynumber = "%2b";
  idx = buff.indexOf("+CNUM:");
  mynumber.concat(buff.substring(idx + 12, idx + 24));
  
  gsm.println(F("AT+CREG?"));
  waitForResponse();
  if (buff.indexOf("+CREG: 0,1") == -1) {
    Serial.println(F("Not connected to the network"));
    return;
  }
 
  gsm.println(F("AT+SAPBR=3,1,\"Contype\",\"GPRS\""));
  waitForResponse();
  gsm.print(F("AT+SAPBR=3,1,\"APN\",\""));
  gsm.print(apn);
  gsm.println(F("\""));
  waitForResponse();
  gsm.println(F("AT+SAPBR=1,1"));
  waitForResponse();
  gsm.println(F("AT+HTTPINIT"));
  waitForResponse();
  gsm.println(F("AT+HTTPPARA=\"CID\",1"));
  waitForResponse();
  gsm.print(F("AT+HTTPPARA=\"URL\",\""));
  gsm.print(url);
  gsm.print(F("?token="));
  gsm.print(token);
  gsm.print(F("&source="));
  gsm.print(mynumber);
  gsm.print(F("&message="));
  gsm.print(F("1:")); 
  gsm.print(hum);
  gsm.print(F(";2:"));
  gsm.print(temp);
  gsm.println("\"");
  
  waitForResponse();
  gsm.print(F("AT+HTTPSSL="));
  gsm.println(url.startsWith("https")? 1 : 0);
  waitForResponse();
  
  gsm.println(F("AT+HTTPACTION=0"));
  waitForResponse();
  
  if (buff.indexOf("ERROR") == -1) {
    waitForResponse();
    if (buff.indexOf("200") != -1) {
      gsm.println(F("AT+HTTPREAD"));
      waitForResponse();
      idx = buff.indexOf("NOW:");
      if (idx != -1) {
        serverTime = buff.substring(idx + 4, idx + 23);
        mins = serverTime.substring(14, 16).toInt();
        remains = periodMinutes - mins % periodMinutes;
      } 
      success = true;
    }
  }
  
  gsm.println(F("AT+HTTPTERM"));
  waitForResponse();
  gsm.println(F("AT+SAPBR=0,1"));
  waitForResponse();

  gsm.println(F("AT+CFUN=0"));
  waitForResponse();
  waitForResponse();
  
  gsm.println(F("AT+CSCLK=2"));
  waitForResponse();
  delay(6000);
}

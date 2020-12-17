#include <LowPower.h>
#include <SoftwareSerial.h>
#include <DHT.h>

#define periodMinutes 20
#define timeout 20000
#define attempts 3

#define APN "internet"
#define URL "http://18XXXXX.eg3XXXXX.web.hosting-test.net/api/"
#define TOKEN "1234567890"

SoftwareSerial gsm(2, 3); //SIM800L Tx & Rx is connected to Arduino #2 & #3
DHT dht(4, DHT22); // Initialize DHT sensor for normal 16mhz Arduino

uint16_t remains;

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
  lowPower(remains);
}

void lowPower(uint16_t minutes) {
  uint16_t sleepCounter = minutes * 60 / 8; // It's not very accurate but acceptable
  Serial.print(F("Sleeping for "));
  Serial.print(minutes);
  Serial.println(F(" minute(s)"));
  Serial.flush();
  for (uint16_t i = 0; i < sleepCounter; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
}

void attempt() {
  Serial.println(F("Reading sensors..."));
  hum = dht.readHumidity();
  temp = dht.readTemperature();

  String message = F("1:");
  message.concat(hum);
  message.concat(F(";2:"));
  message.concat(temp);
  
  for(int i = 1; i <= attempts; i++) {
    Serial.print(F("Attempt #"));
    Serial.println(i);
    if (sendInfoToGSM(&message)) {
      Serial.println(F("SUCCESS"));
      break;
    }
    lowPower(1);
  }
}

void waitForResponse(String *buff) {
  uint32_t started = millis();
  
  while(!gsm.available()) {
    if (millis() - started > timeout) {
      Serial.println(F("Timed out!"));
      break;
    }
  }
  
  if (gsm.available()) {
    *buff = gsm.readString();
    Serial.print(*buff);
  }
  else {
    *buff = "";
  }
}

void listenResponses() {
  uint32_t started = millis();

  while (millis() - started < timeout) {
    while (gsm.available()) {
      Serial.write(gsm.read());
    }
  }
}

boolean sendInfoToGSM(String *message)
{
  boolean success = false;
  String buff;

  Serial.println(F("Waking up modem...")); 
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  delay(500);
  pinMode(5, INPUT);
  listenResponses();
 
  gsm.println(F("AT"));
  waitForResponse(&buff);
  if (buff.indexOf(F("OK")) == -1) {
    Serial.println(F("Not connected to GSM modem"));
  }

  gsm.println(F("AT+CNUM"));
  waitForResponse(&buff);
  
  String mynumber = "%2b";
  int idx = buff.indexOf(F("+CNUM:"));
  mynumber.concat(buff.substring(idx + 12, idx + 24));
  
  gsm.println(F("AT+CREG?"));
  waitForResponse(&buff);
  if (buff.indexOf(F("+CREG: 0,1")) == -1) {
    Serial.println(F("Not connected to the network"));
  }
 
  gsm.println(F("AT+SAPBR=3,1,\"Contype\",\"GPRS\""));
  waitForResponse(&buff);
  gsm.print(F("AT+SAPBR=3,1,\"APN\",\""));
  gsm.print(F(APN));
  gsm.println(F("\""));
  waitForResponse(&buff);
  gsm.println(F("AT+SAPBR=1,1"));
  delay(6000);
  waitForResponse(&buff);
  gsm.println(F("AT+SAPBR=2,1"));
  waitForResponse(&buff);
  gsm.println(F("AT+HTTPINIT"));
  waitForResponse(&buff);
  gsm.println(F("AT+HTTPPARA=\"CID\",1"));
  waitForResponse(&buff);
  gsm.print(F("AT+HTTPPARA=\"URL\",\""));
  gsm.print(F(URL));
  gsm.print(F("?token="));
  gsm.print(F(TOKEN));
  gsm.print(F("&source="));
  gsm.print(mynumber);
  gsm.print(F("&message="));
  gsm.print(*message); ;
  gsm.println("\"");
  
  waitForResponse(&buff);
  gsm.print(F("AT+HTTPSSL="));
  gsm.println(strstr(URL, "https") != 0 ? 1 : 0);
  waitForResponse(&buff);
  
  gsm.println(F("AT+HTTPACTION=0"));
  waitForResponse(&buff);
  
  if (buff.indexOf(F("ERROR")) == -1) {
    waitForResponse(&buff);
    if (buff.indexOf(F("200")) != -1) {
      gsm.println(F("AT+HTTPREAD"));
      waitForResponse(&buff);
      idx = buff.indexOf(F("NOW:"));
      if (idx != -1) {
        remains = periodMinutes - buff.substring(idx + 18, idx + 20).toInt() % periodMinutes;
      } 
      success = true;
    }
  }
  
  gsm.println(F("AT+HTTPTERM"));
  waitForResponse(&buff);
  gsm.println(F("AT+SAPBR=0,1"));
  waitForResponse(&buff);

  gsm.println(F("AT+CFUN=0"));
  waitForResponse(&buff);
  waitForResponse(&buff);
  
  gsm.println(F("AT+CSCLK=2"));
  waitForResponse(&buff);
  delay(6000);

  return success;
}

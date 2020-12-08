#include <SoftwareSerial.h>

#define period 120000
#define timeout 10000
#define attempts 3

static SoftwareSerial gsm(2, 3); //SIM800L Tx & Rx is connected to Arduino #2 & #3

const String url = "http://18XXXXXX.egXXXXXXX.web.hosting-test.net/api/";
const String token = "1234567890";

static unsigned long started;
static String buff;
static String mynumber;
static bool success;

void setup()
{
  started = millis();
  Serial.println("Starting serials..."); 
  Serial.begin(9600);
  gsm.begin(9600);

  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  Serial.println("GSM RST LOW (shutdown)");

  attempt();
}

void loop()
{
  while (Serial.available()) gsm.write(Serial.read());
  while(gsm.available()) Serial.write(gsm.read());

  if (millis() - started > period) {
    Serial.println("Ellapsed");
    started = millis();
    
    attempt();
  }
}

void attempt() {
  for(int i = 1; i <= attempts; i++) {
    Serial.print(F("Attempt #"));
    Serial.println(i);
    sendInfoToGSM();
    if (success) break;
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
  success = false;
  
  pinMode(5, INPUT_PULLUP);
  Serial.println("GSM RST PULLUP (wake up)");
  delay(15000);
 
  gsm.println(F("AT"));
  waitForResponse();
  
  if (buff.indexOf("OK") == -1) return; // If not connected to GSM modem

  gsm.println(F("AT+CNUM"));
  waitForResponse();
  
  mynumber = "%2b";
  int idx = buff.indexOf("+CNUM:");
  mynumber.concat(buff.substring(idx + 12, idx + 24));
  
  gsm.println(F("AT+CREG?"));
  waitForResponse();

  if (buff.indexOf("+CREG: 0,1") == -1) return; // if not connected to the network
 
  gsm.println(F("AT+SAPBR=3,1,\"Contype\",\"GPRS\""));
  waitForResponse();
  gsm.println(F("AT+SAPBR=3,1,\"APN\",\"internet\""));
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
  gsm.print(random(350, 370)*.1);
  gsm.print(";2:");
  gsm.print(random(350, 370)*.1);
  gsm.println("\"");
  
  waitForResponse();
  gsm.print(F("AT+HTTPSSL="));
  gsm.println(url.startsWith("https")? 1 : 0);
  waitForResponse();
  
  gsm.println(F("AT+HTTPACTION=0"));
  waitForResponse();
  
  if (buff.indexOf("ERROR") == -1) {
    waitForResponse();
    gsm.println(F("AT+HTTPREAD"));
    waitForResponse();
    success = true;
  }
  
  gsm.println(F("AT+HTTPTERM"));
  waitForResponse();
  gsm.println(F("AT+SAPBR=0,1"));
  waitForResponse();

  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  Serial.println("GSM RST LOW (shutdown)");
}

#include <SoftwareSerial.h>

SoftwareSerial gsm(2, 3); //SIM800L Tx & Rx is connected to Arduino #2 & #3

const unsigned long period = 10800000; // Every 3 hours
unsigned long started;
const unsigned long timeout = 10000;
String buff;
String message;
String response;
String mynumber;
String token = "1234567890";


void setup()
{
  Serial.begin(9600);
  
  gsm.begin(9600);

  Serial.println("Initializing..."); 
  delay(15000);

  started = millis();

  message = "Started:+";
  message.concat(started);
  sendInfoToGSM();
}

void loop()
{
  unsigned long delta;
  unsigned long current;
  
  while (Serial.available()) gsm.write(Serial.read());
  while(gsm.available()) Serial.write(gsm.read());

  current = millis();
  delta = current - started;
  if (delta >= period) {
    started = current;
    
    message = "Repeated:+";
    message.concat(current);
    sendInfoToGSM();
  }
}

void waitForResponse() {
  if (wait(timeout)) {
    buff = gsm.readString();
    Serial.print(buff);
  }
  else {
    buff = "";
  }
}

bool wait(unsigned long timeout)
{
  unsigned long start = millis();
  unsigned long delta = 0;
  
  while(!gsm.available()) {
    delta = millis() - start;
    if (delta >= timeout) {
      Serial.println("Timed out!");
      break;
    }
  }

  return gsm.available();
}

void sendInfoToGSM()
{
  gsm.println("AT");
  waitForResponse();
  
  if (buff.indexOf("OK") == -1) return; // If not connected to GSM modem

  gsm.println("AT+CNUM");
  waitForResponse();
  
  mynumber = "%2b";
  int idx = buff.indexOf("+CNUM:");
  mynumber.concat(buff.substring(idx + 12, idx + 24));
  Serial.print("My number: ");
  Serial.println(mynumber);
  
  gsm.println("AT+CREG?");
  waitForResponse();

  if (buff.indexOf("+CREG: 0,1") == -1) return; // if not connected to the network

  gsm.println("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  waitForResponse();
  gsm.println("AT+SAPBR=3,1,\"APN\",\"internet\"");
  waitForResponse();
  gsm.println("AT+SAPBR=1,1");
  waitForResponse();
  gsm.println("AT+HTTPINIT");
  waitForResponse();
  gsm.println("AT+HTTPPARA=\"CID\",1");
  waitForResponse();
  gsm.print("AT+HTTPPARA=\"URL\",\"web.hosting-test.net/api/?token=");
  gsm.print(token);
  gsm.print("&source=");
  gsm.print(mynumber);
  gsm.print("&type=");
  gsm.print("0");
  gsm.print("&value=");
  gsm.print(message);
  gsm.println("\"");
  // "web.hosting-test.net/api/?token=1234567890&source=%2b380956330650&type=0&value=Started:+123456"
  waitForResponse();
  gsm.println("AT+HTTPSSL=0");
  waitForResponse();
  
  gsm.println("AT+HTTPACTION=0");
  waitForResponse();
  
  if (buff.indexOf("ERROR") == -1) {
    waitForResponse();
    gsm.println("AT+HTTPREAD");
    waitForResponse();
  }
  
  gsm.println("AT+HTTPTERM");
  waitForResponse();
  gsm.println("AT+SAPBR=0,1");
  waitForResponse();
}

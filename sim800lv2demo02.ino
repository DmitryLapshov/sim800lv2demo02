//#include <DHT.h>
//#include <OneWire.h>
//#include <DallasTemperature.h>
#include <SoftwareSerial.h>

SoftwareSerial gsm(2, 3); //SIM800L Tx & Rx is connected to Arduino #2 & #3

//OneWire oneWire(10);
//DallasTemperature ds(&oneWire);

//#define DHTPIN 4
//#define DHTTYPE DHT11

#define amoisture_sensor A0
#define bmoisture_sensor A1
#define cmoisture_sensor A2
#define raine_sensor A3
#define moisture_sensor_power 8
#define raine_sensor_power 7

//DHT dht(DHTPIN, DHTTYPE);

//int very_moist_value = 78;

const String token = "1234567890";
const String url = "http://XXXXXXXX.XXXXXXXX.web.hosting-test.net/api/";
const unsigned long period = 60000; // Every minute
const unsigned long timeout = 10000;

unsigned long started;
String buff;
String mynumber;

//DeviceAddress sensor1 = {0x28, 0xB6, 0x5F, 0x17, 0xC, 0x0, 0x0, 0x54};
//DeviceAddress sensor2 = {0x28, 0xB4, 0x4, 0x17, 0xC, 0x0, 0x0, 0xDF};

void setup()
{
  started = millis();
  Serial.println("Starting serials..."); 
  Serial.begin(9600);
  gsm.begin(9600);

  //ds.begin();
  //dht.begin();

  //pinMode(moisture_sensor_power, OUTPUT);
  //pinMode(raine_sensor_power, OUTPUT);

  started = millis();

  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  Serial.println("GSM RST LOW (shutdown)");

  sendInfoToGSM();
}

void loop()
{
  while (Serial.available()) gsm.write(Serial.read());
  while(gsm.available()) Serial.write(gsm.read());

  if (millis() - started > period) {
    Serial.println("Ellapsed");
    started = millis();
    sendInfoToGSM();
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
 
  /*ds.requestTemperatures();
  
  float h = dht.readHumidity();
  // Считывание температуры в цельсиях
  float t = dht.readTemperature();
  
  digitalWrite(moisture_sensor_power, HIGH);
  digitalWrite(raine_sensor_power, HIGH);
  delay (10);
  int amoisture_value = analogRead(amoisture_sensor);
  int bmoisture_value = analogRead(bmoisture_sensor);
  int cmoisture_value = analogRead(cmoisture_sensor);
  int raine_value = analogRead(raine_sensor);
  
  digitalWrite(raine_sensor_power, LOW);
  digitalWrite(moisture_sensor_power, LOW);
  int amoisture_value_percent = map(amoisture_value,very_moist_value,1023,100,0);
  int bmoisture_value_percent = map(bmoisture_value,very_moist_value,1023,100,0);
  int cmoisture_value_percent = map(cmoisture_value,very_moist_value,1023,100,0);
  int raine_value_percent = map(raine_value,very_moist_value,1023,100,0);
  */
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
  gsm.print(F("1:")); gsm.print(36.6);
  /*gsm.print(ds.getTempC(sensor1));
  gsm.print(";2:");
  gsm.print(ds.getTempC(sensor2));
  gsm.print(";3:");
  gsm.print(h);
  gsm.print(";4:");
  gsm.print(t);
  gsm.print(";5:");
  gsm.print(amoisture_value_percent);
  gsm.print(";6:");
  gsm.print(bmoisture_value_percent);
  gsm.print(";7:");
  gsm.print(cmoisture_value_percent);
  gsm.print(";8:");
  gsm.print(raine_value_percent);*/
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
  }
  
  gsm.println(F("AT+HTTPTERM"));
  waitForResponse();
  gsm.println(F("AT+SAPBR=0,1"));
  waitForResponse();

  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  Serial.println("GSM RST LOW (shutdown)");
}

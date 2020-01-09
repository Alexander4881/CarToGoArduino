#include <TinyGPS.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "arduino_secrets.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
/////// Wifi Settings ///////
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int id = 3;

///////Pin Outputs//////////
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
///////
static const uint8_t megaInterrupt  = D6;
static const uint8_t interruptPin  = D5;

///////SoftwareSerial//////
SoftwareSerial Serial2(D2, D1); // RX, TX

///////Timers//////////
unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long startGPSMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
int gpsUpdateTimer = 10000;
int commandCheckTimer = 100;
unsigned long time_now = 0;
static char pinCode[5];
static bool newPin = false;
// gps vars
TinyGPS gps;
SoftwareSerial gpsSerial(D8, D7);
float flat, flon;
unsigned long age;
bool newData = false;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(9600);
  gpsSerial.begin(9600);

  WiFi.begin(ssid, pass);        //WiFi connection
  Serial.println();
  Serial.println("Waiting for connection:");
  while (WiFi.status() != WL_CONNECTED) {  //Wait for the WiFI connection completion
    Serial.print(".");
    delay(500);
  }
  Serial.println("connected!!!!!!!!!!");

  pinMode(megaInterrupt, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), Interrupt, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)

  if (currentMillis - startMillis >= commandCheckTimer)  //test whether the period has elapsed
  {
    Serial.println("Command Update every 100 ms");
    startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
    if (newPin) {
      String postData = PostData("http://cartogo.api.dk/Reserve/Unlock", (String)"{\"CarID\": " + id + ",\"PinkCode\": \"" + pinCode + "\"}");
      Serial.print("<<<<<<<<<<<<<<<<<<<<< Payload : ");
      Serial.println(postData);
      if (postData == "true") {
        SendMessage("001", 3);
      }else{
        SendMessage("002", 3);
      }
      newPin = false;
    }
  }

  if (currentMillis - startGPSMillis >= gpsUpdateTimer)  //test whether the period has elapsed
  {
    Serial.println("GPS Update every 5 seconds");
    GPSUpdate();
    PostData("http://cartogo.api.dk/Coordinate/New", (String)"{\"CarID\":" + id + ",\"latitude\":" + flat + ",\"longitude\":" + flon + "}");
    startGPSMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }
}

ICACHE_RAM_ATTR void Interrupt() {
  Serial.println("===========================================================================");
  Serial.println("Interrupt From Mega");

  int serialBuffer = Serial2.available() + 1;
  char message[serialBuffer];

  Serial.print("Serial Buffer : ");
  Serial.println(serialBuffer);

  for (int i = 0; i < serialBuffer; i++) {
    message[i] = Serial2.read();
  }

  Serial.print("command : ");
  for (int i = 0; i < sizeof(message); i++) {
    Serial.print(message[i]);
  }
  Serial.println();

  char tcommand[3];
  GetSubArray(message, 0, sizeof(tcommand), tcommand);

  int command = 0;
  command = atoi(tcommand);

  Serial.print("Get Sub Array : ");
  for (int i = 0; i < sizeof(tcommand); i++) {
    Serial.print(tcommand[i]);
  }
  Serial.println();

  switch (command) {
    case 1:
      Serial.println("command setup complete");

      Serial.print("Code : ");
      for (int i = 0; i < sizeof(message); i++) {
        Serial.print(message[i]);
      }
      Serial.println();

      GetSubArray(message, 3, sizeof(pinCode), pinCode);

      Serial.print("Pin Code : ");
      for (int i = 0; i < sizeof(pinCode); i++) {
        Serial.print(pinCode[i]);
      }
      Serial.println();
      newPin = true;
      break;
    default:
      Serial.print("default was called on : ");
      Serial.println(command);
      break;
  }
  Serial.println("===========================================================================");
}

void SendMessage(char data[], int length) {
  Serial.println("Sending Message");
  Serial.println(length);

  for (int i = 0; i < length; i++) {
    Serial.write(data[i]);
    Serial2.write(data[i]);
  }
  Serial.println();

  digitalWrite(megaInterrupt, HIGH);
  delay(10);
  digitalWrite(megaInterrupt, LOW);
}

void GPSUpdate() {
  unsigned long chars;
  unsigned short sentences, failed;

  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (gpsSerial.available())
    {
      char c = gpsSerial.read();
      // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }

  if (newData)
  {
    gps.f_get_position(&flat, &flon, &age);
    Serial.print("LAT=");
    Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    Serial.print(" LON=");
    Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
    Serial.print(" SAT=");
    Serial.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());
    Serial.print(" PREC=");
    Serial.print(gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop());
    newData = false;
  }

  gps.stats(&chars, &sentences, &failed);
  Serial.print(" CHARS=");
  Serial.print(chars);
  Serial.print(" SENTENCES=");
  Serial.print(sentences);
  Serial.print(" CSUM ERR=");
  Serial.println(failed);
  if (chars == 0)
    Serial.println("** No characters received from GPS: check wiring **");
}

String PostData(String url, String data) {
  Serial.print("Start Post url : ");
  Serial.println(url);
  Serial.print("Start Post data : ");
  Serial.println(data);
  String retuneString = "";

  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status

    HTTPClient http;                                      //Declare object of class HTTPClient
    //http.setTimeout(1000);                                //Set HTTP Timeout

    http.begin(url);                                      //Specify request destination
    http.addHeader("Content-Type", "application/json");   //Specify content-type header

    int httpCode = http.POST(data);                       //Send the request
    String payload = http.getString();                    //Get the response payload

    http.end();                                           //Close connection

    Serial.println(httpCode);                             //Print HTTP return code
    Serial.println(payload);                              //Print HTTP return code
    if (httpCode == 200) {
      Serial.println(httpCode);                           //Print HTTP return code
      Serial.println(payload);                            //Print HTTP return code
      retuneString = payload;                             //Retune Payload
    }
  } else {
    Serial.println("Error in WiFi connection");
  }
  return retuneString;
}

void GetSubArray(char data[], int index, int lengthToCopy, char * charArrayPointer) {
  for (int i = 0; i < lengthToCopy; i++) {
    charArrayPointer[i] = data[index + i];
  }
}

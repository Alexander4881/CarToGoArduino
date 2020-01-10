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

  pinMode(megaInterrupt, OUTPUT);                                                       //Setting mega interrupt pin
  pinMode(interruptPin, INPUT_PULLUP);                                                  //setting external interrrupt pin
  attachInterrupt(digitalPinToInterrupt(interruptPin), Interrupt, RISING);              //setting external interrrupt pin
}

void loop() {
  // put your main code here, to run repeatedly:
  currentMillis = millis();                                                                       //get the current "time" (actually the number of milliseconds since the program started)

  if (currentMillis - startMillis >= commandCheckTimer)                                           //test whether the period has elapsed
  {
    Serial.println("Command Update every 100 ms");                                                //user information
    startMillis = currentMillis;                                                                  //IMPORTANT to save the start time
    if (newPin) {                                                                                 //check if we got a new pin
      String postData = PostData("http://cartogo.api.dk/Reserve/Unlock", 
                                (String)"{\"CarID\": " + id + 
                                ",\"PinkCode\": \"" + pinCode + "\"}");                           //make post for pin code check returns true,false,empty string
      if (postData == "true") {                                                                   //true then the pin code was correct
        SendMessage("001", 3);                                                                    //send unlock command to mega
      }else{                                                                                      //fase or empty string pin not correct
        SendMessage("002", 3);                                                                    //send wrong pin to mega
      }
      newPin = false;                                                                             //reset newPin
    }
  }

  if (currentMillis - startGPSMillis >= gpsUpdateTimer)                                           //test whether is time to send gps update
  {
    Serial.println("GPS Update every 5 seconds");                                                 //user information
    GPSUpdate();                                                                                  //get gps update
    PostData("http://cartogo.api.dk/Coordinate/New", 
            (String)"{\"CarID\":" + id + ",\"latitude\":" + 
            flat + ",\"longitude\":" + flon + "}");                                               //post data with new coorinates
    startGPSMillis = currentMillis;                                                               //IMPORTANT to save the start time
  }
}

ICACHE_RAM_ATTR void Interrupt() {                                                                //external interrupt method 
  Serial.println("===========================================================================");  //user information
  Serial.println("Interrupt From Mega");                                                          //user information

  int serialBuffer = Serial2.available() + 1;                                                     //read from the serial buffer
  char message[serialBuffer];                                                                     //instantiate new char array

  Serial.print("Serial Buffer : ");                                                               //user information
  Serial.println(serialBuffer);                                                                   //user information

  for (int i = 0; i < serialBuffer; i++) {                                                        //read from the serial buffer
    message[i] = Serial2.read();
  }

  Serial.print("command : ");                                                                     //user information
  for (int i = 0; i < sizeof(message); i++) {
    Serial.print(message[i]);
  }
  Serial.println();

  char tcommand[3];                                                                              //temp char command 
  GetSubArray(message, 0, sizeof(tcommand), tcommand);                                           //get the first three bytes from message

  int command = 0;                                                                               //int command var default val 0
  command = atoi(tcommand);                                                                      //try to pass the tcommand in to a int

  Serial.print("Get Sub Array : ");                                                              //user information
  for (int i = 0; i < sizeof(tcommand); i++) {
    Serial.print(tcommand[i]);
  }
  Serial.println();

  switch (command) {                                                                            //switch on command
    case 1:                                                                                     //command 1 pin code check
      Serial.println("command setup complete");                               

      Serial.print("Code : ");                                                                 //user information
      for (int i = 0; i < sizeof(message); i++) {
        Serial.print(message[i]);
      }
      Serial.println();

      GetSubArray(message, 3, sizeof(pinCode), pinCode);                                      //get the pin code from message store in global value pinCode

      Serial.print("Pin Code : ");                                                            //user information
      for (int i = 0; i < sizeof(pinCode); i++) {
        Serial.print(pinCode[i]);
      }
      Serial.println();
      newPin = true;                                                                         //set newPin to true
      break;
    default:
      Serial.print("default was called on : ");                                             //user infromation
      Serial.println(command);
      break;
  }
  Serial.println("===========================================================================");
}

void SendMessage(char data[], int length) {                                                //send a message to mega
  Serial.println("Sending Message");                                                       //user information
  Serial.println(length);                                                                  //user information

  for (int i = 0; i < length; i++) {                                                       //loop throug data array on length
    Serial.write(data[i]);                                                                 //user information
    Serial2.write(data[i]);                                                                //write to mega
  }
  Serial.println();                                                                        //user information

  digitalWrite(megaInterrupt, HIGH);                                                       //trigger mega external interrupt
  delay(10);
  digitalWrite(megaInterrupt, LOW);
}

void GPSUpdate() {
  //vars
  unsigned long chars;
  unsigned short sentences, failed;

  for (unsigned long start = millis(); millis() - start < 1000;)                                  // For one second we parse GPS data and report some key values
  {
    while (gpsSerial.available())                                                                 // do while we have a full buffer
    {
      char c = gpsSerial.read();                                                                  //read serial buffer
      if (gps.encode(c))                                                                          // Did a new valid sentence come in?
        newData = true;                                                                           //new data
    }
  }

  if (newData)                                                                                    //check if we got new data
  {
    gps.f_get_position(&flat, &flon, &age);
    Serial.print("LAT=");                                                                        //user information
    Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);                          //user information and we set flat
    Serial.print(" LON=");                                                                       //user informatin
    Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);                          //user information and we set flat
    Serial.print(" SAT=");
    Serial.print(gps.satellites() == TinyGPS::GPS_INVALID_SATELLITES ? 0 : gps.satellites());    //set gps.satellites()
    Serial.print(" PREC=");                                                                      //user informatin
    Serial.print(gps.hdop() == TinyGPS::GPS_INVALID_HDOP ? 0 : gps.hdop());                      //set gps.hdop()
    newData = false;                                                                             //set newData to false
  }

  gps.stats(&chars, &sentences, &failed);                                                        //set gps.stats
  Serial.print(" CHARS=");                                                                       //user informatin
  Serial.print(chars);                                                                           //user informatin
  Serial.print(" SENTENCES=");                                                                   //user informatin
  Serial.print(sentences);                                                                       //user informatin
  Serial.print(" CSUM ERR=");                                                                    //user informatin
  Serial.println(failed);                                                                        //user informatin
  if (chars == 0)                                                                                //if we don get gps chars then we have no connection to it
    Serial.println("** No characters received from GPS: check wiring **");                       //user informatin
}

String PostData(String url, String data) {                                                       //postData
  Serial.print("Start Post url : ");                                                             //user informatin
  Serial.println(url);                                                                           //user informatin
  Serial.print("Start Post data : ");                                                            //user informatin
  Serial.println(data);                                                                          //user informatin
  String retuneString = "";                                                                      //temp var for result

  if (WiFi.status() == WL_CONNECTED) {                                                          //Check WiFi connection status

    HTTPClient http;                                                                            //Declare object of class HTTPClient
    //http.setTimeout(1000);                                                                    //Set HTTP Timeout

    http.begin(url);                                                                            //Specify request destination
    http.addHeader("Content-Type", "application/json");                                         //Specify content-type header

    int httpCode = http.POST(data);                                                             //Send the request
    String payload = http.getString();                                                          //Get the response payload

    http.end();                                                                                 //Close connection

    Serial.println(httpCode);                                                                  //Print HTTP return code
    Serial.println(payload);                                                                   //Print HTTP return code
    if (httpCode == 200) {
      Serial.println(httpCode);                                                                //Print HTTP return code
      Serial.println(payload);                                                                 //Print HTTP return code
      retuneString = payload;                                                                  //Retune Payload
    }
  } else {
    Serial.println("Error in WiFi connection");                                                //user informatin
  }
  return retuneString;
}

void GetSubArray(char data[], int index, int lengthToCopy, char * charArrayPointer) {         //copy a part of an array
  for (int i = 0; i < lengthToCopy; i++) {                                                    //loop throug the data array
    charArrayPointer[i] = data[index + i];                                                    //copy that data array
  }
}

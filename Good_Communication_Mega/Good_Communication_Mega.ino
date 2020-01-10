#include <Keypad.h>

static const uint8_t espInterrupt  = 31;
static const uint8_t interruptPin  = 20;

// Variabler
int lockPin = 33;
char code[5];
bool gotKey = false;
int codeIndex = 0;
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};


byte rowPins[ROWS] = {2, 3, 4, 5};                                           //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 8};                                              //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial1.begin(9600);

  pinMode(espInterrupt, OUTPUT);                                              //set esp interrupt pint to output
  pinMode(lockPin, OUTPUT);                                                   //set lock pin to output
  digitalWrite(espInterrupt, LOW);                                            //set esp interrupt pin to low
  pinMode(interruptPin, INPUT_PULLUP);                                        //set the external interrupt pin to pullup
  attachInterrupt(digitalPinToInterrupt(interruptPin), Interrupt, RISING);    //set the external interrupt

  keypad.addEventListener(keypadEvent);                                      // Add an event listener for this keypad

  //set all the lights to outputs
  for (int i = 22; i < 28; i++) {
    pinMode(i, OUTPUT);
  }

  // check light configuration
  StartBlink();
}

void loop() {
  // put your main code here, to run repeatedly:
  // check if we got a key
  if (gotKey) {
    gotKey = false;
    SetLight(); // set all the lights
  }
  
  // get the pressed key
  char key = keypad.getKey();
  // check if we got a key
  if (key) {
    
    gotKey = true;                          //set we got a ket to true
    Serial.println(key);                    //user information

    if (key == '#' || key == '*') {         // if the key is # or * the we want to delete that char
      if (codeIndex > 0)                    //check if the index is greater then 0
        codeIndex--;                        //we overwrite that char
    } else {                                //else we set that 
      code[codeIndex] = key;                //set the char pin
      codeIndex++;                          //add one to the index
    }

    if (codeIndex > 4) {                    //check if we have reach the end of the pin code
      char temp[8] = {'0', '0', '1'};       //we will run command 001 we will ad the pin to the temp array

      for (int i = 0; i < 5; i++) {         //loop through the pin and adds the char to the array
        temp[i + 3] = code[i];
      }

      SendCommand(temp, sizeof(temp));      // send the commands to the esp
      codeIndex = 0;                        // reset the code index to 0 
    }
  }
}

//external interrupt from the esp
//this interupt is for reading a command
void Interrupt() {
  Serial.println("===========================================================================");    //user information
  Serial.println("Interrupt From Esp");                                                             //user information

  int serialBuffer = Serial1.available();                                                           //get availble bytes from buffer
  char message[serialBuffer];                                                                       //instantiate a char array with the buffer size

  Serial.print("Serial Buffer : ");                                                                 //user information
  Serial.println(serialBuffer);                                                                     //user information

  for (int i = 0; i < serialBuffer; i++) {                                                          //read from buffer to the message char array
    message[i] = Serial1.read();
  }

  Serial.print("command : ");                                                                       //user information
  for (int i = 0; i < sizeof(message); i++) {                                                       //user information
    Serial.print(message[i]);                                                                       //user information
  }
  Serial.println();                                                                                 //user information

  char tcommand[3];                                                                                 //instantiate a tcommand array
  GetSubArray(message, 0, 3, tcommand);                                                             //get the 3 first chars from message

  int command = 0;                                                                                  //instantiate command int
  command = atoi(tcommand);                                                                         //try to passe the tcommand to int

  Serial.print("Get Sub Array : ");                                                                 //user information
  for (int i = 0; i < sizeof(tcommand); i++) {                                                      //user information
    Serial.print(tcommand[i]);                                                                      //user information
  }
  Serial.println();                                                                                 //user information

  switch (command) {
    case 1:
      Serial.println("command setup complete");                                                     //user information
      Serial.println("Car is Unlocked");                                                            //user information
      break;
    case 2:
      Serial.println("command setup complete");                                                     //user information
      Serial.println("Wrong Pin For Car");                                                          //user information
      break;
    default:
      Serial.print("default was called on : ");                                                     //user information
      Serial.println(command);                                                                      //user information
      break;
  }

  Serial.println("===========================================================================");   //user information
}

void SendCommand(char data[], int length) {                                                        //send a command to the esp
  Serial.println("Sending Message");                                                               //user information
  Serial.println(length);                                                                          //user information

  for (int i = 0; i < length; i++) {                                                               //loop through the data array
    Serial.write(data[i]);                                                                         //user information
    Serial1.write(data[i]);                                                                        //write th char to the esp
  }
  Serial.println();                                                                                //user information

  delay(600);                                                                                      //give it time to write to the serial connection

  digitalWrite(espInterrupt, HIGH);                                                                //trigger the interrupt on the esp
  delay(10);                                                                                       //user information
  digitalWrite(espInterrupt, LOW);                                                                 //set pin to low
}

//copy some chars from the array to another
void GetSubArray(char data[], int index, int lengthToCopy, char * charArrayPointer) {
  for (int i = 0; i < lengthToCopy; i++) {                                                         //loop the char array
    charArrayPointer[i] = data[index + i];                                                         //copy a char from one array to antoher
  }
}

// keypad event 
void keypadEvent(KeypadEvent key) {
  switch (keypad.getState()) {                                                                    //key pad event switch
    case HOLD:                                                                                    //hold case
      if (key == '#' || key == '*') {
        Serial.println("Erased Code");                                                            //user information
        codeIndex = 0;                                                                            //reset the code index
      }
      break;
  }
}

void SetLight() {                                                                                //set all light to LOW
  for (int i = 21; i < 28; i++) {
    digitalWrite(i, LOW);
  }
  for (int i = 21; i < 22 + codeIndex; i++) {                                                     //loop through the light
    digitalWrite(i, HIGH);                                                                        //set the output to HIGH
  }
}

void StartBlink() {                                                                               //Blink the lights 3 times
  for (int i = 0; i < 4; i++) {
    for (int j = 21; j < 28; j++) {
      digitalWrite(j, HIGH);
    }
    delay(200);
    for (int j = 21; j < 28; j++) {
      digitalWrite(j, LOW);
    }
    delay(100);
  }
}

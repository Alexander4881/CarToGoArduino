#include <Keypad.h>

static const uint8_t espInterrupt  = 31;
static const uint8_t interruptPin  = 20;

// Variabler
char code[5];
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

  pinMode(espInterrupt, OUTPUT);
  digitalWrite(espInterrupt, LOW);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), Interrupt, RISING);

  keypad.addEventListener(keypadEvent);                                      // Add an event listener for this keypad

  for (int i = 22; i < 28; i++) {
    pinMode(i, OUTPUT);
  }

  StartBlink();
}

void loop() {
  // put your main code here, to run repeatedly:
  SetLight();
  char key = keypad.getKey();

  if (key) {
    Serial.println(key);
    if (key == '#' || key == '*') {
      if (codeIndex > 0)
        codeIndex--;
    } else {
      code[codeIndex] = key;
      codeIndex++;
    }

    if (codeIndex > 4) {
      char temp[8] = {'0', '0', '1'};

      for (int i = 0; i < 5; i++) {
        temp[i + 3] = code[i];
      }

      SendCommand(temp, sizeof(temp));
      codeIndex = 0;
    }
  }
}

void Interrupt() {
  Serial.println("===========================================================================");
  Serial.println("Interrupt From Esp");

  int serialBuffer = Serial1.available();
  char message[serialBuffer];

  Serial.print("Serial Buffer : ");
  Serial.println(serialBuffer);

  for (int i = 0; i < serialBuffer; i++) {
    message[i] = Serial1.read();
  }

  Serial.print("command : ");
  for (int i = 0; i < sizeof(message); i++) {
    Serial.print(message[i]);
  }
  Serial.println();

  char tcommand[3];
  GetSubArray(message, 0, 3, tcommand);

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
      Serial.println("Car is Unlocked");
      break;
    case 2:
      Serial.println("command setup complete");
      Serial.println("Wrong Pin For Car");
      break;
    default:
      Serial.print("default was called on : ");
      Serial.println(command);
      break;
  }

  Serial.println("===========================================================================");
}

void SendCommand(char data[], int length) {
  Serial.println("Sending Message");
  Serial.println(length);

  for (int i = 0; i < length; i++) {
    Serial.write(data[i]);
    Serial1.write(data[i]);
  }
  Serial.println();

  delay(600);

  digitalWrite(espInterrupt, HIGH);
  delay(10);
  digitalWrite(espInterrupt, LOW);
}

void GetSubArray(char data[], int index, int lengthToCopy, char * charArrayPointer) {
  for (int i = 0; i < lengthToCopy; i++) {
    charArrayPointer[i] = data[index + i];
  }
}

void keypadEvent(KeypadEvent key) {
  switch (keypad.getState()) {
    case HOLD:
      if (key == '#' || key == '*') {
        // set index to 0
        Serial.println("Erased Code");
        codeIndex = 0;
      }
      break;
  }
}

void SetLight() {
  for (int i = 21; i < 28; i++) {
    digitalWrite(i, LOW);
  }
  for (int i = 21; i < 22 + codeIndex; i++) {
    digitalWrite(i, HIGH);
  }
}

void StartBlink() {
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

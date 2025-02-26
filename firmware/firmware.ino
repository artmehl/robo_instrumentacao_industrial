#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Ultrasonic.h>

// Pinos RFID
#define RST_PIN 10
#define SS_PIN 9

// Pinos HC-05
#define TX 8
#define RX 7

// Pinos L298N
#define IN_1 A2
#define IN_2 A3
#define IN_3 A0
#define IN_4 6

//Pinos Ultrasom
#define ECHO 5
#define TRIGGER 4 

// Info OLED
#define LARGURA_OLED 128
#define ALTURA_OLED 64
#define RESET_OLED -1

#define INERTIA_DELAY 10

Adafruit_SSD1306 display(LARGURA_OLED, ALTURA_OLED, &Wire, RESET_OLED);
MFRC522 rfid(SS_PIN, RST_PIN);
SoftwareSerial hc05(RX, TX);
Ultrasonic ultrasonic(TRIGGER, ECHO);

char carac;
bool result = false;

void printOled(String msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,10);
  display.print(msg);
  display.display();
}

void forward() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);
}

void right() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, HIGH);
}

void left() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);
}

void backward() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);
}

void back_left() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);
}

void stopMotors() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, HIGH);
}

void processCommand(char command) {
  switch (command) {
    case 'F':
      printOled("FORWARD");
      forward();
      break;

    case 'E':
      printOled("LEFT");
      left();
      break;

    case 'D':
      printOled("RIGHT");
      right();
      break;

    case 'T':
      printOled("BACKWARD");
      backward();
      break;

    case 'P':
      printOled("STOP");
      stopMotors();
      break;

    case 'A':
      printOled("AUTO");
      autonomo();
      break;

    default:
      break;
  }
}

void varredura() {
  printOled("VARREDURA");
  Serial.println("Inicio da Varredura")
  while (true) {
    float distancia = ultrasonic.read();
    Serial.println(distancia);
    if (distancia < 50) break; // Achou objeto
    left();
    delay(50);
    stopMotors();
    delay(INERTIA_DELAY);
  }
  
  stopMotors(); 
  delay(INERTIA_DELAY);
}

bool buscaTAG() {
  printOled("BUSCA TAG");
  while(true) {
    if (!mfrc522.PICC_IsNewCardPresent()) || (!mfrc522.PICC_ReadCardSerial()) { // Não leu a TAG
      if (ultrasonic.read()) <= 10) {
        back_left();
        delay(100);
        stopMotors();
        delay(INERTIA_DELAY);
        backward();
        delay(1000);
        stopMotors();
        delay(100);
        return false; // Perto do objeto, mas não achou a TAG
      }

      else {
        forward();
        delay(50);
        stopMotors();
        delay(50);
      }
    }
    else {
      printOled("LEU TAG");
      Serial.println("Leu a Tag - Terminou :)");
      backward();
      delay(3000);
      stopMotors();
      return true;
    }
  }
}

void autonomo() {
  printOled("INICIANDO AUTO");
  Serial.println("Iniciando modo autonomo");

  Serial.println("ENTRA NA AREA");
  forward();
  delay(1000);
  stopMotors();

  while (true) {
    varredura();
    result = buscaTAG();
    if (!result) continue; // Reinicia o Loop
    else break; // Terminou o circuito
  }
}

void setup() {
  Serial.begin(9600);
  hc05.begin(9600);
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(IN_3, OUTPUT);
  pinMode(IN_4, OUTPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  SPI.begin();
  rfid.PCD_Init();
}

void loop() {
  printOled("Modo manual");
  Serial.println("Modo manual");
  if (hc05.available() > 0) {
    carac = hc05.read();
    processCommand(carac);
  }
  delay(100);
}
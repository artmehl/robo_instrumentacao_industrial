#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SimpleStack.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Ultrasonic.h>

// Pinos RFID
#define RST_PIN 10
#define SS_PIN 9
#define IRQ_PIN 2

// Pinos HC-05
#define TX 8
#define RX 7

// Pinos L298N
#define IN_1 6
#define IN_2 5
#define IN_3 4
#define IN_4 3

//Pinos Ultrasom
#define ECHO 1
#define TRIGGER 0

// Info OLED
#define LARGURA_OLED 128
#define ALTURA_OLED 64
#define RESET_OLED -1

struct Movimento {
  char tipo;
  int tempo;
};

Adafruit_SSD1306 display(LARGURA_OLED, ALTURA_OLED, &Wire, RESET_OLED);
MFRC522 rfid(SS_PIN, RST_PIN);
SoftwareSerial hc05(RX, TX);
SimpleStack<Movimento> historicoMovimentos(10);
Ultrasonic ultrasonic(TRIGGER, ECHO);

bool flag = false;
char carac;
long lastIntr = 0;

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

void left() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, HIGH);
}

void right() {
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

void stopMotors() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, HIGH);
}

float getDistancia() {
  long microsec = ultrasonic.timing();
  float distancia = ultrasonic.convert(microsec, Ultrasonic::CM);
  return distancia;
}

void readMyCard() { // ISR
  if (millis() - lastIntr >= 100) {
    lastIntr = millis();
    flag = true;
  }
}

void processCommand(char command) {
  switch (command) {
    case 'F':
      printOled("FORWARD");
      forward();
      printOled("VAI PARAR");
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

float varreduraEsquerda() {
  float menorDistancia = 0;
  for (int i = 0; i < 10; i++) {
    left();
    delay(100);
    stopMotors();
    float distancia = getDistancia();
    if (distancia < menorDistancia) menorDistancia = distancia;
  }
  return menorDistancia;
}

float varreduraDireita() {
  float menorDistancia = 0;
  for (int i = 0; i < 15; i++) {
    left();
    delay(100);
    stopMotors();
    float distancia = getDistancia();
    if (distancia < menorDistancia) menorDistancia = distancia;
  }
  return menorDistancia;
}

void buscaMenorDistancia(float menorDistancia, float tolerancia) {
  while(true) {
    left();
    delay(100);
    stopMotors();
    float distancia = getDistancia();
    if (distancia - menorDistancia <= tolerancia) break;
  }
}

void buscaTAG(float menorDistancia) {
  int cont = 0;
  for (int i = 0; i < 10; i++) {
    forward();
    delay(100);
    stopMotors();
    float distancia = getDistancia();
    if (distancia < 5) break;
    if (menorDistancia < distancia) cont++;
  }
  if (cont >= 2) //nova varredura
}

void autonomo() {
  printOled("INICIANDO AUTO");
  while (true) { // wait for rfid to get out
    if (flag) {
      flag = false;
      printOled("INTR");
      stopMotors();
      refazerCaminho();
      if (!rfid.PICC_IsNewCardPresent()) return;
      if (!rfid.PICC_ReadCardSerial()) return;
      break;
    }

    float menorDistanciaEsquerda = varreduraEsquerda();
    float menorDistanciaDireita = varreduraDireita();
    float menorDistancia = 0;

    if (menorDistanciaEsquerda <= menorDistanciaDireita) menorDistancia = menorDistanciaEsquerda;
    else menorDistancia = menorDistanciaDireita;

    float tolerancia = menorDistancia * (1 / 100);
    buscaMenorDistancia(menorDistancia, tolerancia);

    rfid.PCD_WriteRegister(rfid.FIFODataReg, rfid.PICC_CMD_REQA);
    rfid.PCD_WriteRegister(rfid.CommandReg, rfid.PCD_Transceive);
    rfid.PCD_WriteRegister(rfid.BitFramingReg, 0x87);
  }
}

void setup() {
  hc05.begin(9600);
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(IN_3, OUTPUT);
  pinMode(IN_4, OUTPUT);
  pinMode(IRQ_PIN, INPUT_PULLUP);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  SPI.begin();
  rfid.PCD_Init();
  rfid.PCD_WriteRegister(rfid.ComIEnReg, 0xA0);
  attachInterrupt(digitalPinToInterrupt(IRQ_PIN), readMyCard, FALLING);
}

void loop() {
  printOled("Modo manual");
  if (hc05.available() > 0) {
    carac = hc05.read();
    processCommand(carac);
  }
  delay(100);
}

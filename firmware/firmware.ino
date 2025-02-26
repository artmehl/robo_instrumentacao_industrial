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

void back_right() {
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

float getDistancia() {
  float distancia = ultrasonic.read();
  return distancia;
}

float menorValor (float x, float y) {
  if (x > y) return x;
  else return y;
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

void varreduraEsquerda() {
  // float distancia = 99999999999;
  while (true) {
    left();
    delay(50);
    stopMotors();
    delay(50);
    float distancia = getDistancia();
    Serial.println(distancia);
    if (distancia < 50) break;
  }
  // stopMotors();
}

float varreduraDireita() {
  float menorDistancia = 99999999999;
  for (int i = 0; i < 20; i++) {
    left();
    delay(15);
    stopMotors();
    float distancia = getDistancia();
    // Serial.println(distancia);
    if ((distancia < menorDistancia) && (distancia != 0)) menorDistancia = distancia;
  }
  stopMotors();
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

void buscaTAG() {
  while(true) {
    //le a TAG, SE LER, BREAK
    if ((rfid.PICC_IsNewCardPresent())) {
      backward();
      delay(3000);
      stopMotors();
      break;
    }
    else {
      float distancia = getDistancia();
      if (distancia <= 10) {
        back_right();
        delay(100);
        stopMotors();
        delay(100);
        backward();
        delay(100);
        stopMotors();
        delay(100);
        right();
        delay(100);
        stopMotors();
        delay(100);
        continue;
      }
      else {
        forward();
        delay(50);
        stopMotors();
        delay(50);
      }
    }
  }
}

void autonomo() {
  printOled("INICIANDO AUTO");
  Serial.println("INICIANDO AUTO");
  Serial.println("Entra na area");
  forward();
  delay(1000);
  stopMotors();
  while (true) { // wait for rfid to get out
    // if (flag) {
    //   flag = false;
    //   printOled("LEU TAG");
    //   Serial.println("LEU TAG");
    //   backward();
    //   delay(5000);
    //   stopMotors();
    //   if (!rfid.PICC_IsNewCardPresent()) return;
    //   if (!rfid.PICC_ReadCardSerial()) return;
    //   break;
    // }
    
    //Varredura
    // printOled("Var Esq");
    // float menorDistanciaEsquerda = varreduraEsquerda();

    Serial.println("Var Esq");
    varreduraEsquerda();
    stopMotors();
    buscaTAG();
    // right();
    // delay(250);
    // stopMotors();
    break;

    //Inicia a funcção busca TAG
    
    // float menorDistancia = 0;

    // if (menorDistanciaEsquerda <= menorDistanciaDireita) menorDistancia = menorDistanciaEsquerda;
    // else menorDistancia = menorDistanciaDireita;

    // float tolerancia = menorDistancia * (1 / 100);
    // buscaMenorDistancia(menorDistancia, tolerancia);
    // bool achouTag = buscaTAG(menorDistancia);
    // if (!achouTag) continue;
    // for (int i = 0; i < 5; i++) {
    //   if (achouTag && !flag) {
    //     //Posição Certa, mas não leu
    //     back_right();
    //     delay(1000);
    //     stopMotors();
    //     backward();
    //     delay(3000);
    //     stopMotors();
    //     while (getDistancia() > 1500) {
    //       right();
    //     }
    //     stopMotors();
    //     buscaTAG(menorDistancia);
    //     right();
    //     delay(500);
    //     stopMotors();
    //   }
    //   else break;
    // }

    rfid.PCD_WriteRegister(rfid.FIFODataReg, rfid.PICC_CMD_REQA);
    rfid.PCD_WriteRegister(rfid.CommandReg, rfid.PCD_Transceive);
    rfid.PCD_WriteRegister(rfid.BitFramingReg, 0x87);
  }
}

void setup() {
  Serial.begin(9600);
  hc05.begin(9600);
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(IN_3, OUTPUT);
  pinMode(IN_4, OUTPUT);
  // pinMode(IRQ_PIN, INPUT_PULLUP);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  SPI.begin();
  rfid.PCD_Init();
  // rfid.PCD_WriteRegister(rfid.ComIEnReg, 0xA0);
  // attachInterrupt(digitalPinToInterrupt(IRQ_PIN), readMyCard, FALLING);
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
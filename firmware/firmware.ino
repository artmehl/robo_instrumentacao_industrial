#include <SoftwareSerial.h>
#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <SimpleStack.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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

// U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);  // Display OLED
Adafruit_SSD1306 display(LARGURA_OLED, ALTURA_OLED, &Wire, RESET_OLED);
MFRC522 rfid(SS_PIN, RST_PIN);
SoftwareSerial hc05(RX, TX);
SimpleStack<Movimento> historicoMovimentos(10);

bool flag = false;
char carac;
long lastIntr = 0;

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

long mediaMovel(int numLeituras) {
  long soma = 0;
  int leiturasValidas = 0;
  for (int i = 0; i < numLeituras; i++) {
    long duracao = pulseIn(1, HIGH);
    printOled(String (duracao));
    if (duracao >= 100 && duracao <= 8000) {
      soma += duracao;
      leiturasValidas++;
    }
    delay(15);
  }
  return leiturasValidas > 0 ? soma / leiturasValidas : 999999;
}

void refazerCaminho() {
  printOled("Voltando...");
  while (!historicoMovimentos.isEmpty()) {
    Movimento mov; 
    if (historicoMovimentos.pop(&mov)) {  // Passando um ponteiro para a função pop
      switch (mov.tipo) {
        case 'F':
          printOled("BACKWARD");
          backward();
          break;
        case 'E':
          printOled("RIGHT");
          right();
          break;
        case 'D':
          printOled("LEFT");
          left();
          break;
      }
      delay(mov.tempo);
    }
  }
  stopMotors();
}

void seguirObjetoHexagono() {
  long leituraAtual = mediaMovel(10);
  
  // Movimento para frente
  printOled("AUTO_FORWARD");
  forward();
  historicoMovimentos.push({'F', 1500});
  delay(1500);
  
  // Movimento para a esquerda até a leitura do sensor ser <= 500
  printOled("AUTO_LEFT");
  left();
  
  long tempoInicio = millis();  // Marca o tempo de início do giro
  while (mediaMovel(10) > 500) {
    //Serial.println(mediaMovel(10));  // Exibe a leitura do sensor no serial
    delay(50);  // Pequena espera para suavizar a leitura
  }
  long tempoGiro = millis() - tempoInicio;  // Calcula o tempo total do giro
  historicoMovimentos.push({'E', tempoGiro});
  
  // Movimento para a direita para evitar colisão
  printOled("AUTO_RIGHT");
  right();
  historicoMovimentos.push({'R', 1000});
  delay(1000);
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

    seguirObjetoHexagono();

    rfid.PCD_WriteRegister(rfid.FIFODataReg, rfid.PICC_CMD_REQA);
    rfid.PCD_WriteRegister(rfid.CommandReg, rfid.PCD_Transceive);
    rfid.PCD_WriteRegister(rfid.BitFramingReg, 0x87);
  }
  // seguirObjetoHexagono();
}

void printOled(String msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,10);
  display.print(msg);
  display.display();
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

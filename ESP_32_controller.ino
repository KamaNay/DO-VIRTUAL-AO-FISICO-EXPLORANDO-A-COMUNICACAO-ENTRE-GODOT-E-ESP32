#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <QMI8658.h> 
#include <Arduino_GFX_Library.h>
#include <math.h>

// --- CONFIGURAÇÕES DE REDE ---
const char* ssid = "Lilinoronha";
const char* password = "Ilbra@1234";

// IPv4 do computador
const char* pc_ip = "192.168.15.6";
const int udp_port = 4242;

// --- PINOS E ENDEREÇO DO SENSOR ---
#define SDA_PIN 6
#define SCL_PIN 7
#define ENDERECO_SENSOR 0x6B
#define PINO_BATERIA 1    
#define TFT_BL 40
#define BOTAO_BOOT 0     

// --- CORES ---
#define PRETO    0x0000
#define BRANCO   0xFFFF
#define CIANO    0x07FF
#define VERMELHO 0xF800
#define VERDE    0x07E0
#define CINZA    0x8410 

Arduino_DataBus *bus = new Arduino_ESP32SPI(8, 9, 10, 11, GFX_NOT_DEFINED, FSPI);
Arduino_GFX *tft = new Arduino_GC9A01(bus, 12, 0, true);

WiFiUDP udp;
QMI8658 imu;

unsigned long ultimoTempoTela = 0; 
float voltagemSuavizada = 0; 
float anguloVolanteAtual = 0; 
String velocidadeAtual = "0";

// ==========================================
// DESENHA OS RAIOS (SEM O NÚCLEO CENTRAL)
// ==========================================
void desenharRaios(float anguloGraus, uint16_t cor) {
  int cx = 120, cy = 120;
  int r_in = 45, r_out = 94; 
  float rad = anguloGraus * PI / 180.0;
  float angulosIniciais[3] = {0.0, PI, PI/2.0}; 

  for(int i = 0; i < 3; i++) {
    float a = angulosIniciais[i] + rad;
    for(int espessura = -6; espessura <= 6; espessura++) { 
      float px = sin(a) * espessura;
      float py = -cos(a) * espessura;
      int x1 = cx + (r_in * cos(a)) + px;
      int y1 = cy + (r_in * sin(a)) + py;
      int x2 = cx + (r_out * cos(a)) + px;
      int y2 = cy + (r_out * sin(a)) + py;
      tft->drawLine(x1, y1, x2, y2, cor);
    }
  }
}

// ==========================================
// DESENHA O PAINEL CENTRAL (VELOCIDADE)
// ==========================================
void atualizarVelocidadeNaTela() {
  tft->fillCircle(120, 120, 42, CINZA);
  tft->fillCircle(120, 120, 36, PRETO);
  
  tft->setCursor(105, 110);
  if (velocidadeAtual.length() > 2) tft->setCursor(95, 110); 
  
  tft->setTextColor(BRANCO);
  tft->setTextSize(3);
  tft->print(velocidadeAtual);
  
  tft->setTextSize(1);
  tft->setCursor(108, 135);
  tft->print("Km/h");
}

void desenharAroFixo() {
  tft->fillCircle(120, 120, 110, CINZA);
  tft->fillCircle(120, 120, 95, PRETO);
}

void setup() {
  Serial.begin(115200);
  pinMode(BOTAO_BOOT, INPUT_PULLUP);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); 
  tft->begin();
  tft->setRotation(2);
  tft->fillScreen(PRETO); 
  Wire.begin(SDA_PIN, SCL_PIN);
  imu.begin(Wire, ENDERECO_SENSOR);
  imu.setAccelUnit_mg(true); 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(100); 
  }
  udp.begin(udp_port); 

  // --- MOSTRA O IP NA TELA POR 5 SEGUNDOS ---
  tft->fillScreen(PRETO);
  tft->setCursor(50, 90);
  tft->setTextColor(VERDE);
  tft->setTextSize(2);
  tft->println("Wi-Fi OK!");

  tft->setCursor(20, 130);
  tft->setTextColor(CIANO);
  tft->println(WiFi.localIP().toString());
  
  delay(5000); // Fica parado 5 segundos para você anotar o IP!
  // -----------------------------------------
  
  tft->fillScreen(PRETO);
  desenharAroFixo();
  desenharRaios(0, CINZA);
  atualizarVelocidadeNaTela();
}

void loop() {
  // 1. ESCUTA O GODOT (VELOCIDADE)
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char pBuffer[10];
    int len = udp.read(pBuffer, 9);
    if (len > 0) pBuffer[len] = 0;
    velocidadeAtual = String(pBuffer);
    atualizarVelocidadeNaTela(); 
  }

  // 2. BOTÃO DESLIGAR
  if (digitalRead(BOTAO_BOOT) == LOW) {
    unsigned long tempoPress = millis();
    while (digitalRead(BOTAO_BOOT) == LOW) { delay(10); } 
    if (millis() - tempoPress > 500) { 
      tft->fillScreen(PRETO);
      digitalWrite(TFT_BL, LOW);
      esp_sleep_enable_ext0_wakeup((gpio_num_t)BOTAO_BOOT, 0);
      esp_deep_sleep_start();
    }
  }

  // 3. ENVIO E ANIMAÇÃO
  QMI8658_Data data;
  if (imu.readSensorData(data)) {
    
    // ⚠️ A MÁGICA ACONTECE AQUI! 
    // Trocamos data.accelY por data.accelZ para ler a inclinação da tela de pé
    float accY = -(data.accelZ / 1000.0);
    float accX = -(data.accelX / 1000.0); 
    
    udp.beginPacket(pc_ip, udp_port);
    udp.print(String(accY) + "," + String(accX));
    udp.endPacket();

    float inclinaX = constrain(accX, -1.0, 1.0);
    float novoAngulo = (inclinaX * 90.0);

    if (abs(novoAngulo - anguloVolanteAtual) > 4.0) { 
      desenharRaios(anguloVolanteAtual, PRETO);
      desenharRaios(novoAngulo, CINZA);
      anguloVolanteAtual = novoAngulo;
      atualizarVelocidadeNaTela(); 
    }
  }

  // 4. BATERIA
  unsigned long tempoAtual = millis();
  if (tempoAtual - ultimoTempoTela > 1000) {
    ultimoTempoTela = tempoAtual;
    float vReal = (analogReadMilliVolts(PINO_BATERIA) / 1000.0) * 3.0;
    voltagemSuavizada = (voltagemSuavizada == 0) ? vReal : (voltagemSuavizada * 0.9) + (vReal * 0.1);
    int bat = constrain(((voltagemSuavizada - 3.3) / (4.2 - 3.3)) * 100, 0, 100);
    tft->fillRect(100, 10, 40, 20, PRETO); 
    tft->setCursor(105, 12); tft->setTextSize(1);
    tft->setTextColor(bat <= 20 ? VERMELHO : CIANO); 
    tft->print(bat); tft->print("%");
  }
  delay(2); 
}

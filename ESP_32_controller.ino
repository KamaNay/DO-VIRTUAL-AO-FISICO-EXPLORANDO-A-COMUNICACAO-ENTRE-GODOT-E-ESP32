#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <QMI8658.h> 

// --- CONFIGURAÇÕES DE REDE ---
const char* ssid = "Lilinoronha";
const char* password = "Ilbra@1234";

// IPv4 do computador
const char* pc_ip = "192.168.15.2";
const int udp_port = 4242;

// --- PINOS E ENDEREÇO DO SENSOR ---
#define SDA_PIN 6
#define SCL_PIN 7
#define ENDERECO_SENSOR 0x6B

WiFiUDP udp;
QMI8658 imu;

void setup() {
  Serial.begin(115200);
  delay(3000); 
  
  log_i("--- Iniciando o Controle ESP32 (Wiimote) ---");

  // 1. Comunicação I2C com os pinos da placa
  Wire.begin(SDA_PIN, SCL_PIN);

  // 2. Inicia o sensor apontando para a "porta" 0x6B
  if (!imu.begin(Wire, ENDERECO_SENSOR)) {
      log_e("ERRO CRÍTICO: Sensor não respondeu no endereço 0x6B!");
      while (1) { delay(100); } 
  }
  
  imu.setAccelUnit_mg(true); 
  log_i("Sensor QMI8658 iniciado com SUCESSO!!!");

  // 3. Conexão com a rede Wi-Fi
  log_i("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  log_i("Wi-Fi Conectado com Sucesso!");
  log_i("Pronto para jogar! Enviando dados para o IP: %s", pc_ip);
}

void loop() {
  QMI8658_Data data;
  
  // 4. Lê os dados de movimento do acelerômetro
  if (imu.readSensorData(data)) {
    // Normaliza os valores (divide por 1000 para transformar de mg para G)
    float accY = data.accelY / 1000.0; // Eixo de acelerar e frear
    float accX = data.accelX / 1000.0; // Eixo de virar o volante

    // Monta a mensagem final: "Y,X"
    String mensagem = String(accY) + "," + String(accX);

    // Envia o pacote UDP pelo Wi-Fi direto para o Godot
    udp.beginPacket(pc_ip, udp_port);
    udp.print(mensagem);
    udp.endPacket();
  }
  
  // Uma pausa de 20ms garante que enviaremos dados a 50 FPS
  delay(20); 
}
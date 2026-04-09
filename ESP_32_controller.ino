#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <QMI8658.h> 
#include <Arduino_GFX_Library.h>
#include <math.h>

// --- CONFIGURAÇÕES DE REDE ---
const char* ssid = "Lilinoronha";
const char* password = "Ilbra@1234";

// IP do computador onde o Godot está a correr. O ESP32 envia os movimentos para cá.
const char* pc_ip = "192.168.15.3";
const int udp_port = 4242; // Porta de comunicação (deve ser a mesma no Godot)

// --- PINOS E ENDEREÇO DO SENSOR ---
#define SDA_PIN 6
#define SCL_PIN 7
#define ENDERECO_SENSOR 0x6B // Endereço I2C padrão do sensor QMI8658
#define PINO_BATERIA 1       // Pino analógico que lê a voltagem da bateria
#define TFT_BL 40            // Pino que controla a luz de fundo (Backlight) da tela
#define BOTAO_BOOT 0         // Botão físico da placa usado para ligar/desligar

// --- CORES RGB565 ---
// A tela usa um formato de cor 16-bits. Estas são as conversões hexadecimais.
#define PRETO    0x0000
#define BRANCO   0xFFFF
#define CIANO    0x07FF
#define VERMELHO 0xF800
#define VERDE    0x07E0
#define CINZA    0x8410 

// --- INICIALIZAÇÃO DA TELA E COMUNICAÇÃO ---
Arduino_DataBus *bus = new Arduino_ESP32SPI(8, 9, 10, 11, GFX_NOT_DEFINED, FSPI);
Arduino_GFX *tft = new Arduino_GC9A01(bus, 12, 0, true); // GC9A01 é o chip da tela redonda

WiFiUDP udp;  // Cria o objeto para comunicação via rede (rápida e sem atrasos)
QMI8658 imu;  // Cria o objeto do sensor de movimento (Acelerômetro/Giroscópio)

// --- VARIÁVEIS GLOBAIS ---
unsigned long ultimoTempoTela = 0; // Controla a taxa de atualização da bateria
float voltagemSuavizada = 0;       // Evita que a bateria fique "piscando" entre números
float anguloVolanteAtual = 0;      // Guarda a última posição do desenho do volante
String velocidadeAtual = "0";      // Guarda a velocidade recebida do Godot

// ==========================================
// FUNÇÃO: DESENHAR OS RAIOS DO VOLANTE ANIMADO
// ==========================================
void desenharRaios(float anguloGraus, uint16_t cor) {
  int cx = 120, cy = 120; // O centro exato da tela de 240x240 pixels
  int r_in = 45, r_out = 94; // Onde os raios nascem (r_in) e onde terminam (r_out)
  
  // Converte graus para radianos
  float rad = anguloGraus * PI / 180.0;
  
  // Posições originais dos 3 raios (Direita, Esquerda e Baixo)
  float angulosIniciais[3] = {0.0, PI, PI/2.0}; 

  for(int i = 0; i < 3; i++) {
    float a = angulosIniciais[i] + rad; // Soma a inclinação da mão do jogador
    
    // Desenha 12 linhas juntas (de -6 a +6) para criar um raio grosso em vez de uma linha fina
    for(int espessura = -6; espessura <= 6; espessura++) { 
      float px = sin(a) * espessura;
      float py = -cos(a) * espessura;
      
      // Calcula os pontos X e Y usando trigonometria
      int x1 = cx + (r_in * cos(a)) + px;
      int y1 = cy + (r_in * sin(a)) + py;
      int x2 = cx + (r_out * cos(a)) + px;
      int y2 = cy + (r_out * sin(a)) + py;
      
      tft->drawLine(x1, y1, x2, y2, cor);
    }
  }
}

// ==========================================
// FUNÇÃO: DESENHAR O VELOCÍMETRO CENTRAL
// ==========================================
void atualizarVelocidadeNaTela() {
  // Desenha o "miolo" do volante
  tft->fillCircle(120, 120, 42, CINZA);
  tft->fillCircle(120, 120, 36, PRETO);
  
  // Centraliza o texto dependendo se tem 1, 2 ou 3 dígitos
  tft->setCursor(105, 110);
  if (velocidadeAtual.length() > 2) tft->setCursor(95, 110); 
  
  // Imprime a velocidade
  tft->setTextColor(BRANCO);
  tft->setTextSize(3);
  tft->print(velocidadeAtual);
  
  // Imprime "Km/h" menor logo abaixo
  tft->setTextSize(1);
  tft->setCursor(108, 135);
  tft->print("Km/h");
}

// ==========================================
// FUNÇÃO: DESENHAR O ARO EXTERIOR DO VOLANTE
// ==========================================
void desenharAroFixo() {
  tft->fillCircle(120, 120, 110, CINZA);
  tft->fillCircle(120, 120, 95, PRETO); 
}

void setup() {
  // Configuração dos pinos físicos
  pinMode(BOTAO_BOOT, INPUT_PULLUP);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // Liga a luz da tela
  
  tft->begin();
  tft->fillScreen(PRETO); 
  
  // Inicia a comunicação com o sensor de movimento
  Wire.begin(SDA_PIN, SCL_PIN);
  imu.begin(Wire, ENDERECO_SENSOR);
  imu.setAccelUnit_mg(true); // Lê a aceleração em mili-gravidades (mG)
  
  // Conecta ao Wi-Fi (o loop prende o código aqui até conectar)
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(100); }
  
  udp.begin(udp_port); // Começa a ouvir a porta 4242

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
  
  // Desenha a interface inicial do controle
  tft->fillScreen(PRETO);
  desenharAroFixo();
  desenharRaios(0, CINZA);
  atualizarVelocidadeNaTela();
}

void loop() {
  // --- 1. ESCUTA O GODOT (VELOCIDADE) ---
  int packetSize = udp.parsePacket();
  if (packetSize) {
    char pBuffer[10];
    int len = udp.read(pBuffer, 9);
    if (len > 0) pBuffer[len] = 0;
    
    velocidadeAtual = String(pBuffer); // Guarda o número recebido
    atualizarVelocidadeNaTela();       // Redesenha apenas o centro da tela
  }

  // --- 2. GESTÃO DE ENERGIA (BOTÃO DESLIGAR) ---
  // Se o botão BOOT for pressionado...
  if (digitalRead(BOTAO_BOOT) == LOW) {
    unsigned long tempoPress = millis();
    while (digitalRead(BOTAO_BOOT) == LOW) { delay(10); } // Aguarda o usuário soltar
    
    // Se segurou por mais de meio segundo (500ms), entra em modo Deep Sleep
    if (millis() - tempoPress > 500) { 
      tft->fillScreen(PRETO);
      digitalWrite(TFT_BL, LOW); // Apaga a luz da tela
      // Configura o botão BOOT para acordar a placa depois
      esp_sleep_enable_ext0_wakeup((gpio_num_t)BOTAO_BOOT, 0);
      esp_deep_sleep_start(); // Hiberna (consumo de energia quase zero)
    }
  }

  // --- 3. LEITURA, ENVIO E ANIMAÇÃO ---
  QMI8658_Data data;
  if (imu.readSensorData(data)) {
    // Converte de mili-G para G (1G = gravidade terrestre)
    float accY = (data.accelY / 1000.0); 
    float accX = (data.accelX / 1000.0); 
    
    // Empacota e envia para o PC ("Y,X")
    udp.beginPacket(pc_ip, udp_port);
    udp.print(String(accY) + "," + String(accX));
    udp.endPacket();

    // Lógica da Animação do Volante
    float inclinaX = constrain(accX, -1.0, 1.0); // Trava o valor entre -1 e 1
    float novoAngulo = -(inclinaX * 90.0);       // Converte 1G em 90 graus (inverte o sinal para girar certo)

    // "Amortecedor Visual": Só redesenha se a mão virou mais de 4 graus
    // Isso evita que o desenho fique tremendo por causa de micro movimentos
    if (abs(novoAngulo - anguloVolanteAtual) > 4.0) { 
      desenharRaios(anguloVolanteAtual, PRETO); // Pinta os raios antigos de preto (apaga)
      desenharRaios(novoAngulo, CINZA);         // Pinta os raios na nova posição
      anguloVolanteAtual = novoAngulo;          // Guarda a posição atual
      atualizarVelocidadeNaTela();              // Refaz o centro para tapar buracos deixados pela linha preta
    }
  }

  // --- 4. ATUALIZAÇÃO DA BATERIA ---
  // Roda apenas a cada 1 segundo (1000 ms) para economizar processamento
  unsigned long tempoAtual = millis();
  if (tempoAtual - ultimoTempoTela > 1000) {
    ultimoTempoTela = tempoAtual;
    
    // Lê a voltagem do pino e multiplica pela proporção do divisor de tensão da placa
    float vReal = (analogReadMilliVolts(PINO_BATERIA) / 1000.0) * 3.0;
    
    // Filtro Passa-Baixa (Suavização): Mistura 90% da leitura antiga com 10% da nova
    voltagemSuavizada = (voltagemSuavizada == 0) ? vReal : (voltagemSuavizada * 0.9) + (vReal * 0.1);
    
    // Mapeia de Volts (3.3v a 4.2v) para Porcentagem (0% a 100%)
    int bat = constrain(((voltagemSuavizada - 3.3) / (4.2 - 3.3)) * 100, 0, 100);
    
    // Limpa o cantinho do topo e escreve o valor
    tft->fillRect(100, 10, 40, 20, PRETO); 
    tft->setCursor(105, 12); 
    tft->setTextSize(1);
    tft->setTextColor(bat <= 20 ? VERMELHO : CIANO); // Fica vermelho se bateria <= 20%
    tft->print(bat); 
    tft->print("%");
  }
  
  delay(2); // Micro-respiro para o Wi-Fi não desconectar sozinho
}

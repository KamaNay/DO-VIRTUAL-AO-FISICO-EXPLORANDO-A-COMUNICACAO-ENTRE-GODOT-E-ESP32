# DO-VIRTUAL-AO-FISICO-EXPLORANDO-A-COMUNICACAO-ENTRE-GODOT-E-ESP-32

O trabalho descreve a integração via Wi-Fi entre a engine Godot e o microcontrolador ESP-32 para criar jogos imersivos. Usando sensores para converter ações físicas em comandos digitais, o projeto validou a estabilidade e o tempo de resposta do sistema. O resultado prova a viabilidade de soluções de baixo custo para jogos, educação e automação.

# 🏎️ Controle de movimento sem fio ESP32 para Godot

Este projeto transforma um **ESP32-S3 (com tela redonda GC9A01)** em um volante de videogame sem fio de alta precisão. O controle utiliza sensores de movimento (IMU) para dirigir veículos no motor de jogos **Godot 4**, enviando dados via protocolo **UDP** e recebendo telemetria em tempo real.

## 🚀 Funcionalidades

- **Controle por Movimento:** Direção e aceleração baseadas na inclinação física (Acelerômetro QMI8658).
- **Dashboard Ativo:** A tela do controle exibe um volante animado que gira sincronizado com as mãos do jogador.
- **Telemetria Bidirecional:** Recebe a velocidade atual do carro (Km/h) direto do Godot e exibe no centro do visor.
- **Gestão de Energia:** Sistema de *Deep Sleep* ativado por botão físico para preservação da bateria.
- **Monitor de Bateria:** Exibição dinâmica da carga com alertas visuais (Ciano -> Vermelho).
- **Comunicação UDP:** Baixíssima latência para uma experiência de condução fluida.

## 🛠️ Hardware Utilizado

* **Microcontrolador:** ESP32-S3 (Waveshare/LILYGO com display redondo).
* **Display:** LCD Redondo 1.28" (Driver GC9A01).
* **Sensor:** IMU QMI8658 (Acelerômetro e Giroscópio).
* **Comunicação:** Wi-Fi 2.4GHz.

## 📂 Estrutura do Projeto

* `/arduino`: Código fonte `.ino` para a placa ESP32.
* `/godot_script`: Script GDScript para o veículo no Godot 4.

## 🔧 Configuração Rápida

### 1. Arduino IDE
1. Instale as bibliotecas: `GFX_Library_for_Arduino`, `WiFi`, `WiFiUdp` e a biblioteca específica para o sensor `QMI8658`.
2. No arquivo `.ino`, atualize as credenciais de rede:
   ```cpp
   const char* ssid = "NOME_DO_WIFI";
   const char* password = "SENHA";
   const char* pc_ip = "192.168.15.X"; // IP do seu PC rodando o Godot

3. Faça o upload para a placa.

### 2. Godot 4
1. Adicione o script fornecido ao seu nó VehicleBody3D ou RigidBody3D.

2. Certifique-se de que o IP na variável esp32_ip no script car.gd corresponde ao que aparece na tela da placa ao iniciar.

### 🎮 Como Jogar
1. Ligue o controle (ele conectará automaticamente ao Wi-Fi).

2. Abra o jogo no Godot.

3. Inclinar para Frente/Trás: Acelera e freia.

4. Inclinar para os Lados: Gira o volante.

5. Segurar Botão BOOT: Desliga o controle (Deep Sleep).

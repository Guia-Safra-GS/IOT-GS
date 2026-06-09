# AgroMonitor

Projeto IoT com ESP32 para monitoramento agrícola. O sistema lê umidade do solo, temperatura e umidade do ar, exibe os dados em um OLED, sinaliza o status com LED RGB e disponibiliza uma página web com atualização automática.

## Características do projeto

- ESP32 como controlador principal
- Potenciômetro simulando a umidade do solo no Wokwi
- Sensor DHT22 para temperatura e umidade do ar
- Display OLED SSD1306 com 3 páginas de informações
- Botão para alternar as páginas do OLED
- LED RGB indicando o estado do solo
- WebServer na porta 80 com interface HTML
- Endpoint `/status` retornando os dados em JSON
- Atualização das leituras a cada 2 segundos

## Componentes

| Componente | Função |
| --- | --- |
| ESP32 DevKit V1 | Processamento e conectividade Wi-Fi |
| Potenciômetro | Simula a umidade do solo |
| DHT22 | Mede temperatura e umidade do ar |
| OLED SSD1306 | Exibe os dados localmente |
| Botão | Alterna as telas do OLED |
| LED RGB | Mostra o status do solo por cor |

## Pinos utilizados

- Potenciômetro: GPIO34
- DHT22: GPIO4
- Botão: GPIO15
- OLED: SDA GPIO21 / SCL GPIO22
- LED RGB: GPIO25, GPIO26, GPIO27

## Links

- [Vídeo no YouTube](https://youtu.be/HiZqSwmcQiY)
- [Projeto no Wokwi](https://wokwi.com/projects/466199764448632833)

## Como executar no Wokwi

1. Abra o projeto no Wokwi.
2. Inicie a simulação.
3. Acesse a interface web exibida no navegador ou pelo endereço configurado no simulador.


# Comunicação de Dados — ESP32 + 433 MHz

Projeto de camada de enlace implementada manualmente sobre módulos RF de 433 MHz (FS1000A e XY-MK-5V), com protocolo Stop-and-Wait ARQ, CRC-8 e exibição em display ST7735.

## Funcionalidades

- Transmissão de **texto** e **imagens monocromáticas (64×64)** entre dois ESP32
- Protocolo Stop‑and‑Wait com ACK e retransmissão
- Detecção de erros com CRC-8 (polinômio 0x07)
- Fragmentação de dados em quadros de até 16 bytes
- Exibição do conteúdo recebido em display TFT 1.8"
- Comunicação half‑duplex simulada (cada lado tem um par TX/RX)

## Hardware

| Componente         | Quantidade |
|--------------------|------------|
| ESP32 (30 pinos)   | 2          |
| FS1000A (433 MHz)  | 2          |
| XY‑MK‑5V (433 MHz) | 2          |
| Display ST7735 1.8"| 1          |
| Protoboard         | 4          |
| Jumpers macho‑macho e macho‑fêmea | — |

## Pinagem

### ESP32 A (transmissor principal)
- GPIO2  → FS1000A DATA
- GPIO34 → XY‑MK‑5V DATA (recebe ACK)

### ESP32 B (receptor + display)
- GPIO35 → XY‑MK‑5V DATA
- GPIO2  → FS1000A DATA (envia ACK)
- Display ST7735 (SPI): SCK=18, MOSI=23, DC=16, RST=4, CS=5

## Como usar

1. Carregue o sketch do transmissor no ESP32 A.
2. Carregue o sketch do receptor no ESP32 B.
3. No transmissor, edite as variáveis no topo do código para escolher entre texto ou imagem.
4. Ligue os módulos e alimente ambos os ESP32.
5. O receptor mostrará a mensagem ou imagem no display automaticamente.

## Estrutura do quadro
Flag (0x7E) | Tipo | Sequência | Tamanho | Dados (0–16 B) | CRC-8

**Tipos:** 0x04 (texto), 0x01 (imagem), 0x02 (ACK)

## Relatório
O relatório completo do projeto está disponível em [Relatorio completo.pdf](Relatorio%20completo.pdf).

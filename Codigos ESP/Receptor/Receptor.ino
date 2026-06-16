#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define RX_PIN 35
#define TX_PIN 2

#define TFT_CS  5
#define TFT_RST 4
#define TFT_DC  16

#define FLAG 0x7E

#define TYPE_IMAGE 0x01
#define TYPE_ACK   0x02
#define TYPE_NACK  0x03
#define TYPE_TEXT  0x04

#define BIT_PERIOD 1000
#define payloadMax 16

#define IMG_W 64
#define IMG_H 64
#define tamImagem 512

Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

uint8_t seqEsperado = 0;

uint8_t bufferImagem[tamImagem];
uint16_t indiceImagem = 0;

String mensagemRecebida = "";

uint8_t crc8(uint8_t *data, uint8_t len) {
  uint8_t crc = 0;

  for (int i = 0; i < len; i++) {
    crc ^= data[i];

    for (int j = 0; j < 8; j++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ 0x07;
      else
        crc <<= 1;
    }
  }

  return crc;
}

void sendBit(bool b) {
  digitalWrite(TX_PIN, b);
  delayMicroseconds(BIT_PERIOD);
}

void sendByte(uint8_t b) {
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(BIT_PERIOD);

  for (int i = 0; i < 8; i++) {
    digitalWrite(TX_PIN, (b >> i) & 1);
    delayMicroseconds(BIT_PERIOD);
  }

  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(BIT_PERIOD);
}

void sendPreamble() {
  for (int i = 0; i < 8; i++)
    sendByte(0xAA);
}

void sendFrame(uint8_t type, uint8_t seq, uint8_t *data, uint8_t len) {
  uint8_t quadroCRC[22];

  quadroCRC[0] = FLAG;
  quadroCRC[1] = type;
  quadroCRC[2] = seq;
  quadroCRC[3] = len;

  for (int i = 0; i < len; i++)
    quadroCRC[4 + i] = data[i];

  quadroCRC[4 + len] = crc8(quadroCRC, 4 + len);

  int total = 5 + len;

  sendPreamble();

  for (int i = 0; i < total; i++)
    sendByte(quadroCRC[i]);

  digitalWrite(TX_PIN, LOW);
}

bool readByte(uint8_t &byteRecebido) {
  uint32_t inicio = millis();
  while (digitalRead(RX_PIN) == LOW) {
    if (millis() - inicio > 100)
      return false;
  }
  delayMicroseconds(BIT_PERIOD + BIT_PERIOD / 2);
  uint8_t b = 0;
  for (int i = 0; i < 8; i++) {
    if (digitalRead(RX_PIN))
      b |= (1 << i);
    delayMicroseconds(BIT_PERIOD);
  }
  delayMicroseconds(BIT_PERIOD / 2);
  byteRecebido = b;
  return true;
}

void mostraTexto(String msg) {
  tft.fillScreen(ST77XX_BLACK);

  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);

  tft.println("Mensagem:");
  tft.setTextColor(ST77XX_GREEN);

  const int maxChars = 20;

  while (msg.length() > maxChars) {
    tft.println(msg.substring(0, maxChars));
    msg = msg.substring(maxChars);
  }

  tft.println(msg);
}

void mostraImagemMono(int x, int y) {
  tft.fillRect(x, y, IMG_W, IMG_H, ST77XX_BLACK);

  for (int row = 0; row < IMG_H; row++) {
    for (int col = 0; col < IMG_W; col++) {

      int idx = (row * IMG_W + col) / 8;
      int bit = 7 - (col % 8);

      bool pixel = (bufferImagem[idx] >> bit) & 1;

      tft.drawPixel(
        x + col,
        y + row,
        pixel ? ST77XX_WHITE : ST77XX_BLACK
      );
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  tft.initR(INITR_144GREENTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.println("Aguardando...");

  pinMode(RX_PIN, INPUT);
  pinMode(TX_PIN, OUTPUT);

  digitalWrite(TX_PIN, LOW);

  Serial.println("RX pronto");
}

void loop() {
  static uint32_t ultimoPacote = 0;

  uint8_t byteRecebido;

  if (readByte(byteRecebido)) {

    if (byteRecebido == FLAG) {

      uint8_t type = 0;
      uint8_t seq = 0;
      uint8_t len = 0;

      if (!readByte(type)) return;
      if (!readByte(seq)) return;
      if (!readByte(len)) return;

      if (len > payloadMax)
        return;

      uint8_t data[payloadMax];

      for (int i = 0; i < len; i++) {
        if (!readByte(data[i]))
          return;
      }

      uint8_t crcRecebido;

      if (!readByte(crcRecebido))
        return;

      uint8_t quadroCRC[20];

      quadroCRC[0] = FLAG;
      quadroCRC[1] = type;
      quadroCRC[2] = seq;
      quadroCRC[3] = len;

      for (int i = 0; i < len; i++)
        quadroCRC[4 + i] = data[i];

      uint8_t crcCalculado = crc8(quadroCRC, 4 + len);

      if (crcCalculado == crcRecebido) {

        if (type == TYPE_TEXT || type == TYPE_IMAGE) {

          if (seq == seqEsperado)
            seqEsperado ^= 1;

          delay(1);
          sendFrame(TYPE_ACK, seq, NULL, 0);

          Serial.printf("ACK %d\n", seq);

          if (type == TYPE_TEXT) {

            for (int i = 0; i < len; i++)
              mensagemRecebida += (char)data[i];

            if (len < payloadMax) {

              Serial.println(mensagemRecebida);
              mostraTexto(mensagemRecebida);

              mensagemRecebida = "";
            }

          } else if (type == TYPE_IMAGE) {

            for (int i = 0; i < len; i++) {

              if (indiceImagem < tamImagem)
                bufferImagem[indiceImagem++] = data[i];
            }

            if (indiceImagem == tamImagem) {

              Serial.println("Imagem recebida");

              int x = (128 - IMG_W) / 2;
              int y = (160 - IMG_H) / 2;

              mostraImagemMono(x, y);

              indiceImagem = 0;
            }
          }

          ultimoPacote = millis();
        }
      }
    }
  }

  if (millis() - ultimoPacote > 5000 && ultimoPacote != 0) {

    seqEsperado = 0;
    indiceImagem = 0;
    mensagemRecebida = "";

    Serial.println("Timeout");

    ultimoPacote = millis();
  }
}
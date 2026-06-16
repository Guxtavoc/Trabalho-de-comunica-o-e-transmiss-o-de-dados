#define TX_PIN 2
#define RX_PIN 34

#define FLAG 0x7E

#define TYPE_IMAGE 0x01
#define TYPE_ACK   0x02
#define TYPE_NACK  0x03
#define TYPE_TEXT  0x04

#define BIT_PERIOD 1000
#define MAX_PAYLOAD 16

uint8_t seq = 0;

const bool enviarTexto = true;
const char *msg = "Mensagem com mais de 16 bits para teste de pacotes 1234";

const uint8_t IMAGE_DATA[] PROGMEM = {
//Bitmap da imagem aqui
};

#define IMAGE_SIZE 512

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
  uint8_t buffer[22];

  buffer[0] = FLAG;
  buffer[1] = type;
  buffer[2] = seq;
  buffer[3] = len;

  for (int i = 0; i < len; i++)
    buffer[4 + i] = data[i];

  buffer[4 + len] = crc8(buffer, 4 + len);

  int total = 5 + len;

  sendPreamble();

  for (int i = 0; i < total; i++)
    sendByte(buffer[i]);

  digitalWrite(TX_PIN, LOW);
}

uint8_t readBit() {
  delayMicroseconds(BIT_PERIOD / 2);

  uint8_t v = digitalRead(RX_PIN);

  delayMicroseconds(BIT_PERIOD / 2);

  return v;
}

uint8_t readByte() {
  while (digitalRead(RX_PIN) == LOW);

  delayMicroseconds(BIT_PERIOD + BIT_PERIOD / 2);

  uint8_t b = 0;

  for (int i = 0; i < 8; i++) {
    b |= digitalRead(RX_PIN) << i;
    delayMicroseconds(BIT_PERIOD);
  }

  delayMicroseconds(BIT_PERIOD / 2);

  return b;
}

uint8_t readFrame(uint8_t &rseq, uint8_t *data, uint8_t &len, uint32_t timeout) {
  uint32_t inicio = millis();

  while (millis() - inicio < timeout) {
    uint8_t b = readByte();

    if (b != FLAG)
      continue;

    uint8_t type = readByte();

    rseq = readByte();
    len = readByte();

    if (len > MAX_PAYLOAD)
      continue;

    for (int i = 0; i < len; i++)
      data[i] = readByte();

    uint8_t fcs = readByte();

    uint8_t tmp[20];

    tmp[0] = FLAG;
    tmp[1] = type;
    tmp[2] = rseq;
    tmp[3] = len;

    for (int i = 0; i < len; i++)
      tmp[4 + i] = data[i];

    if (crc8(tmp, 4 + len) != fcs)
      continue;

    return type;
  }

  return 0;
}

bool sendData(uint8_t type, const uint8_t *data, uint16_t length) {
  uint16_t offset = 0;

  while (offset < length) {
    uint16_t remaining = length - offset;
    uint8_t chunk = (remaining > MAX_PAYLOAD) ? MAX_PAYLOAD : remaining;
    uint8_t payload[MAX_PAYLOAD];
    memcpy(payload, data + offset, chunk);
    bool ack = false;
    int tent = 0;

    while (!ack && tent < 5) {
      Serial.printf("TX tipo=%X seq=%d bloco=%d tam=%d tent=%d\n", type, seq, offset, chunk, tent + 1);
      sendFrame(type, seq, payload, chunk);
      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(500);
      uint8_t rseq, rlen, rdata[16];
      uint8_t rtype = readFrame(rseq, rdata, rlen, 200);
      if (rtype == TYPE_ACK && rseq == seq)
        ack = true;
      else
        tent++;
    }
    if (!ack) {
      Serial.println("Falha TX");
      return false;
    }
    seq ^= 1;
    offset += chunk;
  }

  if (length % MAX_PAYLOAD == 0 && length > 0) {
    bool ack = false;
    int tent = 0;

    while (!ack && tent < 5) {
      sendFrame(type, seq, NULL, 0);

      digitalWrite(TX_PIN, LOW);
      delayMicroseconds(500);

      uint8_t rseq, rlen, rdata[16];

      uint8_t rtype = readFrame(rseq, rdata, rlen, 200);

      if (rtype == TYPE_ACK && rseq == seq)
        ack = true;
      else
        tent++;
    }

    if (!ack)
      return false;

    seq ^= 1;
  }

  return true;
}

void setup() {
  Serial.begin(115200);

  delay(1000);

  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);

  digitalWrite(TX_PIN, LOW);

  Serial.println("TX pronto");
}

void loop() {
  if (enviarTexto)
    sendData(TYPE_TEXT, (const uint8_t *)msg, strlen(msg));
  else
    sendData(TYPE_IMAGE, IMAGE_DATA, IMAGE_SIZE);

  delay(5000);
}
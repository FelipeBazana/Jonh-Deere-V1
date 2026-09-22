#include <SPI.h>
#include <LoRa.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <iostream>
#include <cmath>


TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

#define SS 5
#define RST 21
#define DIO0 4

#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600

#define RAIO_TERRA 6371000.0

unsigned long tempoAnterior = 0;
int sender = 0;

int LoRa_1m = -31;
float Perda_Ambiente = 2;

void setup() {
  Serial.begin(115200);

  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);

  randomSeed(analogRead(34));

  LoRa.setPins(SS, RST, DIO0);

  while (!LoRa.begin(915E6)) {
    Serial.println("--LoRa não funcionando--");
    delay(500);
  }

  LoRa.setSyncWord(0x34);
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(8);
  LoRa.enableCrc();

  Serial.println("--LoRa Configurado--");
}

void loop() {
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  float latitude = gps.location.lat();
  float longitude = gps.location.lng();
  float velocidade = gps.speed.kmph();
  float altitude = gps.altitude.meters();

  static int number = 2;

  if (millis() - tempoAnterior > 3000) {
    sender = random(0, 2); // sorteia 0 ou 1
    tempoAnterior = millis();

  }

  if (sender == 1) {

    LoRa.beginPacket();
    LoRa.print(number);
    LoRa.print(",");
    LoRa.print(latitude);
    LoRa.print(",");
    LoRa.print(longitude);
    LoRa.print(",");
    LoRa.print(altitude);
    LoRa.print(",");
    LoRa.print(velocidade);
    LoRa.endPacket();

    delay(100);
  }

  int pacote = LoRa.parsePacket();

  if (pacote) {

    String mensagem = "";

    while (LoRa.available()) {
      mensagem += (char)LoRa.read();
    }

    int p1 = mensagem.indexOf(',');
    int p2 = mensagem.indexOf(',', p1 + 1);
    int p3 = mensagem.indexOf(',', p2 + 1);
    int p4 = mensagem.indexOf(',', p3 + 1);

    int numero_recebido = mensagem.substring(0, p1).toInt();
    float latitude_recebida = mensagem.substring(p1 + 1, p2).toFloat();
    float longitude_recebida = mensagem.substring(p2 + 1, p3).toFloat();
    float altitude_recebida = mensagem.substring(p3 + 1, p4).toFloat();
    float velocidade_recebida = mensagem.substring(p4 + 1).toFloat();

    int rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    Serial.print("\nNumero recebido = ");
    Serial.println(numero_recebido);

    Serial.print("Latitude recebida = ");
    Serial.println(latitude_recebida);

    Serial.print("Longitude recebida = ");
    Serial.println(longitude_recebida);

    Serial.print("Altitude recebida = ");
    Serial.println(altitude_recebida);

    Serial.print("Velocidade recebida = ");
    Serial.println(velocidade_recebida);

    Serial.print("RSSI = ");
    Serial.println(rssi);

    Serial.print("SNR = ");
    Serial.println(snr);

    float distancia_rssi = pow(10.0, (LoRa_1m - rssi) / (10.0 * Perda_Ambiente));

    Serial.print("Distancia por RSSI = ");
    Serial.println(distancia_rssi);

    float lat1Rad = radians(latitude_recebida);
    float lon1Rad = radians(longitude_recebida);

    float lat2Rad = radians(latitude);
    float lon2Rad = radians(longitude);

    float deltaLat = lat2Rad - lat1Rad;
    float deltaLon = lon2Rad - lon1Rad;

    float a = sin(deltaLat / 2) * sin(deltaLat / 2) + cos(lat1Rad) * cos(lat2Rad) * sin(deltaLon / 2) * sin(deltaLon / 2);

    float c = 2 * atan2(sqrt(a), sqrt(1 - a));

    float distancia_GPS = RAIO_TERRA * c;

    Serial.print("Distancia por GPS = ");
    Serial.println(distancia_GPS);

    if ((distancia_rssi < 100)||((distancia_GPS < 100)&&(latitude != 0)&&(latitude_recebida != 0))){

      Serial.print("\nALERTA DE PROXIMIDADE\n");

    }
    else{

      Serial.print("\nLocal Seguro\n");

    }
  }
}
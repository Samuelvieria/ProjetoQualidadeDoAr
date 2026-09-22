/*
  Estacao de Qualidade do Ar - versao KIT REAL (NodeMCU ESP8266 Amica)

  Sensores do kit:
    - DHT11  (temperatura + umidade) -> D2  (GPIO4)
    - MQ-135 (qualidade do ar)       -> A0  (pino analogico unico do ESP8266)

  Saidas:
    - LED verde    -> D5 (GPIO14)
    - LED amarelo  -> D6 (GPIO12)
    - LED vermelho -> D7 (GPIO13)
    - Buzzer       -> D1 (GPIO5)

  Pinos evitados de proposito: D8 (GPIO15) tem pulldown interno e D4 (GPIO2) e
  strapping pin - com carga neles o ESP8266 pode nao dar boot. D1 e D2 nao tem
  esse problema.

  Alimentacao: o DHT11 vai no 3V3 (se for alimentado em 5 V, a linha de dados
  chega a 5 V num GPIO que tolera 3,3 V). O MQ-135 vai no Vin/5 V, porque o
  aquecedor interno nao atinge a temperatura em 3,3 V e puxa ~150 mA, acima do
  que o regulador de 3,3 V da placa entrega.

  Biblioteca necessaria (Arduino IDE > Gerenciar Bibliotecas):
    "DHT sensor library" (Adafruit) + "Adafruit Unified Sensor"
*/

#include <DHT.h>

#define PINO_DHT   D2
#define TIPO_DHT   DHT11
#define PINO_MQ135 A0

#define LED_VERDE    D5
#define LED_AMARELO  D6
#define LED_VERMELHO D7
#define BUZZER       D1

// O ADC do ESP8266 tem 10 bits (0..1023) e fundo de escala de 1,0 V na placa Amica
// (ha um divisor resistivo na placa). O MQ-135 entrega ate 5 V, entao use um divisor
// de tensao na saida analogica do sensor: MQ-135 A0 -- 10k -- no -- 10k -- GND,
// com "no" indo para o A0 do NodeMCU. Sem isso voce satura a leitura.
const int LIMITE_MODERADO = 300;
const int LIMITE_RUIM     = 600;

const unsigned long INTERVALO = 2000;
unsigned long ultimaLeitura = 0;

DHT dht(PINO_DHT, TIPO_DHT);

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  Serial.println();
  Serial.println(F("Estacao de Qualidade do Ar - iniciando..."));
  Serial.println(F("O MQ-135 precisa de 24-48h de burn-in e ~3 min de pre-aquecimento"));
  Serial.println(F("a cada ligada para dar leituras estaveis."));
  Serial.println(F("Temp(C)\tUmid(%)\tGas(ADC)\tStatus"));
}

void loop() {
  if (millis() - ultimaLeitura < INTERVALO) return;
  ultimaLeitura = millis();

  float temperatura = dht.readTemperature();
  float umidade     = dht.readHumidity();
  int   gas         = analogRead(PINO_MQ135);

  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println(F("Falha ao ler o DHT11 - verifique a fiacao e o resistor de pull-up."));
    return;
  }

  const char *status;
  bool alarme = false;
  if (gas < LIMITE_MODERADO) {
    status = "BOM";
    digitalWrite(LED_VERDE, HIGH);
    digitalWrite(LED_AMARELO, LOW);
    digitalWrite(LED_VERMELHO, LOW);
  } else if (gas < LIMITE_RUIM) {
    status = "MODERADO";
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_AMARELO, HIGH);
    digitalWrite(LED_VERMELHO, LOW);
  } else {
    status = "RUIM";
    digitalWrite(LED_VERDE, LOW);
    digitalWrite(LED_AMARELO, LOW);
    digitalWrite(LED_VERMELHO, HIGH);
    alarme = true;
  }

  if (alarme) {
    tone(BUZZER, 1000, 300);
  } else {
    noTone(BUZZER);
  }

  Serial.print(temperatura, 1);
  Serial.print(F("\t"));
  Serial.print(umidade, 0);
  Serial.print(F("\t"));
  Serial.print(gas);
  Serial.print(F("\t\t"));
  Serial.println(status);
}

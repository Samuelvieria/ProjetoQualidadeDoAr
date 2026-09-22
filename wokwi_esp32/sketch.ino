/*
  Estacao de Qualidade do Ar - ESP32
  Simulacao: https://wokwi.com  |  Hardware: Kit IoT Iniciante

  Grandezas lidas:
    - Temperatura e umidade ... DHT   -> GPIO4   (1-wire)
    - Qualidade do ar ......... MQ    -> GPIO34  (ADC1, input-only)
    - Luminosidade ............ LDR   -> GPIO35  (ADC1, input-only)
    - Chuva ................... AO    -> GPIO32  (ADC1)
    - Pressao e temperatura ... BMP180-> GPIO21/22 (I2C)

  Saidas:
    - LED verde    -> GPIO25   (ar bom)
    - LED amarelo  -> GPIO26   (ar moderado)
    - LED vermelho -> GPIO27   (ar ruim)
    - Buzzer       -> GPIO14   (alarme no nivel ruim)
    - Botao        -> GPIO13   (INPUT_PULLUP, silencia o alarme)

  Por que esses pinos: o ADC2 do ESP32 fica indisponivel quando o Wi-Fi esta
  ativo, entao todo sinal analogico usa ADC1 (GPIO 32-39). GPIO 0, 2, 12 e 15
  sao strapping pins e foram evitados para nao atrapalhar o boot.
*/

#include <Wire.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>

// ---------------------------------------------------------------------------
// Simulacao x hardware real: o Wokwi so tem DHT22; o kit traz um DHT11.
// Trocar apenas esta linha ao gravar na placa fisica.
#define TIPO_DHT DHT22   // <- no kit real: DHT11
// ---------------------------------------------------------------------------

#define PINO_DHT    4
#define PINO_GAS    34
#define PINO_LDR    35
#define PINO_CHUVA  32
#define PINO_SDA    21
#define PINO_SCL    22

#define LED_VERDE    25
#define LED_AMARELO  26
#define LED_VERMELHO 27
#define BUZZER       14
#define BOTAO        13

// O ESP32 nao tem tone(); o buzzer e acionado pelo periferico LEDC (PWM).
// A API do LEDC mudou no core ESP32 3.x: ledcSetup() e ledcAttachPin() foram
// removidas em favor de ledcAttach(), e ledcWrite() passou a receber o PINO no
// lugar do CANAL. Os dois caminhos ficam abaixo para o sketch compilar tanto no
// core 2.x quanto no 3.x (que e o usado hoje pelo Wokwi).
#define CANAL_BUZZER 0
#define FREQ_ALARME  1000
#define RES_BUZZER   8

static inline void buzzerInit(uint8_t pino) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(pino, FREQ_ALARME, RES_BUZZER);
#else
  ledcSetup(CANAL_BUZZER, FREQ_ALARME, RES_BUZZER);
  ledcAttachPin(pino, CANAL_BUZZER);
#endif
}

static inline void buzzerEscrever(uint8_t pino, uint8_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pino, duty);
#else
  (void)pino;
  ledcWrite(CANAL_BUZZER, duty);
#endif
}

// O ADC do ESP32 tem 12 bits: 0..4095, fundo de escala ~3,3 V.
const int ADC_MAX = 4095;

// Limiares do sensor de gas em valor bruto do ADC. Rode uma vez em ar limpo,
// veja o valor de repouso no Monitor Serial e ajuste estes dois a partir dele.
const int LIMITE_MODERADO = 1200;
const int LIMITE_RUIM     = 2200;

// Abaixo deste valor o sensor de chuva considera a superficie molhada.
const int LIMITE_CHUVA = 2000;

const unsigned long INTERVALO = 2000;
unsigned long ultimaLeitura = 0;

// Estado do botao de silenciar (antirrepique por tempo).
bool alarmeSilenciado = false;
unsigned long ultimoClique = 0;

DHT dht(PINO_DHT, TIPO_DHT);
Adafruit_BMP085 bmp;
bool bmpOk = false;

// Le o pino varias vezes e devolve a media, para reduzir o ruido do ADC.
int lerMedia(uint8_t pino, uint8_t amostras = 16) {
  long soma = 0;
  for (uint8_t i = 0; i < amostras; i++) {
    soma += analogRead(pino);
    delay(2);
  }
  return soma / amostras;
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(BOTAO, INPUT_PULLUP);

  buzzerInit(BUZZER);
  buzzerEscrever(BUZZER, 0);

  dht.begin();

  Wire.begin(PINO_SDA, PINO_SCL);
  bmpOk = bmp.begin();
  if (!bmpOk) {
    Serial.println(F("BMP180 nao encontrado - seguindo sem pressao."));
  }

  Serial.println();
  Serial.println(F("=== Estacao de Qualidade do Ar (ESP32) ==="));
  Serial.println(F("Temp(C)\tUmid(%)\tGas\tLuz\tChuva\tPressao(hPa)\tStatus"));
}

void loop() {
  // Botao silencia ou reativa o alarme sonoro, com 250 ms de antirrepique.
  if (digitalRead(BOTAO) == LOW && millis() - ultimoClique > 250) {
    ultimoClique = millis();
    alarmeSilenciado = !alarmeSilenciado;
    Serial.print(F(">> Alarme "));
    Serial.println(alarmeSilenciado ? F("silenciado") : F("reativado"));
  }

  if (millis() - ultimaLeitura < INTERVALO) return;
  ultimaLeitura = millis();

  float temperatura = dht.readTemperature();
  float umidade     = dht.readHumidity();
  int   gas         = lerMedia(PINO_GAS);
  int   luzBruta    = lerMedia(PINO_LDR);
  int   chuvaBruta  = lerMedia(PINO_CHUVA);

  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println(F("Falha ao ler o DHT - verifique a fiacao do pino de dados."));
    return;
  }

  int luzPercentual = map(luzBruta, 0, ADC_MAX, 0, 100);
  bool chovendo = chuvaBruta < LIMITE_CHUVA;

  // Classificacao do ar em tres faixas.
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

  buzzerEscrever(BUZZER, (alarme && !alarmeSilenciado) ? 128 : 0);

  Serial.print(temperatura, 1);
  Serial.print(F("\t"));
  Serial.print(umidade, 0);
  Serial.print(F("\t"));
  Serial.print(gas);
  Serial.print(F("\t"));
  Serial.print(luzPercentual);
  Serial.print(F("\t"));
  Serial.print(chovendo ? F("SIM") : F("nao"));
  Serial.print(F("\t"));

  if (bmpOk) {
    Serial.print(bmp.readPressure() / 100.0, 1);
  } else {
    Serial.print(F("--"));
  }

  Serial.print(F("\t\t"));
  Serial.println(status);
}

# Estação de Qualidade do Ar — NodeMCU ESP8266

Leitura de **temperatura**, **umidade** e **qualidade do ar** com o Kit IoT Iniciante,
classificando o ar em três faixas (BOM / MODERADO / RUIM) com sinalização por LEDs e
alarme sonoro.

## Por onde começar

| Quero... | Arquivo |
|---|---|
| Montar na protoboard | [MONTAGEM.md](MONTAGEM.md) — passo a passo e testes |
| Ver o esquema de ligação | [ESQUEMA.md](ESQUEMA.md) — diagramas e mapa de pinos |
| O modelo da montagem em JSON | [hardware.json](hardware.json) |
| Gravar na placa | [esp8266_kit_real/](esp8266_kit_real/) |
| Simular antes de montar | [wokwi_esp32/](wokwi_esp32/) |

## As duas versões

| Pasta | Onde roda | Placa | Sensores |
|---|---|---|---|
| [esp8266_kit_real/](esp8266_kit_real/) | **Hardware do kit** | NodeMCU ESP8266 | DHT11 + MQ-135 |
| [wokwi_esp32/](wokwi_esp32/) | Simulador [Wokwi](https://wokwi.com) | ESP32 DevKit-C | DHT22 + MQ2 + BMP180 + LDR |

A versão do Wokwi existe porque o simulador não tem ESP8266 com os sensores do kit. Ela usa
ESP32 e as peças equivalentes mais próximas, mantendo a mesma lógica de classificação — serve
para testar o comportamento antes de ter o circuito na mão. As substituições estão
documentadas em [wokwi_esp32/README.md](wokwi_esp32/README.md).

## Pinagem

| Componente | Pino | GPIO |
|---|---|---|
| DHT11 VCC / GND | `3V3` / `GND` | — |
| DHT11 DATA | `D2` | 4 |
| MQ-135 VCC / GND | `Vin` / `GND` | — |
| MQ-135 AO (via divisor 10k/10k) | `A0` | — |
| LED verde + 220 Ω | `D5` | 14 |
| LED amarelo + 220 Ω | `D6` | 12 |
| LED vermelho + 220 Ω | `D7` | 13 |
| Buzzer | `D1` | 5 |

`D3`, `D4` e `D8` ficam livres de propósito: são strapping pins ou têm pull-down interno, e
carga neles pode impedir o boot do ESP8266.

## Compilar e gravar

```bash
arduino-cli core install esp8266:esp8266
arduino-cli lib install "DHT sensor library" "Adafruit Unified Sensor"
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 esp8266_kit_real
arduino-cli upload  --fqbn esp8266:esp8266:nodemcuv2 -p /dev/cu.usbserial-XXXX esp8266_kit_real
```

Monitor Serial a **115200**.

## Os três erros que inutilizam o projeto

1. **DHT11 em 5 V** — a linha de dados passaria a 5 V num GPIO que tolera 3,3 V. Use o `3V3`.
2. **MQ-135 direto no A0** — a saída chega a 5 V e satura a leitura em 1023 para sempre.
   Use o divisor de 10 kΩ / 10 kΩ descrito em [ESQUEMA.md](ESQUEMA.md).
3. **MQ-135 alimentado pelo 3V3** — o aquecedor puxa ~150 mA, acima do que o regulador da
   placa entrega, e em 3,3 V ele nem atinge a temperatura de operação. Use o `Vin`.

## Calibração

Os valores de `LIMITE_MODERADO` e `LIMITE_RUIM` no sketch são **chute inicial, não
calibração**. Depois de montar, deixe o sensor 3 minutos ligado em ar limpo, anote o valor da
coluna `Gas` e ajuste a partir dele.

As leituras são **ADC bruto (0–1023), não ppm**. Converter para ppm exige a curva do
datasheet do MQ-135 e um ponto de referência conhecido.

Na primeira vez o MQ-135 precisa de 24 a 48 h ligado (*burn-in*) para estabilizar. Antes
disso o valor oscila sozinho e não adianta calibrar.

## Estado

- Sketch do ESP8266: **compilado e verificado** (`esp8266:esp8266:nodemcuv2`, core 3.1.2) —
  240 KB de flash (22%), 28 KB de RAM (35%).
- Sketch do ESP32 / Wokwi: **não compilado localmente**. O `diagram.json` tem JSON válido e
  os IDs de peça e nomes de pino foram conferidos contra a documentação do Wokwi e a
  definição oficial da placa. A compilação acontece ao iniciar a simulação.

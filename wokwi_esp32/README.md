# Versão ESP32 no simulador Wokwi

## Como abrir

1. Acesse [wokwi.com/projects/new/esp32](https://wokwi.com/projects/new/esp32).
2. Abra a aba **diagram.json**, apague o conteúdo e cole o
   [diagram.json](diagram.json) desta pasta.
3. Abra a aba **sketch.ino**, apague e cole o [sketch.ino](sketch.ino).
4. Clique em **Start Simulation**. O Wokwi baixa sozinho as bibliotecas listadas em
   [libraries.txt](libraries.txt).

## Peças e pinagem

| Grandeza | Peça no Wokwi | Pino do ESP32 |
|---|---|---|
| Temperatura + umidade | `wokwi-dht22` | GPIO4 |
| Qualidade do ar | `wokwi-gas-sensor` (MQ2) | GPIO34 |
| Luminosidade | `wokwi-photoresistor-sensor` | GPIO35 |
| Chuva (simulada) | `wokwi-potentiometer` | GPIO32 |
| Pressão | `board-bmp180` | GPIO21/22 (I2C) |
| LEDs verde/amarelo/vermelho | `wokwi-led` + 220 Ω | GPIO25/26/27 |
| Buzzer | `wokwi-buzzer` | GPIO14 |
| Botão (silencia alarme) | `wokwi-pushbutton` | GPIO13 |

**Por que esses pinos:** o ADC2 do ESP32 fica indisponível quando o Wi-Fi está ativo, então
todo sinal analógico usa ADC1 (GPIO 32–39). GPIO 34 e 35 são *input-only*. GPIO 0, 2, 12 e
15 são strapping pins e foram evitados.

## Substituições em relação ao kit

Duas, e só duas:

| Kit real | Wokwi | Motivo |
|---|---|---|
| DHT11 | DHT22 | O Wokwi não tem DHT11. Mesmo protocolo e mesma biblioteca — troque `#define TIPO_DHT` no sketch. |
| Sensor de chuva | Potenciômetro | Não existe sensor de chuva no Wokwi. Os dois são analógicos; o código não distingue. |

O MQ-135 vira o `wokwi-gas-sensor` (MQ2), também analógico. Existe um MQ-135 no Wokwi, mas
só como *custom chip* de terceiros (`mq135.chip.c`), que exigiria arquivos extras no projeto.

## Como testar

Clique em cada sensor durante a simulação para abrir seu controle:

- **DHT22** — sliders de temperatura e umidade.
- **Gas sensor** — slider de ppm. Suba até os LEDs trocarem de verde para amarelo e vermelho.
- **Potenciômetro** — gire para cruzar o `LIMITE_CHUVA` e ver a coluna `Chuva` virar `SIM`.
- **Botão** — silencia e reativa o alarme sonoro.

## Estado de verificação

O `diagram.json` tem JSON válido e todos os IDs de peça e nomes de pino foram conferidos
contra a documentação do Wokwi e contra a definição oficial da placa
(`wokwi/wokwi-boards`). **O sketch não foi compilado** — o core ESP32 do `arduino-cli`
passa de 2 GB e não coube no disco desta máquina. A compilação acontece quando você clicar
em *Start Simulation*.

O ponto de atenção conhecido é o buzzer: o core ESP32 3.x removeu `ledcSetup()` e
`ledcAttachPin()`. O sketch trata os dois casos com `#if ESP_ARDUINO_VERSION_MAJOR >= 3`,
mas isso não foi exercitado por um compilador aqui.

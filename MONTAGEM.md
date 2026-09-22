# Montagem na protoboard — NodeMCU ESP8266

Guia da montagem física do kit. O código correspondente é
[esp8266_kit_real.ino](esp8266_kit_real/esp8266_kit_real.ino), já compilado e verificado
para a placa `nodemcuv2`.

---

## 0. Antes de espetar qualquer coisa

**O NodeMCU tem que ficar a cavalo sobre a canaleta central da protoboard.** Se as duas
fileiras de pinos caírem do mesmo lado da canaleta, cada pino fica em curto com o de frente.

**Identifique o seu DHT11:**

- **3 pinos** (com PCB): já tem o pull-up embutido. Leia a serigrafia — costuma ser
  `S`/`+`/`−` ou `GND`/`DATA`/`VCC`. A ordem varia por fabricante, não presuma.
- **4 pinos** (sensor cru): com a grade virada para você, da esquerda para a direita é
  **VCC, DATA, NC, GND**. Precisa de um resistor de **10 kΩ** entre VCC e DATA — sem ele
  a leitura falha sempre.

**Código de cores dos resistores:**

| Valor | Faixas |
|---|---|
| 220 Ω | vermelho, vermelho, marrom, dourado |
| 10 kΩ | marrom, preto, laranja, dourado |

---

## 1. Os dois erros que inutilizam o projeto

### DHT11 alimentado em 5 V
A linha de dados passaria a chegar a 5 V num GPIO que tolera 3,3 V. **Alimente o DHT11 no
pino 3V3.**

### MQ-135 ligado direto no A0
O MQ-135 precisa de **5 V** no VCC — em 3,3 V o aquecedor interno não atinge a temperatura
de operação e a leitura vira ruído. Mas alimentado em 5 V a saída AO pode chegar a 5 V, e o
A0 do NodeMCU aceita no máximo ~3,3 V. Sem divisor a leitura satura em 1023 para sempre:

```
MQ-135 AO ──[ 10 kΩ ]──┬──[ 10 kΩ ]── GND
                       │
                       └───────────── A0 do NodeMCU
```

O VCC do MQ-135 vai no pino **Vin** (ou **VU**), nunca no 3V3: o aquecedor puxa ~150 mA,
acima do que o regulador de 3,3 V da placa entrega.

---

## 2. Pinagem

| Componente | Pino do NodeMCU | GPIO |
|---|---|---|
| DHT11 VCC / GND | **3V3** / **GND** | — |
| DHT11 DATA | **D2** | GPIO4 |
| MQ-135 VCC / GND | **Vin (5 V)** / **GND** | — |
| MQ-135 AO (via divisor) | **A0** | ADC |
| LED verde + 220 Ω | **D5** | GPIO14 |
| LED amarelo + 220 Ω | **D6** | GPIO12 |
| LED vermelho + 220 Ω | **D7** | GPIO13 |
| Buzzer (+) | **D1** | GPIO5 |

Cada LED: pino do NodeMCU → resistor 220 Ω → perna longa (ânodo); perna curta (cátodo) → GND.

**Pinos evitados de propósito:** D8 (GPIO15) tem pull-down interno e D4 (GPIO2) é strapping
pin. Com carga neles o ESP8266 pode não dar boot. D1 e D2 não têm esse problema.

---

## 3. Monte e teste por partes

Não ligue tudo de uma vez — se falhar, você não saberá onde. A cada etapa, grave o sketch e
abra o Monitor Serial em **115200**:

1. **Só o NodeMCU + USB.** Confirme que a porta aparece na IDE e que a gravação conclui.
2. **Só o DHT11.** As colunas `Temp` e `Umid` devem mostrar valores plausíveis. Se aparecer
   `Falha ao ler o DHT11`, o problema é pull-up ou ordem dos pinos — resolva aqui antes de
   seguir.
3. **MQ-135 com o divisor.** Espere ~3 min de pré-aquecimento e anote o valor da coluna
   `Gas` em ar limpo. É esse número que define os seus limiares.
4. **LEDs**, depois o **buzzer**.

---

## 4. Calibração

Os valores de `LIMITE_MODERADO` e `LIMITE_RUIM` no sketch são chute inicial, não calibração.
Depois do passo 3, use o valor de repouso que você anotou:

```cpp
const int LIMITE_MODERADO = <repouso> + 100;
const int LIMITE_RUIM     = <repouso> + 300;
```

Ajuste soprando perto do sensor (álcool em gel ou um isqueiro sem acender também elevam a
leitura) e veja onde os LEDs trocam.

**Burn-in:** na primeira vez o MQ-135 precisa de 24 a 48 h ligado para estabilizar. Antes
disso o valor sobe e desce sozinho e não adianta calibrar.

Os valores são ADC bruto, **não ppm**. Converter para ppm exige a curva do datasheet e um
ponto de referência conhecido.

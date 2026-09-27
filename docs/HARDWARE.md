# Hardware e comissionamento

## Lista proposta

| Item | Quantidade | Observação |
|---|---:|---|
| ESP32 DevKit com WROOM-32 | 1 | Perfil provisório `esp32dev`, flash 4 MB; confirmar variante |
| Resistência | 1 | Tensão/potência ainda não fornecidas |
| SSR para resistência | 1 | Carga resistiva AC, passagem por zero, dissipador dimensionado |
| Bomba de recirculação | 1 | Tensão, corrente de partida e temperatura máxima pendentes |
| SSR para bomba | 1 | Compatível com motor/carga indutiva e com AC ou DC conforme a bomba |
| DS18B20 externo de 3 fios | 1 | Sonda/poço termométrico adequados ao processo; alimentação externa |
| LCD HD44780 20×4 + PCF8574 | 1 | Backpack padrão P0=RS, P1=RW, P2=EN, P3=BL, P4–P7=dados |
| Botões momentâneos | 4 | Contato para GND, pull-up de entrada, debounce 35 ms |
| Buzzer ativo + driver | Opcional | Para os avisos sonoros descritos no manual |
| Fonte isolada, drivers e conversão de nível | Conforme projeto | Validar níveis de 3,3 V e corrente dos SSRs |
| Corte térmico, emergência física, contato de potência | Conforme projeto | Independentes do firmware |

**SSR não torna potência ou corrente irrelevantes.** Com apenas uma resistência, o software controla diretamente o percentual e usa watts apenas como estimativa visual. Dimensionamento da rede, SSR, dissipação, corrente de partida da bomba e proteções é trabalho de hardware. A frequência de comutação da bomba não é PWM.

## GPIOs provisórios

| Função | GPIO | Condição |
|---|---:|---|
| SSR resistência | 25 | Ativo HIGH; não acionado nos builds distribuídos |
| SSR bomba | 27 | Ativo HIGH; não acionado nos builds distribuídos |
| Buzzer ativo | 26 | Opcional; adaptar driver/corrente |
| DS18B20 | 4 | OneWire, pull-up para 3,3 V; sem alimentação parasita |
| LCD SDA/SCL | 21 / 22 | I²C 100 kHz; endereço inicial 0x27, editável |
| Voltar, −, +, OK | 16 / 17 / 18 / 19 | Não aplicável a variantes com conflitos de PSRAM |
| Retorno da emergência | 32 | NC para GND; aberto = inseguro |
| Nível opcional | 33 | NC/saída compatível; habilitação explícita |
| Fluxo opcional | 23 | Entrada digital de fluxo presente, não contador de pulsos |

Não usar esses números como esquema de rede elétrica. Não alimentar entradas ESP32 com 5 V. Muitos backpacks têm pull-ups para 5 V: avaliar conversor bidirecional. O modelo do backpack ainda precisa de confirmação; outros mapeamentos PCF exigem adaptar o driver.

As saídas precisam permanecer OFF com ESP32 sem energia ou pinos flutuantes. Usar drivers com polaridade confirmada e pull-downs externos. Esta versão implementa somente ativo HIGH, sem configuração remota de polaridade. SSR fechado por falha exige corte de potência independente. Comando OFF não comprova ausência de tensão/corrente.

## Pendências para operação real

1. Confirmar placa/flash, SSRs, bomba, resistência, alimentação, sonda e posições físicas.
2. Dimensionar proteção contra sobrecorrente, DR, aterramento, isolamento, dissipação e invólucro.
3. Instalar e testar emergência física e proteção térmica independente. O GPIO de retorno não substitui o corte físico.
4. Definir nível/fluxo reais, ou documentar sua ausência. Confirmação de água na tela não detecta panela vazia.
5. Validar limite térmico da bomba, temperatura máxima, PI, janela de SSR e comportamento sem recirculação.
6. Completar os testes com cargas de baixa tensão e água.
7. Somente então criar um perfil com `CERVEJINO_SIM=0` e `CERVEJINO_ENABLE_OUTPUTS=1`. Não basta alterar essa macro para considerar o equipamento comissionado; ela é um bloqueio de engenharia, não uma certificação.

A atualização de firmware não substitui validação de hardware. Não foram realizadas medições de precisão, overshoot, isolamento ou tempo real de corte na placa.

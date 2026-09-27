# Cervejino — ESP32 / LCD 20×4

Firmware próprio para panela de cerveja, inspirado no fluxo público do manual **Speidel Braumeister PLUS 10/20/50/100 L**. Não é firmware da Speidel e não oferece integração com MySpeidel.

**Escopo confirmado: 1 resistência via SSR, 1 bomba via SSR, 1 sensor DS18B20, LCD HD44780 20×4 e 4 botões.** A alteração para hardware único substitui o escopo inicial de dois sensores/resistências.

## Estado do projeto

Versão inicial para simulação e bancada. Os dois perfis distribuídos têm **GPIOs de potência desabilitados em compilação**. Não conectar cargas de rede antes do comissionamento elétrico e dos ensaios descritos em [HARDWARE](docs/HARDWARE.md). Os testes de software não demonstram precisão térmica nem segurança elétrica real.

- Máquina de estados: água, escorva, aquecimento, dupla confirmação do malte, mostura, mash-out, retirada do cesto, fervura, resfriamento e conclusão.
- PI único, anti-windup, PWM por janela de 2 s; bomba somente ON/OFF.
- LCD 20×4 e botões como interface principal; receitas e configuração editáveis sem Wi-Fi.
- Até 8 receitas locais, 10 patamares e 6 adições de lúpulo; 3 exemplos demonstrativos.
- Fervura por potência, início por platô térmico configurável ou confirmação manual.
- Sensor com CRC, conversão assíncrona, detecção de dados vencidos e calibração.
- Persistência com duas cópias, CRC32, versão e recuperação somente após confirmação.
- Interface web local autenticada, JSON import/export, comandos com revisão/ID e atualização de firmware em modo parado.
- Testes nativos com AddressSanitizer/UndefinedBehaviorSanitizer e compilação ESP32 no GitHub Actions.

Leia [PARIDADE_MANUAL](docs/PARIDADE_MANUAL.md) para distinguir comportamentos documentados, adaptações e recursos dependentes de acessórios.

## Compilar e testar

Dependências fixadas: PlatformIO Core 6.1.18, espressif32 6.10.0 (Arduino-ESP32 2.0.17), OneWire 2.3.8, ArduinoJson 6.21.5. Placa provisória: `esp32dev`, WROOM-32 com 4 MB de flash; confirme a placa real.

```sh
python -m pip install platformio==6.1.18
python -m platformio run -e esp32-sim
python -m platformio run -e esp32-bench
bash scripts/test_native.sh
```

`esp32-sim` simula a temperatura e não usa GPIOs de potência; LCD/botões podem ser usados. A ausência de LCD físico é tolerada **somente na simulação**. `esp32-bench` usa sensor e LCD reais, mas não permite iniciar aquecimento: serve para conferir leitura, endereços, menus e entradas. Nenhum perfil fornecido permite energizar resistência ou bomba.

Se o ambiente impedir LeakSanitizer de acessar `/proc`, executar `ASAN_OPTIONS=detect_leaks=0 bash scripts/test_native.sh`; AddressSanitizer e UBSan continuam ativos. No CI, a verificação de leaks permanece habilitada.

## Primeira gravação

```sh
python -m platformio run -e esp32-sim -t upload
# SOMENTE na primeira instalação: apaga dados existentes da partição de arquivos!
python -m platformio run -e esp32-sim -t uploadfs
python -m platformio device monitor -b 115200
```

O firmware não formata uma memória que falhou na montagem. Um filesystem não provisionado gera falha de memória até a gravação inicial. Nas atualizações normais, grave somente o firmware: **não repita uploadfs** para conservar receitas e checkpoints.

## Operação

Botões, da esquerda para a direita: **Voltar/Pausar, −/Subir, +/Descer, OK**. A última linha explica as ações. Durante a brassagem, Voltar pausa imediatamente; em pausa, Voltar oferece cancelamento e OK oferece retomada. Setas alternam resumo, temperatura, acionamentos, eventos, ajuste e ajuda.

Configuração: valores elétricos/termomecânicos são provisórios. Em Diagnóstico, conecte exatamente um DS18B20 e confirme sua vinculação; o endereço fica persistido. O programa não substitui automaticamente um sensor desconectado por outro.

No menu Wi-Fi, OK ativa o AP `Cervejino`; a senha aleatória aparece no LCD. Conecte e abra `http://192.168.4.1`. Usuário HTTP: `cervejino`; senha: a mesma mostrada no LCD. Credenciais novas a cada boot. O AP é opcional: a brassagem independe dele. A página permite conectar ao roteador durante a sessão. Não expor a interface HTTP na internet.

OTA: com o processo parado, selecione **Preparar atualização**, aguarde o resultado aplicado e envie o `firmware.bin` do perfil correto. As saídas permanecem bloqueadas durante a manutenção; ao reiniciar, não ocorre retomada automática. Não é atualização assinada nem serviço de nuvem.

## Documentação

- [Hardware, GPIOs e pendências](docs/HARDWARE.md)
- [Máquina de estados, tarefas e segurança](docs/ARQUITETURA.md)
- [Correspondência com o manual](docs/PARIDADE_MANUAL.md)
- [Menus e telas 20×4](docs/INTERFACE.md)
- [Roteiro de validação](docs/VALIDACAO.md)
- [Receita JSON demonstrativa](examples/receita-demonstrativa.json)

O projeto deve ser ensaiado com água e supervisão antes de qualquer brassagem real. O sensor único não detecta nível e não constitui proteção térmica independente.

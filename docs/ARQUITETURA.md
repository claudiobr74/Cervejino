# Arquitetura e estados

`src/core/` é C++ sem Arduino: modelo, validação, PI, PWM, estados, debounce e navegação. `hardware.cpp` adapta DS18B20, PCF8574 e GPIO. `serialization.cpp` valida JSON e produz snapshots. `storage.cpp` mantém dois arquivos independentes. `web.cpp` trabalha em tarefa separada e envia comandos à tarefa principal por fila. Somente a tarefa de controle altera o modelo; a tarefa de guarda só pode desligar saídas.

Controle monotônico: `esp_timer_get_time()/1000` (64 bits). Loop cooperativo com espera de scheduler de 5 ms. Rede e gravação de flash em tarefas de prioridade menor. I²C com timeout de 5 ms por transação e LCD atualizado um caractere por passagem. A soma das transações e os tempos de flash precisam ser medidos no hardware.

Guarda: atraso de heartbeat acima de 300 ms desliga saídas e trava a sessão; watchdog de 3 s pode reiniciar. Não são proteção independente contra falha do SoC, cache/flash, fonte ou SSR. A segurança física permanece obrigatória.

| Estado | Calor | Bomba | Saída normal |
|---|---|---|---|
| Pronto / concluído / cancelado | OFF | OFF | Início confirmado |
| Conferir água | OFF | OFF | OK confirma líquido |
| Escorva | OFF | Ciclos | Aquecer para malte |
| Aquecer grãos | PI | ON se permitida | Temperatura atingida |
| Adicionar / confirmar grãos | OFF | OFF | Duas confirmações |
| Mostura / mash-out | PI e limite da fase | Ciclos | Tempo efetivo cumprido |
| Mostura concluída / retirar cesto / iniciar fervura | OFF | OFF | Confirmações locais ou web |
| Aquecer fervura | Potência total | Permitida até limite térmico | Platô ou confirmação manual |
| Fervura | Percentual da receita | OFF | Tempo encerrado |
| Fervura concluída | OFF | OFF | Confirmar fim |
| Whirlpool opcional | OFF | ON abaixo do limite | Tempo de circulação cumprido |
| Resfriar | OFF | OFF | Temperatura-alvo atingida |
| Pausa | OFF | OFF | Retomar confirmado ou cancelar |
| Recuperação | OFF | OFF | Avaliação e confirmação |
| Falha | OFF | OFF | Reconhecimento e novo início |
| Manual | PI ou percentual | Comando do usuário, limitado | Temporizador, parada ou falha |

O PI é apropriado à inércia térmica e evita derivada sobre uma medição quantizada. Ganhos iniciais são provisórios. Integração condicional limita windup; ao pausar/interditar calor, integral é zerada. Reinício parte de potência calculada com integral zero, não de um valor antigo acumulado. Não há autotune automático.

PWM é uma janela fixa inicial de 2 segundos: 0% sempre OFF, 100% sempre ON enquanto autorizado. Não usa LEDC para o SSR. Potência nominal só converte percentual em estimativa de watts; não mede consumo. Bomba SSR funciona apenas liga/desliga, com tempo mínimo OFF antes de religar; desligamentos de segurança ignoram qualquer dwell.

Patamar: inicia após ficar dentro da tolerância por `stableSec`. Por padrão, tempo fora da faixa não conta e exige nova estabilização. `elapsed` inclui aquecimento do estado; `effective` é o tempo válido. Pausa congela ambos. Etapas editadas ficam pausadas até retomada, preservando tempo efetivo já realizado. Alterações não reescrevem a receita de origem.

Sensor: endereço ROM persistido, CRC do scratchpad, conversão externa 12 bits não bloqueante (800 ms), marcadores voláteis TH/TL detectam reset durante conversão. 85 °C é aceito se a conversão e marcadores forem válidos. Leitura igual repetida pode ser temperatura estável; não é tratada como sensor congelado somente pelo valor. Sem hardware adicional, um sensor com falha plausível não pode ser diagnosticado com garantia.

Persistência: JSON versão 1 encapsulado em cabeçalho com CRC32 e sequência; escreve cópia inativa, fecha e relê antes de promovê-la. Falha de escrita bloqueia o controlador. Cópia anterior válida é usada se a mais recente estiver truncada. Checkpoint a cada 30 s durante operação e em alterações, limitado para preservar flash. Pode haver repetição de um aviso se faltar energia antes de seu checkpoint: não há garantia de exatamente uma vez entre RAM, flash e ação humana. Revisar avisos ao recuperar.

Receitas/configs em RAM podem estar aplicadas antes de a gravação terminar; a interface distingue isso. Checkpoint conserva receita em execução, estado, índice, tempos e adições. Interrupção em modo manual é descartada, sem retomada de saída arbitrária. Tempo sem energia não conta. Falhas não são retomadas automaticamente.

API: HTTP Basic local + token anti-CSRF aleatório, IDs crescentes por boot, revisão otimista e fila limitada. Edição local bloqueia edição concorrente via web. GET não altera saídas. `202` significa recebido; `/api/result` informa aplicação/recusa. Desconectar rede não altera o processo. HTTP não é TLS: usar somente rede confiável e não encaminhar portas.

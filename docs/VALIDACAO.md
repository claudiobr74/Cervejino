# Validação e limites

## Automatizada

Executar `bash scripts/test_native.sh`. Cobertura inclui PWM 0–100%, sensor único, NaN, ausência de conversão válida, leitura vencida, 85 °C válido, sobretemperatura crua, LCD, emergência, nível/fluxo, pausa, dwell da bomba, confirmação de malte, estabilidade, rampas, platô da fervura, eventos, checkpoints, botões, edição local e ciclo completo. Sanitizadores verificam acesso à memória/UB no núcleo nativo.

GitHub Actions compila `esp32-sim` e `esp32-bench` separadamente e produz firmware, bootloader, partições e imagem inicial LittleFS. O sucesso do compilador não substitui ensaios no dispositivo.

## Bancada sem rede elétrica

1. Verificar placa, pinagem e tensões com todos os SSRs desconectados.
2. Provisionar filesystem uma única vez. Conferir boot sem qualquer acionamento.
3. Testar menus nos quatro botões, botão preso no boot, ruído de contato, valores extremos e descarte.
4. Criar, editar, duplicar/excluir receita; reiniciar e verificar persistência.
5. Vincular um DS18B20. Aquecer/resfriar a sonda com referência; calibrar offset.
6. Remover sonda, LCD, contatos de nível/fluxo e emergência; verificar falhas. Testar SDA/SCL presos.
7. Interromper alimentação durante checkpoint; confirmar recuperação com saídas OFF e tempo desligado excluído.
8. Wi-Fi: sessão autenticada, senha incorreta, repetição de ID, revisão antiga, editar no LCD simultaneamente, cliente lento/desconectado, payload inválido. Medir latência do controle sob carga de rede.
9. Atualização: rejeitar com brassagem ativa; preparar em modo parado; testar imagem inválida e transferência interrompida. Conferir que receitas não foram apagadas e que cargas ficam OFF.
10. Medir memória livre e estabilidade por 24 h, inclusive 8 receitas no tamanho máximo.

## Cargas de teste e água

Somente após revisão elétrica: habilitar perfil de comissionamento em circuito de teste. Medir com instrumento a janela de PWM e o estado das saídas em reset/brownout. Validar corte independente de SSR, watchdog e emergência. Ajustar PI e limiares com água no volume de trabalho; registrar overshoot, erro em regime, tempo de aquecimento, platô de fervura na altitude local, efeito das pausas e funcionamento da bomba.

Não testar falha de SSR com uma panela vazia ou sem corte térmico independente. Não há detecção garantida de SSR fechado, sensor deslocado, circulação real ou nível sem os respectivos sensores externos.

## Não verificado fisicamente

Precisão, compatibilidade elétrica, resposta hidráulica, dissipação de SSRs, redução de ruído, latência máxima no ESP32, durabilidade de flash, recuperação após todas as formas de brownout e segurança elétrica não foram medidos neste ambiente. A versão é para validação em bancada, não equipamento certificado.

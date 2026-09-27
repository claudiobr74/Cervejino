# Correspondência funcional com o manual fornecido

Referência: manual original em inglês Speidels Braumeister PLUS 10, 20, 50, 100 litres, 51 páginas, fornecido pelo usuário em 27/09/2026. A edição descreve tela **touch**, não LCD de quatro linhas: o Cervejino adapta funções a quatro botões. Não copia software, identidade visual ou protocolos privados.

| Manual / seção | Cervejino | Diferença ou limite |
|---|---|---|
| 5.1–5.3: iniciar, receitas, manual, ajustes | Menus locais e web | Tela alfanumérica em páginas |
| 6.3: três receitas iniciais | Três receitas demonstrativas | Não alegamos que sejam as três oficiais, cujos parâmetros completos não constam no texto |
| 6.4.1: confirmar água e ventilar bomba | Precheck e escorva configurável | Aquecimento fica OFF na escorva; ajuste conservador deste projeto |
| 6.4.1: atingir temperatura, sinalizar malte | Estado Adicionar grãos, buzzer opcional | Sem buzzer, aviso visual |
| 6.4.1: segunda confirmação de malte | Estado Graos colocados? | Duas ações OK distintas, botão mantido não repete |
| 6.4: patamares, contagem após atingir alvo | PI, estabilização, patamares automáticos | Tolerância e estabilidade configuráveis; não são parâmetros internos conhecidos da Speidel |
| 6.4: pausas da bomba a cada 10 minutos | Ciclo inicial 600 s ON / 30 s OFF | Os 30 s são escolha provisória, pois o texto não informa a duração exata da pausa |
| 6.4: cancelar, continuar/abortar, visão geral, ajuda | Pausa, confirmação de cancelamento, páginas de resumo e ajuda | Pausa desliga calor e bomba |
| 6.4–6.5: fim da mostura e retirada do cesto | Aviso, confirmação, tela de retirada e confirmação para iniciar fervura | Resistência e bomba OFF durante intervenção |
| 6.6: contagem da fervura após estabilização | Detecção de platô acima da referência e confirmação manual alternativa | Janela 60 s / faixa 0,3 °C configuráveis; heurística própria, requer validação com água e altitude |
| 6.6: editar tempo durante operação | Página Ajustar, com pausa e confirmação | Retomada explícita; não modifica receita salva |
| 6.6: seis adições de lúpulo | Seis eventos em segundos restantes | Bitmask persiste eventos sinalizados/reconhecidos |
| 6.6: aviso de fim e Finished | Fervura concluída, desligamento imediato, confirmação | Não mantém calor aguardando confirmação |
| 2.3: bomba OFF em temperatura alta | Limite configurável da bomba | Valor da bomba artesanal precisa ser informado |
| 5.3: manual P/H e alvo | Modo manual com bomba, aquecimento, temperatura ou potência | Ajuste em execução pela página Ajustar, com pausa |
| 6.7: whirlpool recomendado por agitação | Etapa opcional de circulação | Bomba não garante whirlpool sem geometria hidráulica adequada |
| 6.7: resfriamento e auto cooling com válvula acessória | Monitoramento até alvo; calor OFF | Válvula não pertence ao hardware solicitado; não há controle automático de água fria |
| 5.3: Wi-Fi / supervisão | AP e rede local, interface offline | Sem nuvem, acesso pela rede local |
| 5.4: atualização WLAN | Upload autenticado de firmware em manutenção | Sem assinatura/verificação de procedência por fabricante; operador deve usar build correto |
| 5.3: MySpeidel e sincronização | Importação/exportação JSON local | MySpeidel não implementado: requer serviço e protocolo não especificados |
| 5.4: Tilt/fermentação como atualizações | Não implementados | Manual menciona possibilidades, sem especificar algoritmo ou protocolo; hardware pedido não inclui hidrômetro/refrigeração |
| 5.2: idioma, unidades, som | Português, °C, litros, som configurável | Catálogo multilíngue/unidades alternativas não implementado nesta versão |

O escopo é uma reprodução funcional do **processo de brassagem** no hardware confirmado. Não é uma reprodução integral de todos os acessórios, serviços e futuras versões comerciais. As diferenças estão explícitas para não confundir funções reais com simulações.

Fontes técnicas usadas: [manual público Speidel](https://www.speidels-braumeister.de/en/service/instruction-manuals.html), [datasheet ESP32-WROOM-32](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf), [DS18B20](https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf), [I²C Arduino-ESP32](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/i2c.html), [PlatformIO espressif32 6.10.0](https://github.com/platformio/platform-espressif32/releases/tag/v6.10.0).

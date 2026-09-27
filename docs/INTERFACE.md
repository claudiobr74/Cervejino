# LCD 20×4 e quatro botões

Ordem dos botões: Voltar/Pausar, −/Subir, +/Descer, OK. Toque tem debounce; somente as setas repetem durante edição numérica. Pressão longa é detectada mas não inicia/cancela processo. Botões pressionados no boot devem ser soltos antes de gerar eventos.

Menu principal: Iniciar, Receitas, Manual, Configuração, Calibração, Diagnóstico e Wi-Fi.

Receitas: selecionar, editar, duplicar, excluir; opção Nova ao final. Nome pode ser editado com campo de posição e campo de caractere. Tempos são em segundos. Campos de bomba e rampas são por etapa. Adições no LCD usam identificação numérica; nomes detalhados e descrição podem ser editados pelo JSON web. Limite de 6 adições.

Em cada editor: setas navegam; OK entra no campo; setas alteram; OK/Voltar encerra a edição do campo. Ao final há Salvar. Voltar fora de um campo oferece descarte. Salvar pede confirmação e valida a receita/configuração completa.

Durante a brassagem, setas alternam 6 páginas: resumo, temperatura, acionamentos, eventos, ajustar fase e ajuda. Em Ajustar, OK pausa e abre campos da etapa atual (ou modo manual). Depois de salvar, permanece pausado. Tempos são totais: reduzir para menos que o tempo já cumprido finaliza a etapa ao retomar.

Exemplos de 20 caracteres por linha (o renderizador sempre preenche/trunca para exatamente 20):

```text
Contando patamar    
T:65.0 Alvo:65.0    
Resta 00:35:00      
Pausa <    >  OK    
```

```text
Adicionar graos    
T:38.0 Alvo:0.0    
Resta 00:00:00     
Pausa <    >  OK   
```

O alvo 0 em estados de intervenção significa ausência de controle térmico; a potência fica OFF. O aviso de temperatura inválida usa ERRO. O indicador da bomba reflete o comando, não confirma vazão quando não há sensor de fluxo.

No Wi-Fi, a terceira linha mostra a senha do AP ativo. Endereço padrão `192.168.4.1`. A página HTTP é inteiramente embarcada, sem CDNs. Receita e configuração são editadas em JSON na primeira versão; o LCD oferece os campos guiados.

Alarmes bloqueantes tomam a tela. OK reconhece e retorna a Pronto, sem religar cargas; se a causa persistir a falha reaparece. Memória corrompida ou tarefa travada exigem correção/reinicialização. Um botão da interface não substitui parada física de emergência.

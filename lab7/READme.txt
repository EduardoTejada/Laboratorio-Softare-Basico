SERVIDOR HTTP MULTI-PROCESSO COM POLL
=====================================

Descrição:
----------
Servidor HTTP que utiliza multiprocessamento para atender múltiplas requisições
simultaneamente, com limite configurável de processos filhos e usando poll para
controle de timeout nas conexões.

Características:
---------------
- Multiprocessamento com até N processos filhos (configurável)
- Uso de poll em vez de select para controle de timeout
- Proteção contra erros EINTR em chamadas de sistema
- Timeout de 4 segundos para aceitação de conexões
- Mensagem de "Servidor Ocupado" quando limite é atingido
- Prevenção de processos zombies com handler de sinal SIGCHLD

Compilação:
-----------
   flex -o atv3_lex.c atv3_lex.l
   bison -d -o atv3_sin.tab.c atv3_sin.y
   gcc -o servidor_multi_poll servidor_multi_poll.c myList.c myString.c processar-requisicoes.c atv3_lex.c atv3_sin.tab.c

ou simplesmente:
   make

Execução:
---------
./servidor_multi_poll [IP] [PORTA]

Para fazer a requisição no navegador:
firefox http://127.0.0.1:8080/
OU
telnet 127.0.0.1 8080

Exemplos:
---------
./servidor_multi_poll 127.0.0.1 8080
./servidor_multi_poll 0.0.0.0 8080

Configuração do projeto:
-------------
- Número máximo de processos: altere MAX_PROCESSOS_FILHO no código
- Webspace: diretório './meu-webspace/'
- Timeout: 4 segundos (configurável nas funções)

Configuração do ambiente:
---------
./criar_webspace.sh

Estrutura do Webspace:
----------------------
meu-webspace/
├── index.html
├── dir1/
│   ├── texto1.html
│   └── texto2.html (SEM ACESSO)
└── dir2/ (SEM ACESSO)

Testes Recomendados:
-------------------
1. Teste com múltiplas conexões simultâneas via telnet
2. Teste de limite de processos (conectar mais clientes que MAX_PROCESSOS_FILHO)
3. Teste de timeout (conectar mas não enviar dados)
4. Teste com sinais (kill -SIGCHLD para simular término de filhos)

Teste Padronizado:
./testar_limite.sh

Arquivos Principais:
-------------------
- servidor_multi_poll.c : Servidor principal
- myList.c : Manipulação de listas
- myString.c : Funções de string
- processar-requisicoes.c : Processamento HTTP
- atv3_lex.l : Analisador léxico
- atv3_sin.y : Analisador sintático

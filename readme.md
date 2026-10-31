# Trabalho Pratico de Sistemas Operacionais - Problema da Montanha-Russa (Tema 10)

## Descricao

Este projeto implementa a solucao concorrente para o Problema da Montanha-Russa (Tema 10) utilizando C e POSIX Threads (pthread.h). A aplicacao coordena o acesso de passageiros a um carrinho, garantindo que o embarque so ocorra quando o carro estiver disponivel, o passeio ocorra com capacidade total e o desembarque seja concluido antes do proximo grupo embarcar.

## Requisitos

- Compilador GCC compativel com o padrao C11
- Suporte a biblioteca POSIX Threads (pthread)
- Python 3 com as bibliotecas pandas e matplotlib (para geracao dos graficos)

## Compilacao

Para compilar o codigo-fonte, execute o seguinte comando no terminal:

```bash
gcc -std=c11 -O2 -Wall -Wextra -pthread montanha.c -o simulacao
```

## Parametros de Execucao

O programa recebe tres argumentos via linha de comando:

```bash
./simulacao <Capacidade_C> <Total_Passageiros_N> <Numero_Repeticao>
```

- Capacidade_C: Capacidade maxima do carrinho por viagem.
- Total_Passageiros_N: Quantidade total de passageiros na simulacao (deve ser multiplo de C).
- Numero_Repeticao: Identificador da repeticao (usado para registros no CSV).

## Matriz Experimental (25 Execucoes)

Abaixo estao os comandos necessarios para executar toda a matriz experimental do projeto. Cada configuracao contem 5 repeticoes.

### 1. Configuracao de Referencia (C=4, N=32)
```bash
./simulacao 4 32 1
./simulacao 4 32 2
./simulacao 4 32 3
./simulacao 4 32 4
./simulacao 4 32 5
```

### 2. Variando Capacidade C=2 (N=32)
```bash
./simulacao 2 32 1
./simulacao 2 32 2
./simulacao 2 32 3
./simulacao 2 32 4
./simulacao 2 32 5
```

### 3. Variando Capacidade C=8 (N=32)
```bash
./simulacao 8 32 1
./simulacao 8 32 2
./simulacao 8 32 3
./simulacao 8 32 4
./simulacao 8 32 5
```

### 4. Variando Passageiros N=16 (C=4)
```bash
./simulacao 4 16 1
./simulacao 4 16 2
./simulacao 4 16 3
./simulacao 4 16 4
./simulacao 4 16 5
```

### 5. Variando Passageiros N=64 (C=4)
```bash
./simulacao 4 64 1
./simulacao 4 64 2
./simulacao 4 64 3
./simulacao 4 64 4
./simulacao 4 64 5
```

## Arquivos de Saida

Ao executar os comandos acima, a aplicacao gera e atualiza:

1. resumo.csv: Contem os dados consolidados do experimento (vazao, tempo total, esperas media, p95 e maxima).
2. eventos_C[C]_N[N]_rep[REP].csv: Contem o historico temporal detalhado dos eventos de cada execucao para a construcao do grafico de estado por tempo.
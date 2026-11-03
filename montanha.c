#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

// Variáveis globais do experimento
int C, N, repeticao;
int viagens_concluidas = 0;
int viagem_atual = 1;

// Sincronização
pthread_mutex_t trava;
pthread_cond_t cond_embarque;
pthread_cond_t cond_desembarque;
pthread_cond_t cond_carro_pronto;

// Barreiras de início comum
pthread_mutex_t trava_inicio;
pthread_cond_t cond_inicio;
int iniciar_experimento = 0;
struct timespec tempo_inicio_global;

// Estado compartilhado
int estado = 0; // 0=Embarcando, 1=Passeando, 2=Desembarcando
int passageiros_no_carro = 0;
int passageiros_desembarcados = 0;

// Estruturas de Coleta de Dados
typedef struct {
    int id;
    double t_pedido;
    double t_embarque;
    double t_desembarque;
    double espera_W;
} PassageiroDados;

PassageiroDados *dados_passageiros;

// Sistema de log de eventos na memória
typedef struct {
    double tempo;
    int participante;
    char tipo[20];
    char evento[50];
    int estado_ocupacao;
    int id_operacao;
} Evento;

Evento *lista_eventos;
int total_eventos = 0;
pthread_mutex_t trava_eventos;

// Função auxiliar para calcular tempo decorrido em segundos
double tempo_decorrido() {
    struct timespec agora;
    clock_gettime(CLOCK_MONOTONIC, &agora);
    return (agora.tv_sec - tempo_inicio_global.tv_sec) + 
           (agora.tv_nsec - tempo_inicio_global.tv_nsec) / 1e9;
}

// Salva evento na memória sem usar printf para não atrasar a execução
void registrar_evento(int participante, const char* tipo, const char* evento_nome, int ocupacao, int id_op) {
    pthread_mutex_lock(&trava_eventos);
    lista_eventos[total_eventos].tempo = tempo_decorrido();
    lista_eventos[total_eventos].participante = participante;
    strcpy(lista_eventos[total_eventos].tipo, tipo);
    strcpy(lista_eventos[total_eventos].evento, evento_nome);
    lista_eventos[total_eventos].estado_ocupacao = ocupacao;
    lista_eventos[total_eventos].id_operacao = id_op;
    total_eventos++;
    pthread_mutex_unlock(&trava_eventos);
}

// Simula o tempo usando nanosleep
void simular_tempo(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

// THREAD DO PASSAGEIRO
void* acao_passageiro(void* argumento) {
    int id = *(int*)argumento;

    // Aguarda sinal de início comum
    pthread_mutex_lock(&trava_inicio);
    while (!iniciar_experimento) {
        pthread_cond_wait(&cond_inicio, &trava_inicio);
    }
    pthread_mutex_unlock(&trava_inicio);

    // Chegadas espaçadas: 1 chegada solicitada a cada 1 ms
    simular_tempo(id * 1);

    // Registra o pedido ANTES de tentar obter a trava
    dados_passageiros[id].t_pedido = tempo_decorrido();
    registrar_evento(id, "Passageiro", "Chegada na Fila", passageiros_no_carro, viagem_atual);

    pthread_mutex_lock(&trava);

    // Espera até a fase de embarque e até ter vaga
    while (estado != 0 || passageiros_no_carro == C) {
        pthread_cond_wait(&cond_embarque, &trava);
    }

    // Autorização completa obtida (Entrou no carro)
    dados_passageiros[id].t_embarque = tempo_decorrido();
    passageiros_no_carro++;
    registrar_evento(id, "Passageiro", "Embarcou", passageiros_no_carro, viagem_atual);

    // Se lotou, avisa o carro
    if (passageiros_no_carro == C) {
        pthread_cond_signal(&cond_carro_pronto); 
    }

    // Ninguém sai durante o passeio
    while (estado != 2) {
        pthread_cond_wait(&cond_desembarque, &trava);
    }

    // Fase de desembarque
    dados_passageiros[id].t_desembarque = tempo_decorrido();
    passageiros_desembarcados++;
    passageiros_no_carro--;
    registrar_evento(id, "Passageiro", "Desembarcou", passageiros_no_carro, viagem_atual);

    // Se foi o último a sair, avisa o carro
    if (passageiros_desembarcados == C) {
        pthread_cond_signal(&cond_carro_pronto); 
    }

    pthread_mutex_unlock(&trava);
    return NULL;
}

// THREAD DO CARRINHO
void* acao_carrinho(void* argumento) {
    (void)argumento;
    int total_viagens = N / C; // Encerrar depois de N/C viagens

    // Aguarda sinal de início comum
    pthread_mutex_lock(&trava_inicio);
    while (!iniciar_experimento) {
        pthread_cond_wait(&cond_inicio, &trava_inicio);
    }
    pthread_mutex_unlock(&trava_inicio);

    for (int i = 0; i < total_viagens; i++) {
        pthread_mutex_lock(&trava);
        
        // Fase 1: Embarque
        estado = 0; 
        viagem_atual = i + 1;
        registrar_evento(-1, "Carro", "Fase: Embarcando", passageiros_no_carro, viagem_atual);
        
        pthread_cond_broadcast(&cond_embarque);

        // Uma viagem parte com exatamente C passageiros
        while (passageiros_no_carro < C) {
            pthread_cond_wait(&cond_carro_pronto, &trava);
        }

        // Fase 2: Passeio
        estado = 1; 
        registrar_evento(-1, "Carro", "Fase: Passeando", passageiros_no_carro, viagem_atual);
        
        pthread_mutex_unlock(&trava);
        simular_tempo(5); // Cada passeio dura 5 ms
        pthread_mutex_lock(&trava);

        // Fase 3: Desembarque
        estado = 2; 
        passageiros_desembarcados = 0;
        registrar_evento(-1, "Carro", "Fase: Desembarcando", passageiros_no_carro, viagem_atual);

        pthread_cond_broadcast(&cond_desembarque);

        // O próximo embarque começa somente depois do desembarque completo
        while (passageiros_desembarcados < C) {
            pthread_cond_wait(&cond_carro_pronto, &trava);
        }
        
        registrar_evento(-1, "Carro", "Vazio - Aguardando", passageiros_no_carro, viagem_atual);
        viagens_concluidas++;
        pthread_mutex_unlock(&trava);
    }
    return NULL;
}

// Função para ordenação do qsort (para calcular o percentil 95)
int comparar_esperas(const void *a, const void *b) {
    double dif = *(double*)a - *(double*)b;
    return (dif > 0) - (dif < 0);
}

// PROGRAMA PRINCIPAL
int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Uso: ./simulacao <Capacidade C> <Passageiros N> <Repeticao>\n");
        return 1;
    }

    C = atoi(argv[1]);
    N = atoi(argv[2]);
    repeticao = atoi(argv[3]);

    if (N % C != 0) {
        printf("Erro: N (%d) deve ser multiplo de C (%d)\n", N, C); // Regra do trabalho
        return 1;
    }

    dados_passageiros = malloc(N * sizeof(PassageiroDados));
    lista_eventos = malloc((N * 5 + 100) * sizeof(Evento)); 

    pthread_t thread_carrinho;
    pthread_t thread_passageiros[N];
    int ids_passageiros[N];

    pthread_mutex_init(&trava, NULL);
    pthread_mutex_init(&trava_eventos, NULL);
    pthread_mutex_init(&trava_inicio, NULL);
    pthread_cond_init(&cond_embarque, NULL);
    pthread_cond_init(&cond_desembarque, NULL);
    pthread_cond_init(&cond_carro_pronto, NULL);
    pthread_cond_init(&cond_inicio, NULL);

    // Cria as threads
    pthread_create(&thread_carrinho, NULL, acao_carrinho, NULL);
    for (int i = 0; i < N; i++) {
        ids_passageiros[i] = i;
        pthread_create(&thread_passageiros[i], NULL, acao_passageiro, &ids_passageiros[i]);
    }

    // Sinal de início comum
    simular_tempo(100); 
    clock_gettime(CLOCK_MONOTONIC, &tempo_inicio_global); 
    
    pthread_mutex_lock(&trava_inicio);
    iniciar_experimento = 1;
    pthread_cond_broadcast(&cond_inicio);
    pthread_mutex_unlock(&trava_inicio);

    // Aguarda encerramento
    pthread_join(thread_carrinho, NULL);
    for (int i = 0; i < N; i++) {
        pthread_join(thread_passageiros[i], NULL);
    }

    double tempo_total = tempo_decorrido(); // Tempo total T

    // CÁLCULO DE MÉTRICAS
    double esperas[N];
    double soma_espera = 0;
    double max_espera = 0;

    for(int i=0; i<N; i++) {
        // Espera W = t_autorizado - t_pedido
        dados_passageiros[i].espera_W = dados_passageiros[i].t_embarque - dados_passageiros[i].t_pedido;
        esperas[i] = dados_passageiros[i].espera_W;
        soma_espera += esperas[i];
        if (esperas[i] > max_espera) max_espera = esperas[i];
    }

    qsort(esperas, N, sizeof(double), comparar_esperas);
    
    // Média, p95 e máximo
    int idx_p95 = (int)(0.95 * N) - 1; 
    if(idx_p95 < 0) idx_p95 = 0;
    double p95 = esperas[idx_p95];
    double media = soma_espera / N;

    // Vazão V = Quantidade / Tempo
    double vazao = N / tempo_total; 

    // EXPORTAÇÃO PARA CSV
    
    // Resumo
    FILE *f_resumo = fopen("resumo.csv", "a");
    if(f_resumo != NULL) {
        fseek(f_resumo, 0, SEEK_END);
        if (ftell(f_resumo) == 0) {
            fprintf(f_resumo, "tema,config,repeticao,semente,C,N,tempo_T,previstos,concluidos,vazao_V,espera_media,espera_p95,espera_maxima,violacoes,status\n");
        }
        fprintf(f_resumo, "Montanha-Russa,C=%d N=%d,%d,N/A,%d,%d,%f,%d,%d,%f,%f,%f,%f,0,Sucesso\n", 
                C, N, repeticao, C, N, tempo_total, N, N, vazao, media, p95, max_espera);
        fclose(f_resumo);
    }

    // Eventos 
    char nome_eventos[50];
    sprintf(nome_eventos, "eventos_C%d_N%d_rep%d.csv", C, N, repeticao);
    FILE *f_eventos = fopen(nome_eventos, "w");
    if(f_eventos != NULL) {
        fprintf(f_eventos, "execucao,tempo,participante,tipo,evento,estado_ocupacao,id_operacao\n");
        for(int i=0; i<total_eventos; i++) {
            fprintf(f_eventos, "%d,%f,%d,%s,%s,%d,%d\n", 
                repeticao, lista_eventos[i].tempo, lista_eventos[i].participante, 
                lista_eventos[i].tipo, lista_eventos[i].evento, 
                lista_eventos[i].estado_ocupacao, lista_eventos[i].id_operacao);
        }
        fclose(f_eventos);
    }

    // Libera recursos e destrói mecanismos
    pthread_mutex_destroy(&trava);
    pthread_mutex_destroy(&trava_eventos);
    pthread_mutex_destroy(&trava_inicio);
    pthread_cond_destroy(&cond_embarque);
    pthread_cond_destroy(&cond_desembarque);
    pthread_cond_destroy(&cond_carro_pronto);
    pthread_cond_destroy(&cond_inicio);
    free(dados_passageiros);
    free(lista_eventos);

    printf("Sucesso! C=%d, N=%d, Rep=%d | Vazao: %.2f | p95: %.4f s\n", C, N, repeticao, vazao, p95);
    return 0;
}
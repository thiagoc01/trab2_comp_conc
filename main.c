/*
  * Programa: Algoritmo concorrente para ordenar blocos de valores de um arquivo de entrada
  * Alunos: Gabriele Jandres Cavalcanti e Thiago Figueiredo Lopes de Castro | DREs: 119159948 e 118090044
  * Disciplina: Computacao Concorrente - 2021.1
  * Modulo 2 - Trabalho 2
  * Data: Setembro de 2021
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "./libs/timer.h"
#include "./resources/merge_sort.h"

#define NUM_PRODUCERS 1 // numero de threads produtoras
#define NUM_BUFFER_BLOCKS 10 // numero de blocos do buffer (10 blocos de tamanho N)

/* Variaveis globais */
sem_t consumers; // exclusao mutua entre os consumidores
sem_t empty_slot; // sinalizacao de um slot preenchido para consumo
sem_t full_slot; // sinalizacao de que todos os slots estao preenchidos
sem_t free_file; // exclusao mutua entre os consumidores para a escrita no arquivo, permitindo apenas um por vez.
pthread_mutex_t mutex; // exclusao mutua para a verificacao da necessidade de ler a ultima linha orfa, em caso da presenca da mesma
int *buffer[10]; // buffer compartilhado entre os arquivos
int num_consumers; // numero de consumidores solicitado
int N; // tamanho dos blocos
int quantity_of_numbers; // quantidade de numeros lida na primeira linha dos arquivos
int num_rows; // divisao entre a quantidade de numeros e o tamanho dos blocos. Se houver resto, havera uma linha orfa
char *input_file; // arquivo de entrada
char *output_file; // arquivo de saida

/* Funcoes */

// Funcao utilizada para checar corretude do arquivo de saida apos a ordenacao.
void analyze_output_correctness(FILE *out)
{
    out = fopen(output_file, "r");

    int count_line = 0;
    int smallest, num;

    while (!feof(out))
    {
        // Se o arquivo esta ordenado, o primeiro valor eh sempre o menor da linha
        fscanf(out, "%d ", &smallest);

        for (int i = 1 ; i < N ; i++)
        {
            fscanf(out, "%d ", &num);

            if (num < smallest) // Encontrou um numero no meio da linha que eh menor do que o primeiro
            {
                printf("Ha uma inconsistencia no arquivo. O numero %d na linha %d eh menor que %d.\n", num, count_line + 1, smallest);
                fclose(out);
                return;
            }
        }
        count_line++;
    }

    // Se o numero de linhas for diferente que o contado no inicio do programa, alguma linha nao foi escrita no arquivo de saida
    if (quantity_of_numbers % N == 0)
    {
        if (count_line != (quantity_of_numbers / N))
        {
            printf("Ha menos ou mais linhas no arquivo de saida em relacao ao arquivo de entrada.\n");
            fclose(out);
            return;
        }
    }
    else
    {
        if (count_line != ((quantity_of_numbers / N) + 1 ))
        {
            printf("Ha menos ou mais linhas no arquivo de saida em relacao ao arquivo de entrada.\n");
            fclose(out);
            return;
        }
    }

    printf("O arquivo de saida %s esta correto!\n", output_file);
    fclose(out);
}

// Funcao utilizada para o processamento da ultima linha presente no caso que a quantidade de numeros nao eh divisivel pelo tamanho do bloco
void ordenate_last_row(int *array, FILE *output)
{
    sem_wait(&full_slot); // aguarda o slot para leitura

    // A linha orfa SEMPRE sera a ultima no buffer, pois o arquivo esta sendo lido do inicio ao fim em ordem e so ha um produtor. Logo, sera exatamente o numero de linhas MOD numero de buffers.
    for (int i = 0; i < quantity_of_numbers % N; i++)
        array[i] = buffer[num_rows % NUM_BUFFER_BLOCKS][i];

    sem_post(&empty_slot); // sinaliza que leu a ultima linha
    merge_sort(array, 0, (quantity_of_numbers % N) - 1); // ordenacao
    sem_wait(&free_file); // sinaliza que vai utilizar o arquivo

    for (int i = 0; i < quantity_of_numbers % N; i++)
        fprintf(output, "%d ", array[i]);

    fprintf(output, "\n");
    sem_post(&free_file); // libera o arquivo
}

// Funcao do produtor
void *read_file(void *file_descriptor)
{
    FILE *input = (FILE *) file_descriptor;
    int count = 0; // conta quantas linhas UNIFORMES faltam
    int in = 0; // indica qual slot sera guardado a leitura

    while (count < num_rows)
    {
        sem_wait(&empty_slot); // verifica se ha algum slot liberado para producao

        for (int i = 0; i < N; i++)
            fscanf(input, "%d", &buffer[in][i]); // guarda o numero no slot atual e na posicao do vetor

        in = (in + 1) % NUM_BUFFER_BLOCKS; // incrementa o contador do slot
        sem_post(&full_slot); // sinaliza que um consumidor pode utilizar um slot
        count++; // indica que leu uma linha
    }

    if (quantity_of_numbers % N != 0) // N nao eh divisor da quantidade de numeros no arquivo. Havera uma linha orfa.
    {
        sem_wait(&empty_slot);

        for (int i = 0; i < quantity_of_numbers % N; i++)
            fscanf(input, "%d", &buffer[in][i]);
        
        sem_post(&full_slot);
    }

    pthread_exit(NULL);
}

// Funcao dos consumidores 
void *ordenate_blocks(void *file_descriptor)
{
    FILE *output = (FILE *) file_descriptor;
    int block[N], count_rows_thread; // buffer e contador local da thread
    static int count_rows = 0, out = 0; // contador compartilhado entre as threads e contador do buffer
    static int finished_threads = 0; // apenas a ultima thread deve tratar a linha orfa. Iremos contar quantas ja acabaram.

    pthread_mutex_lock(&mutex);
    count_rows_thread = count_rows; // se a ultima thread ficar na fila, ha a possibilidade de outras ja terem iniciado.
    pthread_mutex_unlock(&mutex);

    while (count_rows_thread < num_rows)
    {
        // Verifica se nao ira ler linhas a mais
        pthread_mutex_lock(&mutex);

        count_rows++;
        count_rows_thread = count_rows;
        
        if (count_rows > num_rows)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        pthread_mutex_unlock(&mutex);

        // Padrao de consumidores
        sem_wait(&full_slot);
        sem_wait(&consumers);

        // toma os blocos do buffer
        for (int i = 0; i < N; i++)
            block[i] = buffer[out][i];
        out = (out + 1) % NUM_BUFFER_BLOCKS;

        sem_post(&consumers);
        sem_post(&empty_slot);

        merge_sort(block, 0, N - 1); // ordenacao

        // escrita dos blocos ordenados
        sem_wait(&free_file); // bloqueia arquivo para escrita

        for (int i = 0; i < N; i++)
            fprintf(output, "%d ", block[i]);

        fprintf(output, "\n");
        
        sem_post(&free_file); // libera arquivo
    }

    pthread_mutex_lock(&mutex);
    // Se nao for a ultima thread, entao deve encerrar normalmente.
    if (finished_threads != num_consumers - 1)
    {
        finished_threads++;
        pthread_mutex_unlock(&mutex);
        pthread_exit(NULL);
    }

    // Havera linha orfa apenas se um nao for multiplo/divisor do outro
    // Nao e necessario o unlock, pois somente a ultima thread chegara aqui.
    if (quantity_of_numbers % N != 0)
        ordenate_last_row(block, output);

    pthread_exit(NULL);
}

// Funcao para abrir/criar arquivo
bool handle_file(FILE **file, const char *local, const char *mode)
{
    *file = fopen(local, mode);
    if (!(*file)) return false;
    return true;
}

// Funcao para realizar a recepcao e tratamento dos argumentos da linha de comando
void handle_arguments(int argc, char **argv)
{
    if (argc < 5)
    {
        printf("Digite %s <numero-de-threads> <tamanho-do-bloco> <arquivo-de-entrada> <arquivo-de-saida>", argv[0]);
        exit(1);
    }

    num_consumers = atoi(argv[1]);
    N = atoi(argv[2]);
    input_file = argv[3];
    output_file = argv[4];

    if (num_consumers <= 0 || N <= 0)
    {
        printf("Digite um numero de consumidores e um tamanho de bloco maior que 0 \n");
        exit(2);
    }
}

// Funcao para alocar espaco para recursos
void init()
{
    // Alocacao para o buffer compartilhado
    for (int i = 0; i < NUM_BUFFER_BLOCKS; i++)
    {
        buffer[i] = (int *) calloc(N, sizeof(int));
        if (!(buffer[i]))
        {
            printf("Erro ao alocar espaco para o buffer %d \n", i);
            for (int j = 0; j < i; j++) free(buffer[j]);
            exit(8);
        }
    }

    // Inicializacao dos semaforos e mutex
    sem_init(&consumers, 0, 1); // apenas um consumidor pode ler o slot
    sem_init(&empty_slot, 0, NUM_BUFFER_BLOCKS); // produtor pode guardar numeros em todos os slots inicialmente
    sem_init(&full_slot, 0, 0); // nao ha slot ocupado no comeco. Consumidores irao aguardar
    sem_init(&free_file, 0, 1); // seguindo o padrao leitor/escritor, apenas uma thread pode escrever no arquivo de saida
    pthread_mutex_init(&mutex, NULL); // exclusao mutua para leitura da variavel de verificacao da linha orfa
}

// Funcao para liberar recursos alocados no comeco do programa 
void destroy()
{
    for (int i = 0; i < NUM_BUFFER_BLOCKS; i++) free(buffer[i]);
    sem_destroy(&consumers);
    sem_destroy(&empty_slot);
    sem_destroy(&full_slot);
    sem_destroy(&free_file);
    pthread_mutex_destroy(&mutex);
}

// Funcao para abrir os arquivos de entrada e saida 
void open_files(FILE **input, FILE **output) {
    if (!handle_file(input, input_file, "r"))
    {
        printf("Erro ao abrir o arquivo de entrada.\n");
        exit(3);
    }

    if (!handle_file(output, output_file, "w"))
    {
        printf("Erro ao criar o arquivo de saida.\n");
        fclose(*input);
        exit(4);
    }

    fscanf(*input, "%d", &quantity_of_numbers); // le a quantidade de numeros no arquivo
    num_rows = quantity_of_numbers / N;
    if (quantity_of_numbers < N)
    {
        printf("A quantidade de numeros no arquivo nao pode ser menor que o tamanho do bloco solicitado \n");
        fclose(*input);
        fclose(*output);
        destroy();
        exit(8);
    }
}

// Funcao para fechar os arquivos de entrada e saida
void close_files(FILE *input, FILE *output) {
    fclose(input);
    fclose(output);
}
// Funcao para aguardar o termino das threads
void join_threads(pthread_t *threads, FILE *input, FILE *output, int num_threads)
{
    for (int i = 0; i < num_threads; i++)
    {
        if (pthread_join(*(threads + i), NULL))
        {
            printf("Erro ao aguardar a thread %d.\n", i);
            close_files(input, output);
            destroy();
            exit(7);
        }
    }

    close_files(input, output);
}

// Funcao para criar as threads
void create_threads(pthread_t **threads, int consumers, int producers, FILE *input, FILE *output)
{
    *threads = (pthread_t *) malloc(sizeof(pthread_t) * (consumers + producers));
    if (*threads == NULL) {
        printf("Erro na alocacao de memoria para threads \n");
        exit(2);
    }

    // cria thread produtora
    if (pthread_create((*threads), NULL, read_file, (void *)input))
    {
        printf("Erro ao criar a thread produtora \n");
        close_files(input, output);
        destroy();
        exit(5);
    }

    // cria threads consumidoras
    for (int i = 1; i <= num_consumers; i++)
    {
        if (pthread_create((*threads) + i, NULL, ordenate_blocks, (void *)output))
        {
            printf("Erro ao criar uma thread consumidora. ID: %d.\n", i);
            close_files(input, output);
            destroy();
            exit(6);
        }
    }
}

// Funcao para mostrar o tempo total de execucao de uma tarefa
void show_task_time(double start, int num_threads) 
{
    double end;
    GET_TIME(end);
    printf("Tempo total com %d threads: %lf s\n", num_threads, end - start);
}

int main(int argc, char **argv)
{
    FILE *input, *output; // arquivos de entrada e saida
    pthread_t *threads; // identificadores das threads
    double start; // variavel para controle de tempo

    handle_arguments(argc, argv);
    open_files(&input, &output);
    init();

    GET_TIME(start);
    create_threads(&threads, num_consumers, NUM_PRODUCERS, input, output);
    join_threads(threads, input, output, num_consumers + NUM_PRODUCERS);

    show_task_time(start, num_consumers);
    analyze_output_correctness(output);

    destroy();

    return 0;
}
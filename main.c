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

#define NUM_PRODUTORES 1 // numero de threads produtoras
#define NUM_BLOCOS_BUFFER 10 // numero de blocos do buffer (10 blocos de tamanho N)

/*
 * Sem�foro de consumidores => Exclus�o m�tua entre os consumidores
 * Sem�foro de slot vazio => Sinaliza��o de um slot preenchido para consumo
 * Sem�foro de slot cheio => Sinaliza��o de que todos os slots est�o preenchidos
 * Sem�foro de arquivo livre => Exclus�o m�tua entre os consumidores para a leitura do arquivo, permitindo apenas um por vez.
*/

sem_t consumidores, slot_vazio, slot_cheio, arquivo_livre;

/* Exclus�o m�tua para a verifica��o da necessidade de ler a �ltima linha �rfa, em caso da presen�a da mesma */

pthread_mutex_t mutex;

/*
 * Buffer (10 X N) => Buffer compartilhado entre os arquivos
 * N�mero de consumidores solicitado
 * N => Tamanho dos blocos
 * Quantidade de n�meros => Lido na primeira linha dos arquivos
 * N�mero de linhas => Divis�o entre a quantidade de n�meros e o tamanho dos blocos. Se houver resto, haver� uma linha �rf�
*/

int *buffer[10], num_consumidores, N, qtd_numeros, num_linhas;

char *local_arquivo_entrada, *local_arquivo_saida; // Locais dos arquivos

/*
 * Func�o utilizada para o processamento da �ltima linha presente no caso de que o tamanho do bloco
 * n�o divide a quantidade de n�meros. Ou seja, os blocos n�o s�o uniformes.
 * Vetor => espa�o para guardar os valores
 * Arq => Arquivo de sa�da
*/

// --------------------------------------------------- FUN��ES ---------------------------------------------------

void ordena_linha_orfa(int *vetor, FILE *arq)
{
    sem_wait(&slot_cheio); // Aguarda o slot para leitura

    /*
     * A linha �rf� SEMPRE ser� a �ltima no buffer, pois o arquivo est� sendo lido do in�cio ao fim em ordem e s� h� um produtor.
     * Logo, ser� exatamente o n�mero de linhas MOD n�mero de buffers.
    */

    for (int i = 0 ; i < qtd_numeros % N ; i++)
        vetor[i] = buffer[num_linhas % NUM_BLOCOS_BUFFER][i];

    sem_post(&slot_vazio); // Sinaliza que leu a �ltima linha

    merge_sort(vetor, 0, (qtd_numeros % N) - 1); // Chama o algoritmo de ordena��o

    sem_wait(&arquivo_livre); // Sinaliza que vai utilizar o arquivo

    for (int i = 0 ; i < qtd_numeros % N ; i++)
        fprintf(arq, "%d ", vetor[i]);

    fprintf(arq,"\n");

    sem_post(&arquivo_livre); // Libera o arquivo

}

/*
 * Func�o do produtor
 * Recebe o descritor do arquivo de entrada
*/

void * le_arquivo(void *arg)
{
    FILE *arq = (FILE *) arg;

    int contador_linhas = 0; // Conta quantas linhas UNIFORMES faltam

    int in = 0; // Indica qual slot ser� guardado a leitura

    while (contador_linhas < num_linhas)
    {
        sem_wait(&slot_vazio); // Verifica se h� algum slot liberado para produ��o

        for (int i = 0 ; i < N ; i++)
        {
            fscanf(arq, "%d", &buffer[in][i]); // Guarda o n�mero no slot atual e na posi��o do vetor
            printf("%d ", buffer[in][i]);
        }

        puts("");

        in = (in + 1) % NUM_BLOCOS_BUFFER; // Incrementa o contador do slot

        sem_post(&slot_cheio); // Sinaliza que um consumidor pode utilizar um slot

        contador_linhas++; // Indica que leu uma linha
    }

    if (qtd_numeros % N != 0) // N n�o � divisor da quantidade de n�meros no arquivo. Haver� uma linha �rf�.
    {
        sem_wait(&slot_vazio);

        for (int i = 0 ; i < qtd_numeros % N ; i++)
        {
            fscanf(arq, "%d", &buffer[in][i]);
            printf("%d ", buffer[in][i]);
        }
        puts("");

        sem_post(&slot_cheio);
    }

    pthread_exit(NULL);
}

/*
 * Fun��o concorrente dos consumidores
 * Segue o padr�o de consumidores, com exce��o da �ltima verifica��o
 * No caso de todos os blocos n�o serem uniformes, haver� uma linha �rfa
 * Depois que ela � processada, � marcada uma vari�vel indicando que ela j� foi processada
 * Recebe o descritor do arquivo de sa�da.
*/

void * ordena_blocos(void *arg)
{
    FILE *arq = (FILE *) arg;

    int bloco[N], contador_linhas_thread; // Buffer e contador local da thread

    static int contador_linhas = 0, out = 0; // Contador compartilhado entre as threads e contador do buffer

    static bool deve_ler_ultima_linha = true; // Inicialmente, todas t�m a possibilidade de serem a primeira a ler uma linha �rf�.

    pthread_mutex_lock(&mutex);

    /* Se �ltima thread ficar na fila, h� a possibilidade de outras j� terem iniciado.
     * Essa linha realiza essa captura inicial.*/

    contador_linhas_thread = contador_linhas; 

    pthread_mutex_unlock(&mutex);

    while (contador_linhas_thread < num_linhas) // Mesmo racioc�nio da thread produtora
    {
        /* Verifica se n�o ir� ler linhas a mais */

        pthread_mutex_lock(&mutex);

        contador_linhas++;

        contador_linhas_thread = contador_linhas;

        if (contador_linhas > num_linhas)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        pthread_mutex_unlock(&mutex);

        /* Padr�o de consumidores */

        sem_wait(&slot_cheio);

        sem_wait(&consumidores);

        for (int i = 0 ; i < N ; i++)
            bloco[i] = buffer[out][i];

        out = (out + 1) % NUM_BLOCOS_BUFFER;

        sem_post(&consumidores);

        sem_post(&slot_vazio);
        merge_sort(bloco, 0, N-1);

        sem_wait(&arquivo_livre); // Bloqueia arquivo para escrita

        for (int i = 0 ; i < N ; i++)
            fprintf(arq, "%d ", bloco[i]);

        fprintf(arq,"\n");

        sem_post(&arquivo_livre); // Libera arquivo para escrita  
            
    }
    
    if (qtd_numeros % N != 0) // Haver� linha �rfa apenas se um n�o for m�ltiplo/divisor do outro
    {
        pthread_mutex_lock(&mutex);

        bool leu_linha_orfa = deve_ler_ultima_linha;

        if (leu_linha_orfa) // Alguma thread j� tratou da linha �rf�
            deve_ler_ultima_linha = false;

        pthread_mutex_unlock(&mutex);

        if (leu_linha_orfa) // � testado novamente apenas para liberar o lock mais rapidamente, chamando a fun��o agora
            ordena_linha_orfa(bloco, arq);
    }    

    pthread_exit(NULL);    
}

/* Fun��o unificada para abrir/criar arquivo */

bool toca_arquivo(FILE **arq, const char *local, const char *modo)
{
    *arq = fopen(local, modo);

    if (!(*arq))
        return false;

    return true;
}

/* Realiza a recep��o e tratamento dos argumentos da linha de comando */

void recebe_argumentos(int argc, char **argv)
{
    if (argc < 5)
    {
        printf("Digite %s <n�mero de consumidores> <tamanho do bloco> <local do arquivo de entrada> <local do arquivo de sa�da>.", argv[0]);
        exit(1);
    }

    num_consumidores = atoi(argv[1]);
    N = atoi(argv[2]);
    local_arquivo_entrada = argv[3];
    local_arquivo_saida = argv[4];

    if (num_consumidores <= 0 || N <= 0)
    {
        puts("Digite um n�mero de consumidores maior que 0 e um tamanho de bloco maior que 1.");
        exit(2);
    }
}

/* Aloca��o de espa�o para recursos */

void inicializa_recursos()
{
    for (int i = 0 ; i < NUM_BLOCOS_BUFFER ; i++)
    {
        buffer[i] = (int *) calloc(N, sizeof(int));

        if (!(buffer[i]))
        {
            printf("Erro ao alocar espa�o para o buffer %d.\n", i);

            for (int j = 0 ; j < i ; j++)
                free(buffer[j]);

            exit(8);
        }
    }

    sem_init(&consumidores, 0, 1); // Apenas um consumidor pode ler o slot
    sem_init(&slot_vazio, 0, NUM_BLOCOS_BUFFER); // Produtor pode guardar n�meros em todos os slots inicialmente
    sem_init(&slot_cheio, 0, 0); // N�o h� slot ocupado no come�o. Consumidores ir�o aguardar
    sem_init(&arquivo_livre, 0, 1); // Seguindo o padr�o leitor/escritor, apenas uma thread pode escrever no arquivo de sa�da
    pthread_mutex_init(&mutex, NULL); // Exclus�o m�tua para leitura da vari�vel de verifica��o da linha �rf�
}

/* Libera��o de recursos alocados no come�o do programa */

void libera_recursos()
{
    for (int i = 0 ; i < NUM_BLOCOS_BUFFER ; i++)
        free(buffer[i]);

    sem_destroy(&consumidores);
    sem_destroy(&slot_vazio);
    sem_destroy(&slot_cheio);
    sem_destroy(&arquivo_livre);
    pthread_mutex_destroy(&mutex);
}

void inicializa_threads()
{
    pthread_t threads[num_consumidores + NUM_PRODUTORES];
    FILE *arq_entrada, *arq_saida;

    if (!toca_arquivo(&arq_entrada, local_arquivo_entrada, "r"))
    {
        printf("Erro ao abrir o arquivo de entrada.\n");
        exit(3);
    }

    if (!toca_arquivo(&arq_saida, local_arquivo_saida, "w"))
    {
        printf("Erro ao criar o arquivo de sa�da.\n");
        fclose(arq_entrada);
        exit(4);
    }

    fscanf(arq_entrada, "%d", &qtd_numeros); // L� a quantidade de n�meros

    /*if (qtd_numeros % 10 != 0)
    {
        puts("A quantidade de n�meros no arquivo deve ser um m�ltiplo de 10.");
        fclose(arq_entrada);
        fclose(arq_saida);
        libera_recursos();
        exit(9);
    }*/

    num_linhas = qtd_numeros / N;

    if (qtd_numeros < N)
    {
        puts("A quantidade de n�meros no arquivo n�o pode ser menor que o tamanho do bloco solicitado.");
        fclose(arq_entrada);
        fclose(arq_saida);
        libera_recursos();
        exit(8);
    }

    if (pthread_create(&threads[0], NULL, le_arquivo, (void *) arq_entrada))
    {
        puts("Erro ao criar a thread produtora.");
        exit(5);
    }

    for (int i = 1 ; i <= num_consumidores ; i++)
    {
        if (pthread_create(&threads[i], NULL, ordena_blocos, (void *) arq_saida))
        {
            printf("Erro ao criar uma thread consumidora. ID: %d.\n", i);
            exit(6);
        }
    }

    for (int i = 0 ; i < num_consumidores + NUM_PRODUTORES ; i++)
    {
        if (pthread_join(threads[i], NULL))
        {
            printf("Erro ao aguardar a thread %d.\n", i);
            exit(7);
        }
    }

    fclose(arq_entrada);
    fclose(arq_saida);
}


int main(int argc, char **argv)
{
    recebe_argumentos(argc, argv);

    inicializa_recursos();

    inicializa_threads();

    libera_recursos();

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#include "merge_sort.h"

#define NUM_PRODUTORES 1 // Definido no roteiro
#define NUM_BLOCOS_BUFFER 10 // Definido no roteiro. São 10 bufferse e cada um com tamanho N


/*
 * Semáforo de consumidores => Exclusão mútua entre os consumidores
 * Semáforo de slot vazio => Sinalização de um slot preenchido para consumo
 * Semáforo de slot cheio => Sinalização de que todos os slots estão preenchidos
 * Semáforo de arquivo livre => Exclusão mútua entre os consumidores para a leitura do arquivo, permitindo apenas um por vez.
*/

sem_t consumidores, slot_vazio, slot_cheio, arquivo_livre;

/* Exclusão mútua para a verificação da necessidade de ler a última linha órfa, em caso da presença da mesma */

pthread_mutex_t mutex;

/*
 * Buffer (10 X N) => Buffer compartilhado entre os arquivos
 * Número de consumidores solicitado
 * N => Tamanho dos blocos
 * Quantidade de números => Lido na primeira linha dos arquivos
 * Número de linhas => Divisão entre a quantidade de números e o tamanho dos blocos. Se houver resto, haverá uma linha órfã
*/

int *buffer[10], num_consumidores, N, qtd_numeros, num_linhas;

char *local_arquivo_entrada, *local_arquivo_saida; // Locais dos arquivos

/*
 * Funcão utilizada para o processamento da última linha presente no caso de que o tamanho do bloco
 * não divide a quantidade de números. Ou seja, os blocos não são uniformes.
 * Vetor => espaço para guardar os valores
 * Arq => Arquivo de saída
*/

// --------------------------------------------------- FUNÇÕES ---------------------------------------------------

void ordena_linha_orfa(int *vetor, FILE *arq)
{
    sem_wait(&slot_cheio); // Aguarda o slot para leitura

    /*
     * A linha órfã SEMPRE será a última no buffer, pois o arquivo está sendo lido do início ao fim em ordem e só há um produtor.
     * Logo, será exatamente o número de linhas MOD número de buffers.
    */

    for (int i = 0 ; i < qtd_numeros % N ; i++)
        vetor[i] = buffer[num_linhas % NUM_BLOCOS_BUFFER][i];

    sem_post(&slot_vazio); // Sinaliza que leu a última linha

    merge_sort(vetor, 0, (qtd_numeros % N) - 1); // Chama o algoritmo de ordenação

    sem_wait(&arquivo_livre); // Sinaliza que vai utilizar o arquivo

    for (int i = 0 ; i < qtd_numeros % N ; i++)
        fprintf(arq, "%d ", vetor[i]);

    fprintf(arq,"\n");

    sem_post(&arquivo_livre); // Libera o arquivo

}

/*
 * Funcão do produtor
 * Recebe o descritor do arquivo de entrada
*/

void * le_arquivo(void *arg)
{
    FILE *arq = (FILE *) arg;

    int contador_linhas = 0; // Conta quantas linhas UNIFORMES faltam

    int in = 0; // Indica qual slot será guardado a leitura

    while (contador_linhas < num_linhas)
    {
        sem_wait(&slot_vazio); // Verifica se há algum slot liberado para produção

        for (int i = 0 ; i < N ; i++)
        {
            fscanf(arq, "%d", &buffer[in][i]); // Guarda o número no slot atual e na posição do vetor
            printf("%d ", buffer[in][i]);
        }

        puts("");

        in = (in + 1) % NUM_BLOCOS_BUFFER; // Incrementa o contador do slot

        sem_post(&slot_cheio); // Sinaliza que um consumidor pode utilizar um slot

        contador_linhas++; // Indica que leu uma linha
    }

    if (qtd_numeros % N != 0) // N não é divisor da quantidade de números no arquivo. Haverá uma linha órfã.
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
 * Função concorrente dos consumidores
 * Segue o padrão de consumidores, com exceção da última verificação
 * No caso de todos os blocos não serem uniformes, haverá uma linha órfa
 * Depois que ela é processada, é marcada uma variável indicando que ela já foi processada
 * Recebe o descritor do arquivo de saída.
*/

void * ordena_blocos(void *arg)
{
    FILE *arq = (FILE *) arg;

    int bloco[N], contador_linhas_thread; // Buffer e contador local da thread

    static int contador_linhas = 0, out = 0; // Contador compartilhado entre as threads e contador do buffer

    static bool deve_ler_ultima_linha = true; // Inicialmente, todas têm a possibilidade de serem a primeira a ler uma linha órfã.

    pthread_mutex_lock(&mutex);

    /* Se última thread ficar na fila, há a possibilidade de outras já terem iniciado.
     * Essa linha realiza essa captura inicial.*/

    contador_linhas_thread = contador_linhas; 

    pthread_mutex_unlock(&mutex);

    while (contador_linhas_thread < num_linhas) // Mesmo raciocínio da thread produtora
    {
        /* Verifica se não irá ler linhas a mais */

        pthread_mutex_lock(&mutex);

        contador_linhas++;

        contador_linhas_thread = contador_linhas;

        if (contador_linhas > num_linhas)
        {
            pthread_mutex_unlock(&mutex);
            break;
        }

        pthread_mutex_unlock(&mutex);

        /* Padrão de consumidores */

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
    
    if (qtd_numeros % N != 0) // Haverá linha órfa apenas se um não for múltiplo/divisor do outro
    {
        pthread_mutex_lock(&mutex);

        bool leu_linha_orfa = deve_ler_ultima_linha;

        if (leu_linha_orfa) // Alguma thread já tratou da linha órfã
            deve_ler_ultima_linha = false;

        pthread_mutex_unlock(&mutex);

        if (leu_linha_orfa) // É testado novamente apenas para liberar o lock mais rapidamente, chamando a função agora
            ordena_linha_orfa(bloco, arq);
    }    

    pthread_exit(NULL);    
}

/* Função unificada para abrir/criar arquivo */

bool toca_arquivo(FILE **arq, const char *local, const char *modo)
{
    *arq = fopen(local, modo);

    if (!(*arq))
        return false;

    return true;
}

/* Realiza a recepção e tratamento dos argumentos da linha de comando */

void recebe_argumentos(int argc, char **argv)
{
    if (argc < 5)
    {
        printf("Digite %s <número de consumidores> <tamanho do bloco> <local do arquivo de entrada> <local do arquivo de saída>.", argv[0]);
        exit(1);
    }

    num_consumidores = atoi(argv[1]);
    N = atoi(argv[2]);
    local_arquivo_entrada = argv[3];
    local_arquivo_saida = argv[4];

    if (num_consumidores <= 0 || N <= 0)
    {
        puts("Digite um número de consumidores maior que 0 e um tamanho de bloco maior que 1.");
        exit(2);
    }
}

/* Alocação de espaço para recursos */

void inicializa_recursos()
{
    for (int i = 0 ; i < NUM_BLOCOS_BUFFER ; i++)
    {
        buffer[i] = (int *) calloc(N, sizeof(int));

        if (!(buffer[i]))
        {
            printf("Erro ao alocar espaço para o buffer %d.\n", i);

            for (int j = 0 ; j < i ; j++)
                free(buffer[j]);

            exit(8);
        }
    }

    sem_init(&consumidores, 0, 1); // Apenas um consumidor pode ler o slot
    sem_init(&slot_vazio, 0, NUM_BLOCOS_BUFFER); // Produtor pode guardar números em todos os slots inicialmente
    sem_init(&slot_cheio, 0, 0); // Não há slot ocupado no começo. Consumidores irão aguardar
    sem_init(&arquivo_livre, 0, 1); // Seguindo o padrão leitor/escritor, apenas uma thread pode escrever no arquivo de saída
    pthread_mutex_init(&mutex, NULL); // Exclusão mútua para leitura da variável de verificação da linha órfã
}

/* Liberação de recursos alocados no começo do programa */

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
        printf("Erro ao criar o arquivo de saída.\n");
        fclose(arq_entrada);
        exit(4);
    }

    fscanf(arq_entrada, "%d", &qtd_numeros); // Lê a quantidade de números

    /*if (qtd_numeros % 10 != 0)
    {
        puts("A quantidade de números no arquivo deve ser um múltiplo de 10.");
        fclose(arq_entrada);
        fclose(arq_saida);
        libera_recursos();
        exit(9);
    }*/

    num_linhas = qtd_numeros / N;

    if (qtd_numeros < N)
    {
        puts("A quantidade de números no arquivo não pode ser menor que o tamanho do bloco solicitado.");
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

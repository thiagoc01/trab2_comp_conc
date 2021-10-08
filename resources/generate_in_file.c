/*
  * Programa: Algoritmo para gerar um arquivo de texto com valores numericos em ordem aleatoria que servirao de entrada para o programa principal
  * Alunos: Gabriele Jandres Cavalcanti e Thiago Figueiredo Lopes de Castro | DREs: 119159948 e 118090044
  * Disciplina: Computacao Concorrente - 2021.1
  * Modulo 2 - Trabalho 2 - Complemento
  * Data: Outubro de 2021
*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv)
{
	FILE *file;
	int result, i, quantity_of_numbers, number, count, N;

	srand(time(NULL));

	// Abertura do arquivo de entrada
	file = fopen("./in/in.txt", "wt");

	if (file == NULL)
	{
		printf("Erro na criacao do arquivo\n");
		return 2;
	}

	// Inicializacoes

	if (atoi(argv[1]) == -1)
		quantity_of_numbers = (rand() % 50) * 10;

	else
		quantity_of_numbers = atoi(argv[1]);

	N = atoi(argv[2]);
	
	count = 0;

	// Escreve a quantidade de valores na primeira linha do arquivo
	fprintf(file, "%d\n", quantity_of_numbers);

	// Escrita no arquivo
	for (i = 0; i < quantity_of_numbers; i++)
	{
		number = rand() % 50;
		count++;
		if (count == N)
		{
			result = i == quantity_of_numbers - 1 ? fprintf(file, "%d", number) : fprintf(file, "%d\n", number);
			count = 0;
		}
		else fprintf(file, "%d ", number);

		if (result == EOF)
			printf("Erro na Gravacao\n");
	}

	fclose(file);

	return 0;
}

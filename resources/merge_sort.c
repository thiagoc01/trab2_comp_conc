/*
  * Programa: Algoritmo merge sort para ordenacao de valores
  * Alunos: Gabriele Jandres Cavalcanti e Thiago Figueiredo Lopes de Castro | DREs: 119159948 e 118090044
  * Disciplina: Computacao Concorrente - 2021.1
  * Modulo 2 - Trabalho 2
  * Data: Setembro de 2021
*/

#include "merge_sort.h"
#include <stdlib.h>

/* Função auxiliar à ordenação Merge Sort para juntar dois "conjuntos" */
void merge(int *vet, int first, int middle, int last) {
	
	// Criamos dois vetores para o lado esquerdo e lado direito do vetor em relacao ao middle.
	int first_half = middle - first + 1;
	int second_half = last - middle;
	int first_vet[first_half];
	int second_vet[second_half];

	int i, j, aux = first; // Cria um indice para o vetor original

	for (i = 0; i < first_half; i++)
		first_vet[i] = vet[first + i];

	for (j = 0; j < second_half; j++)
		second_vet[j] = vet[middle + 1 + j];

	// Reinicia os contadores
	i = 0;
	j = 0;

	// Percorre ambos os vetores ao mesmo tempo e pega o menor de cada posicao relativa 
	while (i < first_half && j < second_half) {
		if (first_vet[i] <= second_vet[j]) {
			vet[aux] = first_vet[i];
			i++;
		} else {
			vet[aux] = second_vet[j];
			j++;
		}
		aux++;
	}

	// Realiza a mesma coisa, so que agora um dos dois chegou ao final
	while (i < first_half) {
		vet[aux] = first_vet[i];
		i++;
		aux++;
	}

	while (j < second_half) {
		vet[aux] = second_vet[j];
		j++;
		aux++;
	}
}

/* Algoritmo de ordenação Merge Sort */
void merge_sort(int *vetor, int first, int last) {
	int middle;

	if (first < last) {
		middle = (first + last) / 2;
		merge_sort(vetor, first, middle);
		merge_sort(vetor, middle + 1, last);
		merge(vetor, first, middle, last);
	}
}
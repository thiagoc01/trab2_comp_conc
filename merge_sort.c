#include "merge_sort.h"


void realiza_juncao(int *vetor, int comeco, int meio, int fim)
{
	/*
	Criamos dois vetores para o lado esquerdo e lado direito do vetor em relação ao meio.
	*/

	int quantidade_esquerda = meio - comeco + 1, quantidade_direita = fim - meio;

	int vetor_esquerdo[quantidade_esquerda], vetor_direito[quantidade_direita];

	int i, j, indice_auxiliar = comeco; // Cria um índice para o vetor original

	for (i = 0; i < quantidade_esquerda; i++)
    	vetor_esquerdo[i] = vetor[comeco + i];

  	for (j = 0; j <  quantidade_direita; j++)
    	vetor_direito[j] = vetor[meio + 1 + j];

    /* Reinicia os contadores */

    i = 0;
    j = 0;

    /* Percorre ambos os vetores ao mesmo tempo e pega o menor de cada posição relativa */

    while (i < quantidade_esquerda && j < quantidade_direita)
    {
    	if (vetor_esquerdo[i] <= vetor_direito[j])
    	{
    		vetor[indice_auxiliar] = vetor_esquerdo[i];
    		i++;
    	}
    	else
    	{
    		vetor[indice_auxiliar] = vetor_direito[j];
    		j++;
    	}
    	indice_auxiliar++;
    }

    /* Realiza a mesma coisa, só que agora um dos dois chegou ao final */

    while (i < quantidade_esquerda)
    {
    	vetor[indice_auxiliar] = vetor_esquerdo[i];
    	i++;
    	indice_auxiliar++;
  	}

  	while (j < quantidade_direita)
  	{
    	vetor[indice_auxiliar] = vetor_direito[j];
    	j++;
    	indice_auxiliar++;
  }

}


void merge_sort(int *vetor, int inicio, int fim)
{
	int meio;

	if (inicio < fim)
	{
		meio = (inicio + fim) / 2;

		merge_sort(vetor, inicio, meio);
		merge_sort(vetor, meio + 1, fim);
		realiza_juncao(vetor, inicio, meio, fim);
	}

}

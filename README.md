# Computação Concorrente | Trabalho 02 | 2021.1
*Solução concorrente para o problema de ordenar blocos de valores em ordem crescente dado um arquivo de entrada.*

## Tabela de Conteúdo

1. [Tecnologias utilizadas](#tecnologias-utilizadas)
2. [Estrutura do repositório](#estrutura-do-repositório)
3. [Como usar o programa?](#como-usar-o-programa)
4. [Autores](#autores)

## 🖥️ Tecnologias utilizadas
O projeto foi desenvolvido utilizando a linguagem C e para a compilação recomenda-se o uso do GCC (GNU Compiler Collection).

![Badge](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)

## 📂 Estrutura do repositório
O repositório é composto pelos subdiretórios:
* **in**: conjunto de arquivos de entrada
* **out**: conjunto de arquivos de saída
* **libs**: bibliotecas e macros externas necessárias para cálculo do desempenho
* **resources**: códigos auxiliares para o programa principal

## 🤔 Como usar o programa?

1.  Clone esse repositório
```
  git clone https://github.com/thiagoc01/trab2_comp_conc.git
```

2. Utilize o comando abaixo do makefile para gerar um arquivo de entrada, que estará em in/in.txt, e também compilar todos os arquivos necessários para rodar o programa. *QTY_NUMBERS* é a quantidade de valores que o arquivo deve ter e *BLOCK_SIZE* é o tamanho de cada bloco
```
  make QTY_NUMBERS=1000000000 BLOCK_SIZE=100
```

3. Execute o programa
```
  ./file_sort <numero-de-threads-consumidoras> <tamanho-do-bloco> <arquivo-de-entrada> <arquivo-de-saida>
```

## 👩‍💻 Autores
* Gabriele Jandres Cavalcanti
* Thiago Figueiredo Lopes de Castro

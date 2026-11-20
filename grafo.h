#ifndef GRAFO_H
#define GRAFO_H

#include <stdio.h>

#define MAX_NOME 64

// Tipos de vertice (usados no grafo bipartido do DF)
#define TIPO_UNIDADE   0   // Regiao Administrativa ou Estado (UF)
#define TIPO_CANDIDATO 1   // Candidato

// No da lista de adjacencias: representa uma aresta u -> destino
typedef struct No {
    int destino;
    float peso;
    struct No* prox;
} No;

// Lista encadeada de adjacencias de um vertice
typedef struct Lista {
    No* inicio;
    No* fim;
    int tamanho;
} Lista;

// Grafo com lista de adjacencias dinamica e vertices rotulados
typedef struct Grafo {
    int n_vertices;
    int capacidade;
    int orientado;      // 0 = nao-orientado, 1 = orientado
    char** rotulos;     // nome de cada vertice (string alocada)
    int* tipos;         // TIPO_UNIDADE ou TIPO_CANDIDATO
    Lista* adj;         // vetor de listas de adjacencia
} Grafo;

Grafo* criar_grafo(int orientado);
void   liberar_grafo(Grafo* g);                       // aceita NULL
int    adicionar_vertice(Grafo* g, const char* rotulo, int tipo); // retorna indice ou -1
int    buscar_vertice(const Grafo* g, const char* rotulo);        // indice ou -1
int    adicionar_aresta(Grafo* g, int u, int v, float peso);      //1 ok, 0 erro
int    existe_aresta(const Grafo* g, int u, int v);
float  peso_aresta(const Grafo* g, int u, int v);     //-1 se nao existe
int    numero_arestas(const Grafo* g);
void   imprimir_grafo(const Grafo* g);

// Utilitarios de CSV
void str_trim(char* s);
int  csv_ler_linha(FILE* f, char* buf, size_t tam);   // le linha sem \r\n e sem BOM; 0 no fim
int  csv_dividir(char* linha, char* campos[], int max);

#endif

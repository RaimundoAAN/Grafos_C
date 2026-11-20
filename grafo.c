#include "grafo.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>


// Criacao / destruicao

Grafo* criar_grafo(int orientado) {
    Grafo* g = (Grafo*)malloc(sizeof(Grafo));
    if (!g) return NULL;

    g->n_vertices = 0;
    g->capacidade = 8;
    g->orientado = orientado;
    g->rotulos = (char**)malloc(g->capacidade * sizeof(char*));
    g->tipos = (int*)malloc(g->capacidade * sizeof(int));
    g->adj = (Lista*)malloc(g->capacidade * sizeof(Lista));

    if (!g->rotulos || !g->tipos || !g->adj) {
        free(g->rotulos);
        free(g->tipos);
        free(g->adj);
        free(g);
        return NULL;
    }
    return g;
}

void liberar_grafo(Grafo* g) {
    if (!g) return;
    for (int i = 0; i < g->n_vertices; i++) {
        No* p = g->adj[i].inicio;
        while (p) {
            No* prox = p->prox;
            free(p);
            p = prox;
        }
        free(g->rotulos[i]);
    }
    free(g->rotulos);
    free(g->tipos);
    free(g->adj);
    free(g);
}

//Dobra a capacidade dos vetores do grafo
static int expandir(Grafo* g) {
    int nova = g->capacidade * 2;

    char** r = (char**)realloc(g->rotulos, nova * sizeof(char*));
    if (!r) return 0;
    g->rotulos = r;

    int* t = (int*)realloc(g->tipos, nova * sizeof(int));
    if (!t) return 0;
    g->tipos = t;

    Lista* a = (Lista*)realloc(g->adj, nova * sizeof(Lista));
    if (!a) return 0;
    g->adj = a;

    g->capacidade = nova;
    return 1;
}

// Vertices e arestas


int adicionar_vertice(Grafo* g, const char* rotulo, int tipo) {
    if (!g || !rotulo) return -1;
    if (g->n_vertices == g->capacidade && !expandir(g)) return -1;

    char* copia = (char*)malloc(strlen(rotulo) + 1);
    if (!copia) return -1;
    strcpy(copia, rotulo);

    int id = g->n_vertices++;
    g->rotulos[id] = copia;
    g->tipos[id] = tipo;
    g->adj[id].inicio = NULL;
    g->adj[id].fim = NULL;
    g->adj[id].tamanho = 0;
    return id;
}

int buscar_vertice(const Grafo* g, const char* rotulo) {
    if (!g || !rotulo) return -1;
    for (int i = 0; i < g->n_vertices; i++) {
        if (strcmp(g->rotulos[i], rotulo) == 0) return i;
    }
    return -1;
}

static int inserir_no(Lista* l, int destino, float peso) {
    No* n = (No*)malloc(sizeof(No));
    if (!n) return 0;
    n->destino = destino;
    n->peso = peso;
    n->prox = NULL;
    if (l->fim) l->fim->prox = n;
    else l->inicio = n;
    l->fim = n;
    l->tamanho++;
    return 1;
}

int adicionar_aresta(Grafo* g, int u, int v, float peso) {
    if (!g || u < 0 || v < 0 || u >= g->n_vertices || v >= g->n_vertices) return 0;
    if (!inserir_no(&g->adj[u], v, peso)) return 0;
    if (!g->orientado && u != v) {
        if (!inserir_no(&g->adj[v], u, peso)) return 0;
    }
    return 1;
}

int existe_aresta(const Grafo* g, int u, int v) {
    if (!g || u < 0 || v < 0 || u >= g->n_vertices || v >= g->n_vertices) return 0;
    for (No* p = g->adj[u].inicio; p; p = p->prox) {
        if (p->destino == v) return 1;
    }
    return 0;
}

float peso_aresta(const Grafo* g, int u, int v) {
    if (!g || u < 0 || v < 0 || u >= g->n_vertices || v >= g->n_vertices) return -1.0f;
    for (No* p = g->adj[u].inicio; p; p = p->prox) {
        if (p->destino == v) return p->peso;
    }
    return -1.0f;
}

int numero_arestas(const Grafo* g) {
    if (!g) return 0;
    int soma = 0;
    for (int i = 0; i < g->n_vertices; i++) soma += g->adj[i].tamanho;
    return g->orientado ? soma : soma / 2;
}

void imprimir_grafo(const Grafo* g) {
    if (!g) return;
    printf("Grafo %s: %d vertices, %d arestas\n",
           g->orientado ? "orientado" : "nao-orientado", g->n_vertices, numero_arestas(g));
    for (int i = 0; i < g->n_vertices; i++) {
        printf("  [%d] %s:", i, g->rotulos[i]);
        for (No* p = g->adj[i].inicio; p; p = p->prox) {
            printf(" -> %s(%.2f)", g->rotulos[p->destino], p->peso);
        }
        printf("\n");
    }
}

// Utilitarios de CSV

void str_trim(char* s) {
    char* ini = s;
    while (*ini && isspace((unsigned char)*ini)) ini++;
    size_t len = strlen(ini);
    while (len > 0 && isspace((unsigned char)ini[len - 1])) len--;
    if (len >= 2 && ini[0] == '"' && ini[len - 1] == '"') {
        ini++;
        len -= 2;
    }
    memmove(s, ini, len);
    s[len] = '\0';
}

int csv_ler_linha(FILE* f, char* buf, size_t tam) {
    if (!fgets(buf, (int)tam, f)) return 0;
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';        // remove BOM UTF-8, se existir
    if ((unsigned char)buf[0] == 0xEF && (unsigned char)buf[1] == 0xBB &&
        (unsigned char)buf[2] == 0xBF) {
        memmove(buf, buf + 3, strlen(buf + 3) + 1);
    }
    return 1;
}

// Divide a linha in-place por ',' se houver mais campos que 'max',
//  o ultimo campo recebe o restante da linha retorna o numero de campos
int csv_dividir(char* linha, char* campos[], int max) {
    int n = 0;
    char* p = linha;
    while (n < max) {
        campos[n++] = p;
        char* v = strchr(p, ',');
        if (!v) break;
        *v = '\0';
        p = v + 1;
    }
    for (int i = 0; i < n; i++) str_trim(campos[i]);
    return n;
}

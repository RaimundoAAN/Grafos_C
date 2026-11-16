#include "analise_nacional.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define TAM_LINHA  2048
#define MAX_UF     64
#define MAX_CAND   16

/* ------------------------------------------------------------------ */
/* Leitura do CSV presidencial                                         */
/* ------------------------------------------------------------------ */

typedef struct {
    int n_uf;
    int n_cand;
    char uf[MAX_UF][MAX_NOME];
    char cand[MAX_CAND][MAX_NOME];
    float pct[MAX_UF][MAX_CAND];
} DadosPres;

static void copiar_nome(char* dest, const char* orig) {
    strncpy(dest, orig, MAX_NOME - 1);
    dest[MAX_NOME - 1] = '\0';
}

/* Formato: UF,Pres_X,Pres_Y,...  (percentuais) */
static int carregar_votos(const char* arquivo, DadosPres* d) {
    FILE* f = fopen(arquivo, "r");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", arquivo);
        return 0;
    }

    char linha[TAM_LINHA];
    char* campos[MAX_CAND + 1];

    if (!csv_ler_linha(f, linha, sizeof(linha))) {
        fprintf(stderr, "Erro: arquivo '%s' vazio\n", arquivo);
        fclose(f);
        return 0;
    }
    int nc = csv_dividir(linha, campos, MAX_CAND + 1);
    if (nc < 2) {
        fprintf(stderr, "Erro: cabecalho invalido em '%s'\n", arquivo);
        fclose(f);
        return 0;
    }

    d->n_cand = nc - 1;
    d->n_uf = 0;
    for (int i = 0; i < d->n_cand; i++) copiar_nome(d->cand[i], campos[i + 1]);

    while (csv_ler_linha(f, linha, sizeof(linha))) {
        if (linha[0] == '\0') continue;
        int n = csv_dividir(linha, campos, MAX_CAND + 1);
        if (n < nc) {
            fprintf(stderr, "Aviso: linha ignorada em '%s' (campos insuficientes)\n", arquivo);
            continue;
        }
        if (d->n_uf >= MAX_UF) {
            fprintf(stderr, "Aviso: limite de %d UFs atingido\n", MAX_UF);
            break;
        }
        copiar_nome(d->uf[d->n_uf], campos[0]);
        for (int i = 0; i < d->n_cand; i++) d->pct[d->n_uf][i] = (float)atof(campos[i + 1]);
        d->n_uf++;
    }

    fclose(f);
    return d->n_uf > 0;
}

/* ------------------------------------------------------------------ */
/* Construcao dos grafos                                               */
/* ------------------------------------------------------------------ */

Grafo* construir_grafo_presidencial(const char* arquivo_votos) {
    DadosPres d;
    if (!carregar_votos(arquivo_votos, &d)) return NULL;

    Grafo* g = criar_grafo(0);
    if (!g) return NULL;

    for (int i = 0; i < d.n_uf; i++) {
        if (adicionar_vertice(g, d.uf[i], TIPO_UNIDADE) < 0) {
            liberar_grafo(g);
            return NULL;
        }
    }

    /* Grafo completo: peso = distancia euclidiana entre vetores de votacao */
    for (int i = 0; i < d.n_uf; i++) {
        for (int j = i + 1; j < d.n_uf; j++) {
            double soma = 0.0;
            for (int k = 0; k < d.n_cand; k++) {
                double diff = (double)d.pct[i][k] - d.pct[j][k];
                soma += diff * diff;
            }
            adicionar_aresta(g, i, j, (float)sqrt(soma));
        }
    }
    return g;
}

/* Formato: UF,Vizinhos   (vizinhos separados por ';') */
Grafo* construir_grafo_territorial(const char* arquivo_votos, const char* arquivo_fronteiras) {
    DadosPres d;
    if (!carregar_votos(arquivo_votos, &d)) return NULL;

    FILE* f = fopen(arquivo_fronteiras, "r");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", arquivo_fronteiras);
        return NULL;
    }

    Grafo* g = criar_grafo(0);
    if (!g) {
        fclose(f);
        return NULL;
    }
    for (int i = 0; i < d.n_uf; i++) {
        if (adicionar_vertice(g, d.uf[i], TIPO_UNIDADE) < 0) {
            liberar_grafo(g);
            fclose(f);
            return NULL;
        }
    }

    char linha[TAM_LINHA];
    char* campos[2];
    csv_ler_linha(f, linha, sizeof(linha)); /* descarta cabecalho */

    while (csv_ler_linha(f, linha, sizeof(linha))) {
        if (linha[0] == '\0') continue;
        if (csv_dividir(linha, campos, 2) < 2) continue;

        int u = buscar_vertice(g, campos[0]);
        if (u < 0) continue; /* UF fora do conjunto de dados */

        for (char* tok = strtok(campos[1], ";, "); tok; tok = strtok(NULL, ";, ")) {
            int v = buscar_vertice(g, tok);
            if (v >= 0 && v != u && !existe_aresta(g, u, v)) {
                adicionar_aresta(g, u, v, 1.0f);
            }
        }
    }

    fclose(f);
    return g;
}

/* ------------------------------------------------------------------ */
/* BFS: propagacao por ondas                                           */
/* ------------------------------------------------------------------ */

void busca_largura_propagacao_fronteiras(Grafo* g_terr, int estado_origem) {
    if (!g_terr || estado_origem < 0 || estado_origem >= g_terr->n_vertices) {
        fprintf(stderr, "BFS: estado de origem invalido\n");
        return;
    }

    int n = g_terr->n_vertices;
    int* dist = (int*)malloc(n * sizeof(int));
    int* fila = (int*)malloc(n * sizeof(int));
    if (!dist || !fila) {
        free(dist);
        free(fila);
        return;
    }
    for (int i = 0; i < n; i++) dist[i] = -1;

    int ini = 0, fim = 0, max_dist = 0;
    dist[estado_origem] = 0;
    fila[fim++] = estado_origem;

    while (ini < fim) {
        int u = fila[ini++];
        for (No* p = g_terr->adj[u].inicio; p; p = p->prox) {
            int v = p->destino;
            if (dist[v] < 0) {
                dist[v] = dist[u] + 1;
                if (dist[v] > max_dist) max_dist = dist[v];
                fila[fim++] = v;
            }
        }
    }

    printf("\n=== BFS: propagacao a partir de %s (ondas de fronteira) ===\n",
           g_terr->rotulos[estado_origem]);
    for (int d = 0; d <= max_dist; d++) {
        printf("  Onda %d:", d);
        for (int v = 0; v < n; v++) {
            if (dist[v] == d) printf(" %s", g_terr->rotulos[v]);
        }
        printf("\n");
    }
    int inalcancaveis = 0;
    for (int v = 0; v < n; v++) {
        if (dist[v] < 0) {
            if (!inalcancaveis) printf("  Inalcancaveis:");
            printf(" %s", g_terr->rotulos[v]);
            inalcancaveis++;
        }
    }
    if (inalcancaveis) printf("\n");

    free(dist);
    free(fila);
}

/* ------------------------------------------------------------------ */
/* DFS: componentes conexas do grafo de similaridade filtrado          */
/* ------------------------------------------------------------------ */

static void dfs_visita(const Grafo* g, int u, float limiar, int* comp, int id) {
    comp[u] = id;
    for (No* p = g->adj[u].inicio; p; p = p->prox) {
        if (p->peso <= limiar && comp[p->destino] < 0) {
            dfs_visita(g, p->destino, limiar, comp, id);
        }
    }
}

void busca_profundidade_clusters(Grafo* g_pres, float limiar_corte) {
    if (!g_pres) return;

    int n = g_pres->n_vertices;
    int* comp = (int*)malloc(n * sizeof(int));
    if (!comp) return;
    for (int i = 0; i < n; i++) comp[i] = -1;

    int n_comp = 0;
    for (int v = 0; v < n; v++) {
        if (comp[v] < 0) dfs_visita(g_pres, v, limiar_corte, comp, n_comp++);
    }

    printf("\n=== DFS: blocos de votacao alinhada (divergencia <= %.2f p.p.) ===\n", limiar_corte);
    printf("  %d componente(s) conexa(s)\n", n_comp);
    for (int c = 0; c < n_comp; c++) {
        printf("  Bloco %d:", c + 1);
        for (int v = 0; v < n; v++) {
            if (comp[v] == c) printf(" %s", g_pres->rotulos[v]);
        }
        printf("\n");
    }

    free(comp);
}

/* ------------------------------------------------------------------ */
/* Kruskal: Arvore Geradora Minima                                     */
/* ------------------------------------------------------------------ */

typedef struct {
    int u, v;
    float peso;
} Aresta;

static int cmp_aresta(const void* a, const void* b) {
    const Aresta* x = (const Aresta*)a;
    const Aresta* y = (const Aresta*)b;
    if (x->peso < y->peso) return -1;
    if (x->peso > y->peso) return 1;
    if (x->u != y->u) return x->u - y->u;
    return x->v - y->v;
}

static int uf_find(int* pai, int x) {
    while (pai[x] != x) {
        pai[x] = pai[pai[x]]; /* compressao de caminho (path halving) */
        x = pai[x];
    }
    return x;
}

void calcular_mst_eleitoral_kruskal(Grafo* g_pres) {
    if (!g_pres || g_pres->n_vertices == 0) return;

    int n = g_pres->n_vertices;

    /* Coleta cada aresta nao-orientada uma unica vez (u < v) */
    int m = 0;
    for (int u = 0; u < n; u++)
        for (No* p = g_pres->adj[u].inicio; p; p = p->prox)
            if (u < p->destino) m++;

    Aresta* arestas = (Aresta*)malloc((m > 0 ? m : 1) * sizeof(Aresta));
    int* pai = (int*)malloc(n * sizeof(int));
    if (!arestas || !pai) {
        free(arestas);
        free(pai);
        return;
    }

    int k = 0;
    for (int u = 0; u < n; u++)
        for (No* p = g_pres->adj[u].inicio; p; p = p->prox)
            if (u < p->destino) {
                arestas[k].u = u;
                arestas[k].v = p->destino;
                arestas[k].peso = p->peso;
                k++;
            }

    qsort(arestas, m, sizeof(Aresta), cmp_aresta);
    for (int i = 0; i < n; i++) pai[i] = i;

    printf("\n=== Kruskal: Arvore Geradora Minima da afinidade eleitoral ===\n");
    int escolhidas = 0;
    double total = 0.0;
    for (int i = 0; i < m && escolhidas < n - 1; i++) {
        int ru = uf_find(pai, arestas[i].u);
        int rv = uf_find(pai, arestas[i].v);
        if (ru != rv) {
            pai[ru] = rv;
            escolhidas++;
            total += arestas[i].peso;
            printf("  %2d) %-4s -- %-4s  divergencia = %.2f\n", escolhidas,
                   g_pres->rotulos[arestas[i].u], g_pres->rotulos[arestas[i].v], arestas[i].peso);
        }
    }
    printf("  Peso total da AGM: %.2f\n", total);
    if (escolhidas < n - 1) {
        printf("  Aviso: grafo desconexo, resultado e uma floresta geradora minima.\n");
    }

    free(arestas);
    free(pai);
}

#include "analise_df.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define TAM_LINHA   2048
#define MAX_CAMPOS  64
#define EPS         1e-4f

// Medida usada na projecao. Troque por similaridade_jaccard se preferir
#define MEDIDA_SIMILARIDADE similaridade_cosseno

// Similaridades

float similaridade_cosseno(const float* a, const float* b, int n) {
    double dot = 0.0, na = 0.0, nb = 0.0;
    for (int i = 0; i < n; i++) {
        dot += (double)a[i] * b[i];
        na += (double)a[i] * a[i];
        nb += (double)b[i] * b[i];
    }
    if (na == 0.0 || nb == 0.0) return 0.0f;
    return (float)(dot / (sqrt(na) * sqrt(nb)));
}

// Jaccard ponderado: soma(min) / soma(max). Distancia de Jaccard = 1 - valor
float similaridade_jaccard(const float* a, const float* b, int n) {
    double mn = 0.0, mx = 0.0;
    for (int i = 0; i < n; i++) {
        mn += (a[i] < b[i]) ? a[i] : b[i];
        mx += (a[i] > b[i]) ? a[i] : b[i];
    }
    if (mx == 0.0) return 0.0f;
    return (float)(mn / mx);
}

// 1) Grafo bipartido

Grafo* construir_grafo_bipartido_df(const char* arquivo_csv) {
    FILE* f = fopen(arquivo_csv, "r");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", arquivo_csv);
        return NULL;
    }

    char linha[TAM_LINHA];
    char* campos[MAX_CAMPOS];

    // Cabecalho: RA,Cand_A,Cand_B,...
    if (!csv_ler_linha(f, linha, sizeof(linha))) {
        fprintf(stderr, "Erro: arquivo '%s' vazio\n", arquivo_csv);
        fclose(f);
        return NULL;
    }
    int nc = csv_dividir(linha, campos, MAX_CAMPOS);
    if (nc < 2) {
        fprintf(stderr, "Erro: cabecalho invalido em '%s'\n", arquivo_csv);
        fclose(f);
        return NULL;
    }

    Grafo* g = criar_grafo(0);
    if (!g) {
        fclose(f);
        return NULL;
    }

    int n_cand = nc - 1;
    int idx_cand[MAX_CAMPOS];
    for (int i = 0; i < n_cand; i++) {
        idx_cand[i] = adicionar_vertice(g, campos[i + 1], TIPO_CANDIDATO);
        if (idx_cand[i] < 0) {
            liberar_grafo(g);
            fclose(f);
            return NULL;
        }
    }

    int num_linha = 1;
    while (csv_ler_linha(f, linha, sizeof(linha))) {
        num_linha++;
        if (linha[0] == '\0') continue;

        int n = csv_dividir(linha, campos, MAX_CAMPOS);
        if (n < nc) {
            fprintf(stderr, "Aviso: linha %d ignorada (campos insuficientes)\n", num_linha);
            continue;
        }
        if (buscar_vertice(g, campos[0]) >= 0) {
            fprintf(stderr, "Aviso: RA duplicada '%s' na linha %d ignorada\n", campos[0], num_linha);
            continue;
        }

        int ra = adicionar_vertice(g, campos[0], TIPO_UNIDADE);
        if (ra < 0) {
            liberar_grafo(g);
            fclose(f);
            return NULL;
        }
        for (int i = 0; i < n_cand; i++) {
            adicionar_aresta(g, ra, idx_cand[i], (float)atof(campos[i + 1]));
        }
    }

    fclose(f);
    return g;
}

// 2) Candidato dominante por RA

void candidatos_dominantes_por_ra(Grafo* g) {
    if (!g) return;
    printf("\n=== Candidato dominante por RA ===\n");

    for (int v = 0; v < g->n_vertices; v++) {
        if (g->tipos[v] != TIPO_UNIDADE) continue;

        float max = -1.0f;
        for (No* p = g->adj[v].inicio; p; p = p->prox) {
            if (p->peso > max) max = p->peso;
        }
        if (max < 0.0f) {
            printf("  %-20s -> sem dados\n", g->rotulos[v]);
            continue;
        }

        printf("  %-20s -> ", g->rotulos[v]);
        int cont = 0;
        for (No* p = g->adj[v].inicio; p; p = p->prox) {
            if (p->peso >= max - EPS) {
                printf("%s%s", cont ? " / " : "", g->rotulos[p->destino]);
                cont++;
            }
        }
        printf(" (%.1f%%)%s\n", max, cont > 1 ? " [EMPATE]" : "");
    }
}

// 3) Projecao monopartida: grafo de similaridade entre RAs

static void preencher_vetor(const Grafo* g, int v, float* vetor) {
    for (int i = 0; i < g->n_vertices; i++) vetor[i] = 0.0f;
    for (No* p = g->adj[v].inicio; p; p = p->prox) vetor[p->destino] = p->peso;
}

Grafo* gerar_grafo_similaridade_ra(Grafo* g_bipartido, float limiar_similaridade) {
    if (!g_bipartido) return NULL;

    int n = g_bipartido->n_vertices;
    Grafo* gs = criar_grafo(0);
    int* mapa = (int*)malloc(n * sizeof(int));
    float* vi = (float*)malloc(n * sizeof(float));
    float* vj = (float*)malloc(n * sizeof(float));
    if (!gs || !mapa || !vi || !vj) {
        liberar_grafo(gs);
        free(mapa);
        free(vi);
        free(vj);
        return NULL;
    }

    // Vertices do novo grafo: apenas as RAs
    for (int v = 0; v < n; v++) {
        mapa[v] = -1;
        if (g_bipartido->tipos[v] == TIPO_UNIDADE) {
            mapa[v] = adicionar_vertice(gs, g_bipartido->rotulos[v], TIPO_UNIDADE);
        }
    }

    // Arestas: pares de RAs com similaridade >= limiar
    for (int i = 0; i < n; i++) {
        if (mapa[i] < 0) continue;
        preencher_vetor(g_bipartido, i, vi);
        for (int j = i + 1; j < n; j++) {
            if (mapa[j] < 0) continue;
            preencher_vetor(g_bipartido, j, vj);
            float s = MEDIDA_SIMILARIDADE(vi, vj, n);
            if (s >= limiar_similaridade) {
                adicionar_aresta(gs, mapa[i], mapa[j], s);
            }
        }
    }

    free(mapa);
    free(vi);
    free(vj);
    return gs;
}

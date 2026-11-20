#include <stdio.h>
#include <stdlib.h>

#include "analise_df.h"
#include "analise_nacional.h"
#include "grafo.h"

// Uso: ./eleicoes [pasta_dados] [limiar_df] [limiar_pres] [UF_origem]
// Padroes: dados  0.98  10.0  DF
int main(int argc, char** argv) {
    const char* dir = (argc > 1) ? argv[1] : "dados";
    float limiar_df = (argc > 2) ? (float)atof(argv[2]) : 0.98f;
    float limiar_pres = (argc > 3) ? (float)atof(argv[3]) : 10.0f;
    const char* origem = (argc > 4) ? argv[4] : "DF";

    char csv_df[512], csv_pres[512], csv_front[512];
    snprintf(csv_df, sizeof(csv_df), "%s/votacao_governador_df_2026.csv", dir);
    snprintf(csv_pres, sizeof(csv_pres), "%s/votacao_presidente_estados_2026.csv", dir);
    snprintf(csv_front, sizeof(csv_front), "%s/fronteiras_estados.csv", dir);

    int erro = 0;

    // Eixo 1: Governador do DF
    printf("##### EIXO 1: GOVERNADOR DO DF #####\n\n");
    Grafo* g_bip = construir_grafo_bipartido_df(csv_df);
    if (g_bip) {
        printf("Grafo bipartido (RAs x Candidatos):\n");
        imprimir_grafo(g_bip);

        candidatos_dominantes_por_ra(g_bip);

        Grafo* g_sim = gerar_grafo_similaridade_ra(g_bip, limiar_df);
        if (g_sim) {
            printf("\n=== Grafo de similaridade entre RAs (cosseno >= %.2f) ===\n", limiar_df);
            imprimir_grafo(g_sim);
            liberar_grafo(g_sim);
        }
        liberar_grafo(g_bip);
    } else {
        erro = 1;
    }

    // Eixo 2: Presidente (UFs)
    printf("\n##### EIXO 2: PRESIDENTE DA REPUBLICA #####\n");
    Grafo* g_terr = construir_grafo_territorial(csv_pres, csv_front);
    Grafo* g_pres = construir_grafo_presidencial(csv_pres);

    if (g_terr && g_pres) {
        printf("\nG_Terr: %d UFs, %d fronteiras | G_Pres: %d UFs, %d arestas\n",
               g_terr->n_vertices, numero_arestas(g_terr),
               g_pres->n_vertices, numero_arestas(g_pres));

        int o = buscar_vertice(g_terr, origem);
        if (o < 0) {
            fprintf(stderr, "UF de origem '%s' nao encontrada nos dados\n", origem);
            erro = 1;
        } else {
            busca_largura_propagacao_fronteiras(g_terr, o);
        }
        busca_profundidade_clusters(g_pres, limiar_pres);
        calcular_mst_eleitoral_kruskal(g_pres);
    } else {
        erro = 1;
    }
    liberar_grafo(g_terr);
    liberar_grafo(g_pres);

    return erro;
}

#ifndef ANALISE_NACIONAL_H
#define ANALISE_NACIONAL_H

#include "grafo.h"

/* Grafo de contiguidade territorial G_Terr: vertices = UFs (na ordem do CSV de votos),
   aresta (peso 1) entre UFs que compartilham fronteira. */
Grafo* construir_grafo_territorial(const char* arquivo_votos, const char* arquivo_fronteiras);

/* Grafo de similaridade presidencial G_Pres: grafo COMPLETO ponderado entre as UFs.
   Peso = divergencia de votos (distancia euclidiana, em pontos percentuais). */
Grafo* construir_grafo_presidencial(const char* arquivo_votos);

/* BFS no grafo de fronteiras: imprime a propagacao em ondas a partir da UF de origem. */
void busca_largura_propagacao_fronteiras(Grafo* g_terr, int estado_origem);

/* DFS no grafo de similaridade filtrado (arestas com divergencia <= limiar_corte):
   imprime as componentes conexas (blocos regionais de votacao alinhada). */
void busca_profundidade_clusters(Grafo* g_pres, float limiar_corte);

/* Kruskal: Arvore Geradora Minima (menor divergencia total) do grafo presidencial. */
void calcular_mst_eleitoral_kruskal(Grafo* g_pres);

#endif

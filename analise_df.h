#ifndef ANALISE_DF_H
#define ANALISE_DF_H

#include "grafo.h"

// Le o CSV pre-processado (RA, Cand_A, Cand_B, ...) e constroi o grafo
// bipartido nao-orientado ponderado: RA <-> Candidato, peso = % de votos
Grafo* construir_grafo_bipartido_df(const char* arquivo_csv);

// Imprime o(s) candidato(s) vencedor(es) em cada RA
void candidatos_dominantes_por_ra(Grafo* g);

// Projeta o grafo bipartido em um grafo de similaridade entre RAs
// Cria aresta entre duas RAs quando similaridade >= limiar_similaridade
Grafo* gerar_grafo_similaridade_ra(Grafo* g_bipartido, float limiar_similaridade);

// Medidas de similaridade entre vetores de votacao (0 a 1)
float similaridade_cosseno(const float* a, const float* b, int n);
float similaridade_jaccard(const float* a, const float* b, int n); // Jaccard ponderado

#endif

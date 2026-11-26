# Análise das Eleições 2026 via Teoria dos Grafos

Projeto em C que modela os resultados eleitorais de 2026 como grafos e aplica algoritmos clássicos (BFS, DFS e Kruskal) para encontrar padrões de votação. A análise tem dois eixos:

1. **Governador do DF**: grafo bipartido entre Regiões Administrativas (RAs) e candidatos, e a projeção dele em um grafo de similaridade entre RAs.
2. **Presidente da República**: grafos das 27 UFs, um de fronteiras e outro de divergência de votos.

## Estrutura do projeto

```
.
├── grafo.h / grafo.c                  # TAD Grafo (lista de adjacências) + utilitários de CSV
├── analise_df.h / analise_df.c        # Eixo 1: grafo bipartido, dominância e similaridade
├── analise_nacional.h / analise_nacional.c  # Eixo 2: BFS, DFS e Kruskal
├── main.c                             # Executa as duas análises
├── Makefile
└── dados/
    ├── votacao_governador_df_2026.csv
    ├── votacao_presidente_estados_2026.csv
    └── fronteiras_estados.csv
```

## O que cada módulo faz

### `grafo` (estrutura base)
Grafo com lista de adjacências dinâmica, vértices rotulados (nome da RA, UF ou candidato) e arestas ponderadas (`float`). Pode ser orientado ou não. Também traz funções de leitura de CSV (tratamento de BOM, espaços e aspas).

### `analise_df` (Governador do DF)
| Função | Descrição |
|---|---|
| `construir_grafo_bipartido_df` | Lê o CSV e liga cada RA a cada candidato; o peso da aresta é o % de votos. |
| `candidatos_dominantes_por_ra` | Mostra o(s) candidato(s) mais votado(s) em cada RA, sinalizando empates. |
| `gerar_grafo_similaridade_ra` | Projeta o grafo bipartido em um grafo só de RAs. Cria aresta entre duas RAs quando a similaridade de votação é maior ou igual ao limiar. |

A similaridade padrão é a do **cosseno**. A de **Jaccard ponderado** também está implementada; para trocar, altere o `#define MEDIDA_SIMILARIDADE` em `analise_df.c`.

### `analise_nacional` (Presidente)
| Função | Descrição |
|---|---|
| `construir_grafo_territorial` | `G_Terr`: UFs ligadas quando fazem fronteira (peso 1). |
| `construir_grafo_presidencial` | `G_Pres`: grafo completo entre UFs; o peso é a **divergência de votos** (distância euclidiana, em pontos percentuais). |
| `busca_largura_propagacao_fronteiras` | **BFS** em `G_Terr`: mostra a propagação em ondas a partir de uma UF de origem. |
| `busca_profundidade_clusters` | **DFS** em `G_Pres` (só arestas com divergência menor ou igual ao limiar): lista os blocos de votação alinhada. |
| `calcular_mst_eleitoral_kruskal` | **Kruskal**: árvore geradora mínima de `G_Pres`, com Union-Find. |

## Como compilar e executar

Requisitos: `gcc` (C11) e, opcionalmente, `make`.

### Linux / macOS / WSL

```bash
make
./eleicoes dados
```

### Windows (MSYS2/MinGW, PowerShell)

```powershell
gcc -std=c11 -Wall -Wextra -O2 -o eleicoes main.c grafo.c analise_df.c analise_nacional.c -lm
.\eleicoes.exe dados
```

Se os acentos aparecerem errados no terminal, rode `chcp 65001` antes.

### Parâmetros

```
./eleicoes [pasta_dados] [limiar_df] [limiar_pres] [UF_origem]
```

| Parâmetro | Padrão | Significado |
|---|---|---|
| `pasta_dados` | `dados` | Pasta com os três CSVs. |
| `limiar_df` | `0.98` | Similaridade mínima (0 a 1) para ligar duas RAs. |
| `limiar_pres` | `10.0` | Divergência máxima (p.p.) para a DFS agrupar duas UFs. |
| `UF_origem` | `DF` | UF de partida da BFS (precisa existir no CSV de votos). |

Exemplo:

```bash
./eleicoes dados 0.95 12.0 GO
```

## Formato dos dados

Os CSVs usam vírgula como separador e a primeira linha é o cabeçalho.

**`votacao_governador_df_2026.csv`**: uma linha por RA, uma coluna por candidato, com o percentual de votos.
```csv
RA,Cand_A,Cand_B,Cand_C
Plano Piloto,45,35,20
Taguatinga,30,50,20
```

**`votacao_presidente_estados_2026.csv`**: uma linha por UF, uma coluna por candidato, com o percentual de votos.
```csv
UF,Pres_X,Pres_Y
DF,48,52
GO,55,45
```

**`fronteiras_estados.csv`**: cada UF com seus vizinhos separados por `;`. Os vizinhos que não estiverem no CSV de votos são ignorados.
```csv
UF,Vizinhos
DF,GO;MG
```

Os arquivos em `dados/` são um **exemplo sintético** para testes. Para a análise real, substitua os dois CSVs de votação pelos dados agregados do TSE, mantendo o formato acima. O arquivo de fronteiras já cobre as 27 UFs.

Limites do código: até 64 colunas no CSV do DF e até 64 UFs e 16 candidatos no CSV presidencial.

## Exemplo de saída (dados sintéticos)

```
=== Candidato dominante por RA ===
  Plano Piloto         -> Cand_A (45.0%)
  Taguatinga           -> Cand_B (50.0%)
  Sobradinho           -> Cand_A / Cand_B (40.0%) [EMPATE]

=== BFS: propagacao a partir de DF (ondas de fronteira) ===
  Onda 0: DF
  Onda 1: GO MG
  Onda 2: SP BA

=== DFS: blocos de votacao alinhada (divergencia <= 10.00 p.p.) ===
  Bloco 1: DF GO MG SP
  Bloco 2: BA

=== Kruskal: Arvore Geradora Minima da afinidade eleitoral ===
   1) DF   -- MG    divergencia = 1.41
   ...
  Peso total da AGM: 29.70
```

## Complexidade

| Algoritmo | Complexidade |
|---|---|
| BFS | O(V + E) |
| DFS | O(V + E) |
| Kruskal | O(E log E) |
| Projeção de similaridade das RAs | O(V² · V), por causa do vetor de votos de cada par |

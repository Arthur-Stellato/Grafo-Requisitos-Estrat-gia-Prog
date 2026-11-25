#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_V    50
#define MAX_NOME 50

/* --- Estruturas --- */

typedef struct Vertice {
    int destino;
    struct Vertice *prox;
} Vertice;

typedef struct {
    int      n;
    char     nome[MAX_V][MAX_NOME];
    Vertice *adj[MAX_V];
    Vertice *radj[MAX_V];
} Grafo;

/* Resultado de uma busca (usado pela DFS e pela BFS) */
typedef struct {
    int origem;
    int objetivo;                 /* -1 = sem objetivo (varredura completa) */
    int ordem[MAX_V];
    int qtd_visitados;
    int arestas_percorridas;      /* arestas examinadas a partir de vertices visitados */
    int arestas_arvore;           /* arestas que descobriram um vertice novo */
    int achou;
    int caminho[MAX_V];           /* caminho origem -> objetivo (se achou) */
    int tam_caminho;
} Busca;

/* --- Grafo --- */

static void inserir_no_fim(Vertice **lista, int destino)
{
    Vertice *novo = (Vertice *)malloc(sizeof(Vertice));
    if (!novo) { fprintf(stderr, "Sem memoria.\n"); exit(1); }
    novo->destino = destino;
    novo->prox = NULL;

    if (*lista == NULL) { *lista = novo; return; }

    Vertice *p = *lista;
    while (p->prox) p = p->prox;
    p->prox = novo;
}

static void liberar_lista(Vertice *p)
{
    while (p) { Vertice *t = p; p = p->prox; free(t); }
}

static void grafo_init(Grafo *g)
{
    g->n = 0;
    for (int i = 0; i < MAX_V; i++) { g->adj[i] = NULL; g->radj[i] = NULL; }
}

static void grafo_liberar(Grafo *g)
{
    for (int i = 0; i < g->n; i++) {
        liberar_lista(g->adj[i]);
        liberar_lista(g->radj[i]);
    }
}

static int grafo_add_vertice(Grafo *g, const char *nome)
{
    if (g->n >= MAX_V) return -1;
    strncpy(g->nome[g->n], nome, MAX_NOME - 1);
    g->nome[g->n][MAX_NOME - 1] = '\0';
    return g->n++;
}

static int existe_aresta(const Grafo *g, int u, int v)
{
    for (Vertice *p = g->adj[u]; p; p = p->prox)
        if (p->destino == v) return 1;
    return 0;
}

/* v alcanca alvo seguindo as arestas? (usado para evitar ciclos) */
static int alcanca(const Grafo *g, int v, int alvo, int visitado[])
{
    if (v == alvo) return 1;
    visitado[v] = 1;
    for (Vertice *p = g->adj[v]; p; p = p->prox)
        if (!visitado[p->destino] && alcanca(g, p->destino, alvo, visitado))
            return 1;
    return 0;
}

/* u -> v  ("u e pre-requisito de v").
 * Retorna 0 ok, -1 invalido, -2 duplicada, -3 criaria ciclo. */
static int grafo_add_aresta(Grafo *g, int u, int v)
{
    if (u < 0 || u >= g->n || v < 0 || v >= g->n || u == v) return -1;
    if (existe_aresta(g, u, v)) return -2;

    int visitado[MAX_V] = {0};
    if (alcanca(g, v, u, visitado)) return -3;   /* v ja chega em u: u->v fecharia ciclo */

    inserir_no_fim(&g->adj[u], v);
    inserir_no_fim(&g->radj[v], u);
    return 0;
}

static void carregar_exemplo(Grafo *g)
{
    const char *disc[] = {
        "Logica de Programacao",          /* 0  */
        "Algoritmos",                     /* 1  */
        "Estrutura de Dados 1",           /* 2  */
        "Estrutura de Dados 2",           /* 3  */
        "Programacao Orientada a Objetos",/* 4  */
        "Banco de Dados",                 /* 5  */
        "Engenharia de Software",         /* 6  */
        "Desenvolvimento Web Backend",    /* 7  */
        "Matematica Discreta",            /* 8  */
        "Teoria dos Grafos",              /* 9  */
        "Sistemas Operacionais",          /* 10 */
        "Redes de Computadores",          /* 11 */
        "Infraestrutura e Seguranca",     /* 12 */
        "Projeto Integrador"              /* 13 */
    };
    int qtd = (int)(sizeof(disc) / sizeof(disc[0]));
    for (int i = 0; i < qtd; i++) grafo_add_vertice(g, disc[i]);

    /* pre-requisito -> disciplina */
    int arestas[][2] = {
        {0, 1},  {1, 2},  {2, 3},  {1, 4},  {1, 5},
        {4, 6},  {4, 7},  {5, 7},  {8, 9},  {2, 9},
        {1, 10}, {10, 11},{11, 12},{10, 12},{6, 13},
        {7, 13}, {12, 13}
    };
    int qa = (int)(sizeof(arestas) / sizeof(arestas[0]));
    for (int i = 0; i < qa; i++) grafo_add_aresta(g, arestas[i][0], arestas[i][1]);
}

/* --- Impressao auxiliar --- */

static void listar_disciplinas(const Grafo *g)
{
    printf("\n=== DISCIPLINAS E RELACOES ===\n");
    for (int v = 0; v < g->n; v++) {
        printf("[%2d] %s\n", v, g->nome[v]);

        printf("       pre-requisitos diretos: ");
        if (!g->radj[v]) printf("(nenhum)");
        for (Vertice *p = g->radj[v]; p; p = p->prox)
            printf("%s%s", g->nome[p->destino], p->prox ? ", " : "");

        printf("\n       libera:                 ");
        if (!g->adj[v]) printf("(nenhuma)");
        for (Vertice *p = g->adj[v]; p; p = p->prox)
            printf("%s%s", g->nome[p->destino], p->prox ? ", " : "");
        printf("\n");
    }
}

static void imprimir_sequencia(const Grafo *g, const int seq[], int tam)
{
    for (int i = 0; i < tam; i++) {
        printf("%s", g->nome[seq[i]]);
        if (i < tam - 1) printf(" -> ");
    }
    if (tam == 0) printf("(vazio)");
    printf("\n");
}

static void relatorio(const Grafo *g, const char *algoritmo, const Busca *b)
{
    printf("\n--- DADOS GERAIS DA BUSCA (%s) ---\n", algoritmo);
    printf("Vertice de origem        : [%d] %s\n", b->origem, g->nome[b->origem]);

    if (b->objetivo >= 0)
        printf("Objetivo                 : [%d] %s\n", b->objetivo, g->nome[b->objetivo]);
    else
        printf("Objetivo                 : (nenhum - varredura completa)\n");

    printf("Quantidade de visitados  : %d\n", b->qtd_visitados);
    printf("Vertices visitados       : ");
    for (int i = 0; i < b->qtd_visitados; i++)
        printf("[%d]%s", b->ordem[i], i < b->qtd_visitados - 1 ? ", " : "");
    printf("\n");

    printf("Arestas percorridas      : %d (examinadas)\n", b->arestas_percorridas);
    printf("Arestas de descoberta    : %d (levaram a um vertice novo)\n", b->arestas_arvore);

    printf("Caminho percorrido (ordem de visita):\n  ");
    imprimir_sequencia(g, b->ordem, b->qtd_visitados);

    if (b->objetivo >= 0) {
        if (b->achou) {
            printf("Objetivo encontrado      : SIM\n");
            printf("Caminho origem -> objetivo:\n  ");
            imprimir_sequencia(g, b->caminho, b->tam_caminho);
        } else {
            printf("Objetivo encontrado      : NAO (nao e alcancavel a partir da origem)\n");
        }
    }
}

/* --- DFS - recursiva com backtracking --- */

static int dfs_rec(const Grafo *g, Vertice *const *lista, Busca *b, int v,
                   int visitado[], int atual[], int *topo, int nivel)
{
    visitado[v] = 1;
    b->ordem[b->qtd_visitados++] = v;
    atual[(*topo)++] = v;                 /* entra no caminho atual */

    printf("  %*s-> visita [%d] %s\n", nivel * 2, "", v, g->nome[v]);

    if (v == b->objetivo) return 1;       /* achou: nao desfaz o caminho */

    for (Vertice *p = lista[v]; p; p = p->prox) {
        b->arestas_percorridas++;
        if (!visitado[p->destino]) {
            b->arestas_arvore++;
            if (dfs_rec(g, lista, b, p->destino, visitado, atual, topo, nivel + 1))
                return 1;                 /* propaga o "achei" pela pilha de chamadas */
        }
    }

    /* BACKTRACKING: v nao levou ao objetivo, sai do caminho atual */
    (*topo)--;
    printf("  %*s<- backtrack de [%d] %s\n", nivel * 2, "", v, g->nome[v]);
    return 0;
}

static void executar_dfs(const Grafo *g, Vertice *const *lista, int origem, int objetivo,
                         const char *titulo)
{
    Busca b;
    memset(&b, 0, sizeof(b));
    b.origem = origem;
    b.objetivo = objetivo;

    int visitado[MAX_V] = {0};
    int atual[MAX_V];
    int topo = 0;

    printf("\n=== %s ===\nExecucao passo a passo:\n", titulo);
    b.achou = dfs_rec(g, lista, &b, origem, visitado, atual, &topo, 0);

    if (b.achou) {
        b.tam_caminho = topo;
        memcpy(b.caminho, atual, topo * sizeof(int));
    }
    relatorio(g, "DFS", &b);
}

/* --- BFS - iterativa com fila --- */

static void executar_bfs(const Grafo *g, Vertice *const *lista, int origem, int objetivo)
{
    Busca b;
    memset(&b, 0, sizeof(b));
    b.origem = origem;
    b.objetivo = objetivo;

    int visitado[MAX_V] = {0};
    int pai[MAX_V];
    int fila[MAX_V];
    int ini = 0, fim = 0;

    for (int i = 0; i < MAX_V; i++) pai[i] = -1;

    printf("\n=== BUSCA EM LARGURA (BFS) ===\nExecucao passo a passo:\n");

    visitado[origem] = 1;
    fila[fim++] = origem;                         /* enfileira */

    while (ini < fim) {
        int v = fila[ini++];                      /* desenfileira */
        b.ordem[b.qtd_visitados++] = v;
        printf("  visita [%d] %s\n", v, g->nome[v]);

        if (v == objetivo) { b.achou = 1; break; }

        for (Vertice *p = lista[v]; p; p = p->prox) {
            b.arestas_percorridas++;
            if (!visitado[p->destino]) {
                visitado[p->destino] = 1;
                pai[p->destino] = v;
                b.arestas_arvore++;
                fila[fim++] = p->destino;
            }
        }
    }

    if (b.achou) {
        /* reconstroi o caminho andando pelos pais, do objetivo ate a origem */
        int tmp[MAX_V], t = 0;
        for (int v = objetivo; v != -1; v = pai[v]) tmp[t++] = v;
        for (int i = t - 1; i >= 0; i--) b.caminho[b.tam_caminho++] = tmp[i];
    }
    relatorio(g, "BFS", &b);
}

/* --- Desafio: o que preciso concluir ANTES de cursar a disciplina X? --- */

/* DFS no grafo reverso; o pos-ordem coloca cada pre-requisito antes
 * de quem depende dele, ou seja, ja sai numa ordem valida de cursar. */
static void pos_ordem(const Grafo *g, int v, int visitado[], int saida[], int *qtd)
{
    visitado[v] = 1;
    for (Vertice *p = g->radj[v]; p; p = p->prox)
        if (!visitado[p->destino]) pos_ordem(g, p->destino, visitado, saida, qtd);
    saida[(*qtd)++] = v;
}

static void consultar_prerequisitos(const Grafo *g, int alvo)
{
    int visitado[MAX_V] = {0};
    int ordem[MAX_V];
    int qtd = 0;

    pos_ordem(g, alvo, visitado, ordem, &qtd);

    printf("\n=== PRE-REQUISITOS DE: [%d] %s ===\n", alvo, g->nome[alvo]);
    if (qtd == 1) {
        printf("Nenhum pre-requisito. Pode cursar direto.\n");
        return;
    }

    printf("Precisam estar concluidas (%d disciplinas):\n", qtd - 1);
    for (int i = 0; i < qtd - 1; i++)
        printf("  - [%d] %s\n", ordem[i], g->nome[ordem[i]]);

    printf("Uma ordem valida para cursar ate la:\n  ");
    imprimir_sequencia(g, ordem, qtd);
}

/* --- Entrada de dados --- */

static int ler_int(const char *prompt)
{
    char buf[64];
    int x;
    for (;;) {
        printf("%s", prompt);
        if (!fgets(buf, sizeof(buf), stdin)) exit(0);
        if (sscanf(buf, "%d", &x) == 1) return x;
        printf("Entrada invalida, tente de novo.\n");
    }
}

static void ler_texto(const char *prompt, char *dest, int tam)
{
    printf("%s", prompt);
    if (!fgets(dest, tam, stdin)) exit(0);
    dest[strcspn(dest, "\r\n")] = '\0';
}

static int ler_vertice(const Grafo *g, const char *prompt)
{
    for (;;) {
        int v = ler_int(prompt);
        if (v >= 0 && v < g->n) return v;
        printf("Vertice inexistente (use 0 a %d).\n", g->n - 1);
    }
}

/* objetivo opcional: -1 = varredura completa */
static int ler_objetivo(const Grafo *g)
{
    for (;;) {
        int v = ler_int("Objetivo (numero da disciplina, ou -1 para varrer tudo): ");
        if (v == -1 || (v >= 0 && v < g->n)) return v;
        printf("Vertice inexistente (use -1 ou 0 a %d).\n", g->n - 1);
    }
}

/* --- Menu --- */

static void menu(void)
{
    printf("\n========== GRAFO DE PRE-REQUISITOS ==========\n");
    printf(" 1 - Listar disciplinas e relacoes\n");
    printf(" 2 - Adicionar disciplina\n");
    printf(" 3 - Adicionar pre-requisito (A -> B)\n");
    printf(" 4 - Busca em profundidade (DFS)\n");
    printf(" 5 - Busca em largura (BFS)\n");
    printf(" 6 - O que preciso concluir antes de cursar X?\n");
    printf(" 0 - Sair\n");
    printf("=============================================\n");
}

int main(void)
{
    Grafo g;
    grafo_init(&g);
    carregar_exemplo(&g);

    int op;
    do {
        menu();
        op = ler_int("Opcao: ");

        switch (op) {
        case 1:
            listar_disciplinas(&g);
            break;

        case 2: {
            char nome[MAX_NOME];
            ler_texto("Nome da disciplina: ", nome, sizeof(nome));
            if (nome[0] == '\0') { printf("Nome vazio.\n"); break; }
            int id = grafo_add_vertice(&g, nome);
            if (id < 0) printf("Limite de %d disciplinas atingido.\n", MAX_V);
            else        printf("Disciplina adicionada com id %d.\n", id);
            break;
        }

        case 3: {
            listar_disciplinas(&g);
            int a = ler_vertice(&g, "\nPre-requisito (A): ");
            int b = ler_vertice(&g, "Disciplina que depende dele (B): ");
            switch (grafo_add_aresta(&g, a, b)) {
            case 0:  printf("'%s' agora e pre-requisito de '%s'.\n", g.nome[a], g.nome[b]); break;
            case -2: printf("Essa relacao ja existe.\n"); break;
            case -3: printf("Recusado: isso criaria um ciclo de pre-requisitos.\n"); break;
            default: printf("Relacao invalida.\n");
            }
            break;
        }

        case 4: {
            int o = ler_vertice(&g, "Origem (numero da disciplina): ");
            int t = ler_objetivo(&g);
            executar_dfs(&g, g.adj, o, t, "BUSCA EM PROFUNDIDADE (DFS)");
            break;
        }

        case 5: {
            int o = ler_vertice(&g, "Origem (numero da disciplina): ");
            int t = ler_objetivo(&g);
            executar_bfs(&g, g.adj, o, t);
            break;
        }

        case 6: {
            int x = ler_vertice(&g, "Disciplina alvo (numero): ");
            consultar_prerequisitos(&g, x);
            break;
        }

        case 0:
            printf("Saindo.\n");
            break;

        default:
            printf("Opcao invalida.\n");
        }
    } while (op != 0);

    grafo_liberar(&g);
    return 0;
}

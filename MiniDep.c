#include <stdio.h>
#include <string.h>

#define MAX_THREADS 5 // threads
#define MAX_LOCKS   40 // nodes

/*
 * detect cycle /// main algo --> dfs ----
 * build graph /// how to listen to threads + thread? --> locks
 * tracking --> // listening ???
 * locks --> graph
 * graph --> nodes --> locks
 * */

/////////////////// JUST TESTS ///////////////////
// th1 --> 4
// th2 --> 4 , 5
// th3 --> 4 , 5
// 4 -> 4 -> 4 -> 5 -> 4

// thread lock --> A , B
// thread 1 -->  4 , 5
// thread 2 -->  5 , 4
// thread 3 -->  5 , 6

struct thread_info {
    int pid;
    int held_locks[MAX_LOCKS];
    int lock_count;
};

// replacing vector<vector<int>> graph
int  graph[MAX_LOCKS+1][MAX_LOCKS+1];
int  graph_degree[MAX_LOCKS+1];

int vis[MAX_LOCKS+1];
int Pathstack[MAX_LOCKS+1];


int dfs(int i) {
    vis[i]       = 1;
    Pathstack[i] = 1;

    int j;
    for (j = 0; j < graph_degree[i]; j++) {
        int x = graph[i][j];
        if (!vis[x]) {
            if (dfs(x)) return 1;
        } else if (Pathstack[x]) {
            return 1;
        }
    }

    Pathstack[i] = 0;
    return 0;
}

int check_For_Safety() {
    memset(vis,       0, sizeof(vis));
    memset(Pathstack, 0, sizeof(Pathstack));

    int k;
    for (k = 1; k <= MAX_LOCKS; k++) {
        if (!vis[k] && graph_degree[k] > 0) {
            if (dfs(k)) {
                printf("\n\n=====================\n\n"
                      "WARNING : THERE IS A DEADLOCK HERE !!! , THREADS ARE STOPPED \n\n===================="
                      "\n\n ");
                return 0;
            }
        }
    }
    return 1;
}

void simulate_Test(struct thread_info *test_cases, int *num_threads) {

    test_cases[0].pid           = 1;
    test_cases[0].lock_count    = 2;
    test_cases[0].held_locks[0] = 4;
    test_cases[0].held_locks[1] = 5;

    test_cases[1].pid           = 2;
    test_cases[1].lock_count    = 2;
    test_cases[1].held_locks[0] = 5;
    test_cases[1].held_locks[1] = 4;

    test_cases[2].pid           = 3;
    test_cases[2].lock_count    = 2;
    test_cases[2].held_locks[0] = 5;
    test_cases[2].held_locks[1] = 6;

    *num_threads = 3;
}

int main() {

    memset(graph,        0, sizeof(graph));
    memset(graph_degree, 0, sizeof(graph_degree));
    memset(vis,          0, sizeof(vis));

    struct thread_info test_cases[MAX_THREADS];
    int num_threads = 0;
    simulate_Test(test_cases, &num_threads);


    int i, j, prev;
    for (i = 0; i < num_threads; i++) {

        if (test_cases[i].lock_count > 0)
            printf("%d-->", test_cases[i].held_locks[0]);

        prev = -1;
        for (j = 0; j < test_cases[i].lock_count; j++, prev++) {
            if (prev == -1)
                continue;

            // graph[from].push_back(to)
            graph[test_cases[i].held_locks[prev]][graph_degree[test_cases[i].held_locks[prev]]] = test_cases[i].held_locks[j];
            graph_degree[test_cases[i].held_locks[prev]]++;

            printf("%d-->", test_cases[i].held_locks[j]);

            if (!check_For_Safety())
                return 1;
        }
    }

    return 0;
}
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>

#define MAX_THREADS 5
#define MAX_LOCKS   40

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

struct processThreadInfo {
    int pid;
    int held_locks[MAX_LOCKS];
    int lock_count;
};

/* Main graph representation */
static int graph[MAX_LOCKS + 1][MAX_LOCKS + 1];
static int graph_degree[MAX_LOCKS + 1];

static int vis[MAX_LOCKS + 1];
static int Pathstack[MAX_LOCKS + 1];

/*Accumulator for storing the order of lock acquisitions */
static int ans[MAX_THREADS * MAX_LOCKS];
static int ans_size = 0;

/* print the entire ans chain from the beginning, e.g.  4-->5-->4--> */
static void print_ans(void) {
    int k;
    for (k = 0; k < ans_size; k++)
        pr_cont("%d-->", ans[k]);
    pr_cont("\n");
}


/*DFS function for main algorithm of deadlock detection */
static int dfs(int i) {// directed graph
    int j;
    vis[i]       = 1;
    Pathstack[i] = 1;

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

/* Safety check function */
static int check_For_Safety(void) {
    int k;
    memset(vis,       0, sizeof(vis));
    memset(Pathstack, 0, sizeof(Pathstack));

    for (k = 1; k <= MAX_LOCKS; k++) {
        if (!vis[k] && graph_degree[k] > 0) {
            if (dfs(k)) {
                pr_warn("\n\n=====================\n\n"
                        "WARNING : THERE IS A DEADLOCK HERE !!! , THREADS ARE STOPPED \n\n"
                        "====================\n\n");
                return 0;
            }
        }
    }
    return 1;
}


/* Simulation function for generating test cases */
static void simulate_Test(struct processThreadInfo *test_cases, int *num_threads) {

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




static int __init minidep_init(void) {
    int i, j, prev;
    struct processThreadInfo test_cases[MAX_THREADS];
    int num_threads = 0;

    memset(graph,        0, sizeof(graph));
    memset(graph_degree, 0, sizeof(graph_degree));
    memset(vis,          0, sizeof(vis));

    simulate_Test(test_cases, &num_threads);

    for (i = 0; i < num_threads; i++) {

        /* printing that thread has locks */
        pr_info("MiniDep: thread %d has locks : ", test_cases[i].pid);
        for (j = 0; j < test_cases[i].lock_count; j++)
            pr_cont("%d-->", test_cases[i].held_locks[j]);
        pr_cont("\n\n");

        
        if (test_cases[i].lock_count > 0)
            ans[ans_size++] = test_cases[i].held_locks[0];

        prev = -1;
        for (j = 0; j < test_cases[i].lock_count; j++, prev++) {
            if (prev == -1) {
                print_ans();
                continue;
            }

            /* graph[from].push_back(to) */
            graph[test_cases[i].held_locks[prev]][graph_degree[test_cases[i].held_locks[prev]]]
                = test_cases[i].held_locks[j];
            graph_degree[test_cases[i].held_locks[prev]]++;

            
            ans[ans_size++] = test_cases[i].held_locks[j];
            print_ans();

            if (!check_For_Safety())
                return 0;
        }
        pr_info("\n");
    }

    pr_info("MiniDep: no deadlock detected.\n");
    return 0;
}

static void __exit minidep_exit(void) {
    pr_info("MiniDep: unloading module\n");
}

module_init(minidep_init);
module_exit(minidep_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Fatema&Menna");
MODULE_DESCRIPTION("MiniDep: deadlock detector as kernel module");

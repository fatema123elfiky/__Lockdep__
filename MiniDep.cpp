#include <iostream>
#include <vector>
#include <cstring>


using namespace std;
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
    pid_t pid;
    int held_locks[MAX_LOCKS];
    int lock_count;
    thread_info(int pid , int lock_count ):pid(pid), lock_count(lock_count){}
};


vector<vector<int>>graph;
bool vis[MAX_LOCKS+1];
bool Pathstack[MAX_LOCKS+1];


bool dfs(int i) {
    vis[i]       = true;
    Pathstack[i] = true;

    for (auto x : graph[i]) {
        if (!vis[x]) {
            if (dfs(x)) return true;
        } else if (Pathstack[x]) {
            return true;
        }
    }

    Pathstack[i] = false;
    return false;
}
bool check_For_Safety (){
    memset(vis,false, sizeof(vis));
    memset(Pathstack, false, sizeof(Pathstack));
    for (int k = 1; k <= MAX_LOCKS; ++k) {
        if(!vis[k] && graph[k].size()>0) {
            if(dfs(k)) {
                cout<<"\n\n=====================\n\n"
                      "WARNING : THERE IS A DEADLOCK HERE !!! , THREADS ARE STOPPED \n\n===================="
                      "\n\n ";
                return false;
            }

        }
    }

    return true;

}




vector<thread_info> simulate_Test (){
    thread_info th1 = thread_info(1,2);
    th1.held_locks[0]=4,th1.held_locks[1]=5;

    thread_info th2 = thread_info(2,2);
    th2.held_locks[0]=5,th2.held_locks[1]=4;

    thread_info th3 = thread_info(3,2);
    th3.held_locks[0]=5,th3.held_locks[1]=6;

    return vector<thread_info>{th1,th2,th3};
}

int main(){

    graph.assign(MAX_LOCKS+1,{});
    memset(vis,false, sizeof(vis));

    // hard code tests
    vector<thread_info >test_cases=simulate_Test();// as tracker

    for (int i = 0; i < test_cases.size(); ++i) {//th1 , th2 , th3


        if(test_cases[i].lock_count>0)
            cout<<test_cases[i].held_locks[0]<<"-->";

        for (int j = 0,prev=-1; j < test_cases[i].lock_count; ++j,prev++) {
            if(prev==-1)
                continue;
            graph[test_cases[i].held_locks[prev]].push_back(test_cases[i].held_locks[j]);
            cout<<test_cases[i].held_locks[j]<<"-->";

           if(!check_For_Safety())
               return 1;

        }



    }



}
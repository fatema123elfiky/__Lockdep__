#include <iostream>
#include <vector>
using namespace std;
#define MAX_THREADS 5 //
#define MAX_LOCKS   40 // nodes
/*
 * detect cycle /// main algo --> dfs ----
 * build graph /// how to listen to threads + thread? --> locks
 * tracking --> // listening ???
 *
 * */

struct thread_info {
    pid_t pid;
    int held_locks[MAX_LOCKS];
    int lock_count;
    thread_info(int pid , int lock_count ):pid(pid), lock_count(lock_count){}
};
// thread lock --> A , B
// thread 1 -->  4 , 5
// thread 2 -->  5 , 4
// thread 3 -->  5 , 6

// locks --> graph
// graph --> nodes --> locks

vector<vector<int>>graph;
vector<bool>vis;
bool cycle=false;
void  dfs(int par , int i){

    vis[i]=true;
    for(auto x : graph[i]){
        if(x==i)continue;
        if(!vis[x]){
            dfs(i,x);
        }else{
            cycle=true;
        }
    }

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
    vis.assign(MAX_LOCKS+1,false);


    // hard code tests
    vector<thread_info >test_cases=simulate_Test();// as tracker

    for (int i = 0; i < test_cases.size(); ++i) {//th1 , th2 , th3
        // th1 --> 4
        // th2 --> 4 , 5
        // th3 --> 4 , 5
        // 4 -> 4 -> 4 -> 5 -> 4
        if(test_cases[i].lock_count>0)
            cout<<test_cases[i].held_locks[0]<<"-->";
        for (int j = 0,prev=-1; j < test_cases[i].lock_count; ++j,prev++) {
            if(prev==-1)
                continue;
            graph[test_cases[i].held_locks[prev]].push_back(test_cases[i].held_locks[j]);
            cout<<test_cases[i].held_locks[j]<<"-->";
            vis.assign(MAX_LOCKS+1,false);
            for (int k = 1; k <= MAX_LOCKS; ++k) {
                if(!vis[k] && graph[k].size()>0) {
                    dfs(-1, k);
                    if(cycle)
                        break;
                    cycle=false;
                }
            }

        }

    }



}
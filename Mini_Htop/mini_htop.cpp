#include<iostream>
#include<fstream>
#include<thread>
#include<chrono>
#include<iomanip>
#include<csignal>
#include<termios.h>
#include<dirent.h>
#include<vector>
#include<unistd.h>

#define GREEN "\x1b[1;32m"
#define RESET "\x1b[0m"
#define ULL unsigned long long

using namespace std;

int cpu_bar_length;
int sleep_time;
int process_cmd_len = 100;
string temp;
string proc_stat_path;
string proc_loadavg_path;

pair<ULL, ULL> get_total_idle_times(ifstream&file){
    file.seekg(0);
    file>>temp;
    ULL cpu_time;
    ULL total_time = 0;
    ULL idle_time = 0;
    for(int i=0;i<10;i++){
        file >> cpu_time;
        total_time += cpu_time;
        if (i==3 || i==4) idle_time += cpu_time;
    }
    return {total_time, idle_time};
}

void set_terminal(bool echo_enable = false){
    termios term;
    tcgetattr(STDIN_FILENO, &term);
    if(echo_enable) term.c_lflag |= ECHO;
    else  term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}
void exit_handler(int signal_num){
    cout << "\033[?25h" << endl;
    set_terminal(true);
    if(signal_num == SIGTSTP){
        signal(SIGTSTP, SIG_DFL);
        raise(SIGTSTP);
        return;
    }
    exit(signal_num);
}
void set_configuration(int);
void set_signals(){
    signal(SIGINT, exit_handler);
    signal(SIGTSTP, exit_handler);
    signal(SIGTERM, exit_handler);
    signal(SIGCONT, set_configuration);
    signal(SIGQUIT, exit_handler);
}

void set_configuration(int signal_num = 0){
    cout << fixed<<setprecision(2);
    cout << "\033[?25l"; // hide the cursor in terminal
    cout << "\033[s"; // to save the first position
    cpu_bar_length = 50;
    sleep_time = 1000;
    proc_stat_path = "/proc/stat";
    proc_loadavg_path = "/proc/loadavg";
    set_signals();
    set_terminal();
}
int main(){
    // set configuration
    set_configuration();
    ifstream proc_loadavg_file(proc_loadavg_path);
    ifstream proc_stat_file(proc_stat_path);
    bool period = false;
    ULL total_times[2]{0};
    ULL idle_times[2]{0};
    pair<ULL, ULL> total_idle;
    ULL total_delta;
    ULL idle_delta;
    double idle_ratio;
    double usage_ratio;
    total_idle = get_total_idle_times(proc_stat_file);
    total_times[period] = total_idle.first;
    idle_times[period] = total_idle.second;
    sleep_time = max(100, sleep_time);
    string proc_path = "/proc";
    
    while (true){
        this_thread::sleep_for(chrono::milliseconds(sleep_time));
        // this part for CPU Usage
        period = !period;
        total_idle = get_total_idle_times(proc_stat_file);
        total_times[period] = total_idle.first;
        idle_times[period] = total_idle.second;
        total_delta = total_times[period] - total_times[!period];
        idle_delta = idle_times[period] - idle_times[!period];
        usage_ratio = 1 - (double)idle_delta / total_delta;
        cout << left << GREEN << setw(6) << "CPU" << RESET
            << "[" << setfill('-') << setw(cpu_bar_length) << string(usage_ratio*cpu_bar_length+0.5,'#')<<"] "
            << right << setfill(' ') << setw(6) << usage_ratio*100 << "%\n";

        // for # running processes
        proc_loadavg_file.seekg(0);
        proc_loadavg_file >> temp >> temp >> temp >> temp;
        cout << GREEN << left << setw(6) << "PROC" << RESET << temp.substr(0, temp.find('/')) << '\n';
        long numProcessors = sysconf(_SC_NPROCESSORS_ONLN);
        cout<<numProcessors<<'\n';
        DIR *proc_dir = opendir(proc_path.c_str());
        dirent *proc_entry;
        // this vector will contain process ids, its path/name
        vector<pair<string, string>> pid_paths;
        while (proc_entry = readdir(proc_dir)){
            if(proc_entry->d_type != DT_DIR || !isdigit(proc_entry->d_name[0])) continue;
            string process_path = proc_path + "/"s + proc_entry->d_name;
            ifstream proc_cmdline_file(process_path + "/cmdline"s);
            if(proc_cmdline_file>>temp){
                for(int i = min(process_cmd_len, (int)temp.size()); i+1 ;i--)
                    if(temp[i] == '\0') temp[i]= ' ';
                pid_paths.push_back({
                    proc_entry->d_name,
                    temp.substr(0, process_cmd_len)
                });    
            
            }else{
                ifstream proc_stat_file(process_path + "/stat"s);
                if(proc_stat_file>>temp>>temp){
                    pid_paths.push_back({
                        proc_entry->d_name,
                        temp.substr(1, temp.size()-2)
                    });
                }
            }
        }
        int iters = 10;
        for(auto it = pid_paths.begin();it != pid_paths.end() && iters--;it++){
            cout<<it->first<<' '<<it->second<<'\n';
        }
        cout << "\033[u" << flush;
    }
    return 0;
}
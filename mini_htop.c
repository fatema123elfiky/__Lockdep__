#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<signal.h>
#include<termios.h>
#include<dirent.h>
#include<stdlib.h>
#include<ctype.h>

#define GREEN "\x1b[1;32m"
#define RESET "\x1b[0m"
#define ULL unsigned long long


int cpu_bar_len;
int sleep_time;
int process_cmd_len = 100;
float ftemp;
int ctemp;
char stemp[256];
char* proc_stat_path;
char* proc_loadavg_path;
int max(int x, int y){return x>y?x:y;}
ULL get_total_idle_times(FILE* file, ULL *idle_time){
    if(!file) return 0ULL;
    rewind(file);
    fscanf(file, "%s ", stemp);

    ULL cpu_time;
    ULL total_time = 0;
    *idle_time = 0;
    for(int i=0;i<10;i++){
        fscanf(file, "%llu", &cpu_time);
        total_time += cpu_time;
        if (i==3 || i==4) *idle_time += cpu_time;
    }
    return total_time;
}

void set_terminal(char echo_enable){
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    if(echo_enable) term.c_lflag |= ECHO;
    else  term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}
void exit_handler(int signal_num){
    printf("\033[?25h\n");
    set_terminal(1);
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

void set_configuration(int signal_num){
    printf("\033[?25l"); // hide the cursor in terminal
    // printf("\033[2J\033[H"); //clear screen
    printf("\033[s"); // to save the first position
    cpu_bar_len = 50;
    sleep_time = 1000;
    proc_stat_path = "/proc/stat";
    proc_loadavg_path = "/proc/loadavg";
    set_signals();
    set_terminal(0);
}
int main(){
    // set configuration
    set_configuration(0);
    FILE* proc_stat_file = fopen(proc_stat_path, "r");
    FILE* proc_loadavg_file = fopen(proc_loadavg_path, "r");
    if(!proc_loadavg_file || !proc_stat_file){
        printf("I can't read %s or %s", proc_stat_path, proc_loadavg_path);
        return 0;
    }
    setvbuf(proc_stat_file, 0,_IONBF, 0);
    setvbuf(proc_loadavg_file, 0,_IONBF, 0);
    char period = 0;
    
    ULL total_times[2] = {0, 0};
    ULL idle_times[2] = {0, 0};
    ULL total_time;
    ULL idle_time;
    ULL total_delta;
    ULL idle_delta;
    double idle_ratio;
    double usage_ratio;
    total_time = get_total_idle_times(proc_stat_file, &idle_time);
    total_times[period] = total_time;
    idle_times[period] = idle_time;
    sleep_time = max(100, sleep_time);
    char* proc_path = "/proc/";
    char cpu_bar[cpu_bar_len +1];
    cpu_bar[cpu_bar_len] = '\0';
    int usage_len;
    int proc_run_num;
    int proc_name_len;
    int zero = 0;
    int proc_cmdline_len;
    char proc_cmdline[process_cmd_len +1];

    proc_cmdline[process_cmd_len] = '\0';
    
    while (1){
        sleep(1);
        // this part for CPU Usage
        period = !period;
        total_time = get_total_idle_times(proc_stat_file, &idle_time);
    
        total_times[period] = total_time;
        idle_times[period] = idle_time;
        total_delta = total_times[period] - total_times[!period];
        idle_delta = idle_times[period] - idle_times[!period];
        usage_ratio = 1 - (double)idle_delta / total_delta;
        usage_len = usage_ratio * cpu_bar_len + 0.5;
        memset(cpu_bar, '#', usage_len);
        memset(cpu_bar + usage_len, '-', cpu_bar_len - usage_len);
        printf("%s%-6s%s [%s] %6.2f%%\n", GREEN, "CPU", RESET, cpu_bar, usage_ratio * 100);
        
        // for # running processes
        rewind(proc_loadavg_file);
        fscanf(proc_loadavg_file, "%f%f%f%d", &ftemp, &ftemp, &ftemp, &proc_run_num);
        printf("%s%-6s%s %d\n", GREEN, "PROC", RESET, proc_run_num);
        DIR *proc_dir = opendir(proc_path);
        struct dirent *proc_entry;

        int iters = 10;
        while (proc_entry = readdir(proc_dir)){
            if(proc_entry->d_type != 4 || !isdigit(proc_entry->d_name[0])) continue;
            if(!iters--){
                fflush(stdout);
                break;
            }
            proc_name_len = strlen(proc_entry->d_name);
            char process_path[6 + proc_name_len + 1];
            strcpy(process_path, proc_path);
            strcat(process_path, proc_entry->d_name);
            char proc_cmdline_path[6 + proc_name_len + 8 + 1];
            strcpy(proc_cmdline_path, process_path);
            strcat(proc_cmdline_path, "/cmdline");
            FILE* proc_cmdline_file = fopen(proc_cmdline_path, "r");
            if(!proc_cmdline_file) continue;
            int i = 0;
            while(i<process_cmd_len && (ctemp=fgetc(proc_cmdline_file)) != EOF){
                proc_cmdline[i++] = ctemp ? ctemp : ' ';
            }
            proc_cmdline[i] = '\0';
            if(i){
                printf("%s\t%s\n", proc_entry->d_name, proc_cmdline);
            }else{
                char process_stat_path[6 + proc_name_len + 5 + 1];
                strcpy(process_stat_path, process_path);
                strcat(process_stat_path, "/stat");
                FILE* process_stat_file = fopen(process_stat_path, "r");
                if(!process_stat_file) continue;
                char valid = 0;
                while(i<process_cmd_len && (ctemp=fgetc(process_stat_file)) != EOF){
                    if(ctemp==')') break;
                    if(valid)
                        proc_cmdline[i++] = ctemp ? ctemp : ' ';
                    if(ctemp=='(') valid=1;
                }
                proc_cmdline[i]='\0';
                printf("%s\t%s\n", proc_entry->d_name, proc_cmdline);
            }
            fclose(proc_cmdline_file);
        }
        closedir(proc_dir);
        printf("\033[u");
        fflush(stdout);
    }
    return 0;
}
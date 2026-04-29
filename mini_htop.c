#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<signal.h>
#include<termios.h>
#include<dirent.h>
#include<stdlib.h>
#include<ctype.h>
#include<fcntl.h>
#include<sys/select.h>

#define RESET    "\x1b[0m"
#define GREEN    "\x1b[1;32m"
#define CYAN     "\x1b[1;36m"
#define YELLOW   "\x1b[1;33m"
#define ULL unsigned long long

int cpu_bar_len     = 30;
int process_cmd_len = 100;
float ftemp;
int ctemp;
char stemp[256];
char* proc_stat_path;
char* proc_loadavg_path;

int max(int x, int y){ return x > y ? x : y; }

/* reads /proc/stat → returns total CPU time, writes idle time to pointer
 * called twice 1 sec apart → delta between calls = actual usage */
ULL get_total_idle_times(FILE* file, ULL *idle_time){
    if(!file) return 0ULL;
    rewind(file); /* rewind every call: /proc/stat is a live kernel file */
    fscanf(file, "%s ", stemp); /* skip the word "cpu" */
    ULL cpu_time, total_time = 0;
    *idle_time = 0;
    for(int i = 0; i < 10; i++){
        fscanf(file, "%llu", &cpu_time);
        total_time += cpu_time;
        if(i == 3 || i == 4) *idle_time += cpu_time; /* col 3=idle, 4=iowait */
    }
    return total_time;
}

/* reads /proc/meminfo → fills mem_total and mem_available in KB */
int get_memory_info(unsigned long *mem_total, unsigned long *mem_available){
    FILE *f = fopen("/proc/meminfo", "r");
    if(!f) return 0;
    char label[64];
    unsigned long value;
    *mem_total = 0; *mem_available = 0;
    while(fscanf(f, "%s %lu %*s", label, &value) == 2){
        if(strcmp(label, "MemTotal:")     == 0) *mem_total     = value;
        if(strcmp(label, "MemAvailable:") == 0) *mem_available = value;
        if(*mem_total && *mem_available) break;
    }
    fclose(f);
    return 1;
}

/* reads /proc/uptime → returns seconds since boot */
int get_uptime_secs(){
    FILE *f = fopen("/proc/uptime", "r");
    if(!f) return 0;
    double up;
    fscanf(f, "%lf", &up);
    fclose(f);
    return (int)up;
}

/* checks keyboard without blocking → returns char or -1 if nothing pressed */
int read_key_nonblocking(){
    struct timeval tv = {0, 0}; /* zero timeout = dont wait */
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if(select(1, &fds, NULL, NULL, &tv) > 0) return getchar();
    return -1;
}

/* switches terminal between normal mode (1) and raw mode (0)
 * raw mode: keypresses hidden + read instantly without Enter */
void set_terminal(char echo_enable){
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    if(echo_enable){ term.c_lflag |= ECHO;  term.c_lflag |= ICANON;  }
    else            { term.c_lflag &= ~ECHO; term.c_lflag &= ~ICANON; }
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

/* restores terminal to normal state before exit */
void exit_handler(int signal_num){
    printf("\033[?25h"); /* show cursor */
    printf("\033[0m\n"); /* reset colors */
    set_terminal(1);     /* restore echo */
    if(signal_num == SIGTSTP){
        signal(SIGTSTP, SIG_DFL);
        raise(SIGTSTP);
        return;
    }
    exit(signal_num);
}

void set_signals();
void set_configuration(int signal_num){
    printf("\033[?25l");     /* hide cursor */
    printf("\033[2J\033[H"); /* clear screen once at startup */
    cpu_bar_len       = 30;
    proc_stat_path    = "/proc/stat";
    proc_loadavg_path = "/proc/loadavg";
    set_signals();
    set_terminal(0);
}

void set_signals(){
    signal(SIGINT,  exit_handler);
    signal(SIGTSTP, exit_handler);
    signal(SIGTERM, exit_handler);
    signal(SIGCONT, set_configuration);
    signal(SIGQUIT, exit_handler);
}

/* draws [###---] bar
 *
 * WHY 0x23 and 0x2D instead of '#' and '-':
 * memset fills memory BYTE BY BYTE with the given value
 * '#' = 0x23 and '-' = 0x2D, both fit in 1 byte so memset is safe
 * using memset with numbers like 2 on int arrays is WRONG because
 * each int is 4 bytes and becomes 0x02020202 = 33,686,018 not 2 */
void print_bar(double ratio, int bar_len){
    int filled = ratio * bar_len + 0.5;
    char bar[bar_len + 1];
    bar[bar_len] = '\0';
    memset(bar,          0x23, filled);           /* 0x23 = '#' */
    memset(bar + filled, 0x2D, bar_len - filled); /* 0x2D = '-' */
    printf("[%s]", bar);
}

int main(){
    set_configuration(0);

    FILE* proc_stat_file    = fopen(proc_stat_path, "r");
    FILE* proc_loadavg_file = fopen(proc_loadavg_path, "r");
    if(!proc_stat_file || !proc_loadavg_file){
        printf("Cannot open /proc files\n");
        return 1;
    }

    /* _IONBF disables buffering → every read goes directly to kernel
     * needed because /proc files are live and change every millisecond */
    setvbuf(proc_stat_file,    0, _IONBF, 0);
    setvbuf(proc_loadavg_file, 0, _IONBF, 0);

    char period = 0;
    ULL total_times[2] = {0,0}, idle_times[2] = {0,0};
    ULL total_time, idle_time, total_delta, idle_delta;
    double usage_ratio;

    /* first snapshot before loop starts */
    total_time          = get_total_idle_times(proc_stat_file, &idle_time);
    total_times[period] = total_time;
    idle_times[period]  = idle_time;

    int proc_run_num, proc_name_len;
    char proc_cmdline[process_cmd_len + 1];
    proc_cmdline[process_cmd_len] = '\0';
    unsigned long mem_total, mem_available, mem_used;
    double mem_ratio;
    char* proc_path = "/proc/";

    while(1){
        sleep(1);

        /* quit if user pressed q */
        int key = read_key_nonblocking();
        if(key == 'q' || key == 'Q') exit_handler(0);

        /* CPU: take second snapshot and compute delta */
        period = !period;
        total_time          = get_total_idle_times(proc_stat_file, &idle_time);
        total_times[period] = total_time;
        idle_times[period]  = idle_time;
        total_delta = total_times[period] - total_times[!period];
        idle_delta  = idle_times[period]  - idle_times[!period];
        usage_ratio = 1.0 - (double)idle_delta / total_delta;

        /* memory */
        get_memory_info(&mem_total, &mem_available);
        mem_used  = mem_total - mem_available;
        mem_ratio = (double)mem_used / mem_total;

        /* process count */
        rewind(proc_loadavg_file);
        fscanf(proc_loadavg_file, "%f%f%f%d", &ftemp, &ftemp, &ftemp, &proc_run_num);

        /* uptime */
        int uptime = get_uptime_secs();

        /*
         * \033[H moves cursor to row 1 col 1 (top-left) every frame
         * this is what makes the display refresh IN PLACE
         *
         * WHY NOT \033[u:
         * \033[s saves position ONCE at row 20 after first print
         * \033[u jumps back to row 20 not row 1 → output scrolls down forever
         * \033[H always goes to row 1 → overwrites same lines every time
         *
         * \033[K clears from cursor to end of line after each printf
         * needed to erase leftover characters if new line is shorter
         * example: "12.44%" then "8.01%" → without \033[K shows "8.01%4"
         */
        printf("\033[H");

        /* draw UI */
        printf("%sMini-HTOP v2 Pro%s\033[K\n", GREEN, RESET);
        printf("Press q to quit\033[K\n");
        printf("\033[K\n");

        printf("%sCPU%s  ", CYAN, RESET);
        print_bar(usage_ratio, cpu_bar_len);
        printf("  %5.2f%%\033[K\n", usage_ratio * 100);

        printf("%sMEM%s  ", CYAN, RESET);
        print_bar(mem_ratio, cpu_bar_len);
        printf("  %5.2f%%  (%lu / %lu MB)\033[K\n",
               mem_ratio * 100, mem_used / 1024, mem_total / 1024);

        printf("%sPROC%s   %d\033[K\n",       CYAN, RESET, proc_run_num);
        printf("%sUPTIME%s %d sec\033[K\n",   CYAN, RESET, uptime);
        printf("\033[K\n");

        printf("%s%-8s  %s%s\033[K\n", YELLOW, "PID", "NAME", RESET);
        printf("%s", CYAN);
        for(int i = 0; i < 40; i++) printf("-");
        printf("%s\033[K\n", RESET);

        /* scan /proc/ for PID folders */
        DIR *proc_dir = opendir(proc_path);
        struct dirent *proc_entry;
        int iters = 15;

        while(proc_entry = readdir(proc_dir)){
            /* skip non-directories and non-numeric names */
            if(proc_entry->d_type != 4 || !isdigit(proc_entry->d_name[0])) continue;
            if(!iters--){ fflush(stdout); break; }

            proc_name_len = strlen(proc_entry->d_name);

            char process_path[6 + proc_name_len + 1];
            strcpy(process_path, proc_path);
            strcat(process_path, proc_entry->d_name);

            /* try /proc/[pid]/cmdline first (user processes) */
            char proc_cmdline_path[6 + proc_name_len + 8 + 1];
            strcpy(proc_cmdline_path, process_path);
            strcat(proc_cmdline_path, "/cmdline");

            FILE* proc_cmdline_file = fopen(proc_cmdline_path, "r");
            if(!proc_cmdline_file) continue;

            int i = 0;
            /* cmdline args are separated by \0, replace with space */
            while(i < process_cmd_len && (ctemp = fgetc(proc_cmdline_file)) != EOF)
                proc_cmdline[i++] = ctemp ? ctemp : ' ';
            proc_cmdline[i] = '\0';

            if(i){
                /* user process: print cmdline */
                printf("%-8s  %s\033[K\n", proc_entry->d_name, proc_cmdline);
            } else {
                /* kernel thread: cmdline is empty, read name from /proc/[pid]/stat
                 * stat format: "PID (name) state ..." → extract between ( and ) */
                char process_stat_path[6 + proc_name_len + 5 + 1];
                strcpy(process_stat_path, process_path);
                strcat(process_stat_path, "/stat");

                FILE* process_stat_file = fopen(process_stat_path, "r");
                if(!process_stat_file){ fclose(proc_cmdline_file); continue; }

                char valid = 0; i = 0;
                while(i < process_cmd_len && (ctemp = fgetc(process_stat_file)) != EOF){
                    if(ctemp == ')') break;
                    if(valid) proc_cmdline[i++] = ctemp ? ctemp : ' ';
                    if(ctemp == '(') valid = 1;
                }
                proc_cmdline[i] = '\0';
                printf("%-8s  %s\033[K\n", proc_entry->d_name, proc_cmdline);
                fclose(process_stat_file);
            }
            fclose(proc_cmdline_file);
        }
        closedir(proc_dir);

        fflush(stdout); /* push everything to screen immediately */
    }
    return 0;
}

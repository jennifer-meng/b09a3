#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <utmp.h> //user
#include <sys/utsname.h> //for system info
#include <sys/resource.h> //rusage for memory
#include <sys/sysinfo.h> //for memory
#include <math.h>
#include "stats_functions.h"

float cal_cpuoccupy(CPUINFO *o, CPUINFO *n) {
    unsigned long T1, T2;
    unsigned long U1, U2;
    float cpu_use = 0;
    T1 = (unsigned long) (o->utime + o->ntime + o->stime + o->itime
                          + o->iowtime + o->irqtime + o->sirqtime);//first time
    T2 = (unsigned long) (n->utime + n->ntime + n->stime + n->itime
                          + n->iowtime + n->irqtime + n->sirqtime);//second time
    U1 = (unsigned long) (T1-o->itime );    //idle
    U2 = (unsigned long) (T2-n->itime );    //idle
    if ((T2 - T1) != 0) {
        cpu_use = (U2-U1)*100.00 / (T2 - T1);
    } else {
        // In case there's no change in CPU time, set CPU usage to 0%
        cpu_use = 0;
    }
    return cpu_use;
//    printf("total cpu use =%1.2f%%\n", cpu_use);
}

//get cpu information by read the file /proc/stat
//CPUINFO *cpuinfo: Pointer to a CPUINFO structure where the CPU stats will be stored.
//This function reads the CPU statistics from the /proc/stat file and populates the given cpuinfo structure.
//get_cpu_core_count function:
//
//Return Value: Returns an integer representing the number of CPU cores or -1 if there's an error.
//This function opens /proc/cpuinfo, reads it line by line, and counts the number of unique processor entries to determine the core count.
void get_cpuinfo(CPUINFO *cpuinfo){
    FILE *fd;
    fd = fopen("/proc/stat", "r");
    fscanf(fd, "cpu  %lu %lu %lu %lu %lu %lu %lu", &cpuinfo->utime, &cpuinfo->ntime, &cpuinfo->stime,
           &cpuinfo->itime, &cpuinfo->iowtime, &cpuinfo->irqtime, &cpuinfo->sirqtime);
    fclose(fd);
}


//char cpu_graphics[][128]: A 2D character array to store CPU usage graphics.
//int i_sample: The current sample index for storing the CPU usage data.
//CPUINFO *pre_cpu_stat: Pointer to a CPUINFO structure containing the previous CPU stats.
//This function calculates the CPU usage, updates the cpu_graphics array, and updates the pre_cpu_stat with the latest CPU stats.
void get_cpu_usage(char cpu_graphics[][BUF_LEN],char* cpu_usages, int i_sample, int graphics, CPUINFO *pre_cpu_stat){
    CPUINFO cur_cpu_stat;
    get_cpuinfo((CPUINFO *) &cur_cpu_stat);
    int core_count = get_nprocs(); //get num of cores
    sprintf(cpu_usages,"---------------------------------------\n");
    sprintf(cpu_usages+strlen(cpu_usages),"Number of cores: %d\n",core_count);
    float cpu_usage=cal_cpuoccupy(pre_cpu_stat, (CPUINFO *) &cur_cpu_stat);
    sprintf(cpu_usages+strlen(cpu_usages),"total cpu use = %2.2f\% "    "  \n",cpu_usage);
    memcpy(pre_cpu_stat,&cur_cpu_stat,sizeof(CPUINFO));

    if(graphics){
        char bars[BUF_LEN] = "|||";
        int number_of_bars=(int)cpu_usage;
        for (int i = 1; i <= number_of_bars; i++)
            strcat(bars, "|");
        sprintf(cpu_graphics[i_sample], "        %s %.2lf", bars, cpu_usage);
    }
}
//int sequential: A flag indicating whether to display the CPU graphic sequentially (1 for yes, 0 for no).
//int i: The current sample index.
//int num_samples: The total number of CPU usage samples.
//const char cpu_graphics[][128]: The 2D array containing CPU usage graphics.
//This function displays the CPU usage graphics stored in the cpu_graphics array based on the sequential flag.


void get_memory_usage(char *kilobytes, char mem_graphics[][BUF_LEN], int i_sample, int graphics, double *prev_used) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) return;
    char buf[512]={0};

    sprintf(kilobytes,"Memory usage: %ld kilobytes\n", usage.ru_maxrss);
    strcat(kilobytes,"---------------------------------------\n");
    struct sysinfo info;
    sysinfo(&info);

    double GB = 1024 * 1024 * 1024;
    double phys_used, phys_tol, vir_used, vir_tol, diff;
    char *sym1, *sym2;
    //calculate
    phys_used = ((double) (info.totalram - info.freeram)) / GB;
    phys_tol = ((double) info.totalram) / GB;

    vir_used = ((double) ((info.totalram - info.freeram) +
                          (info.totalswap - info.freeswap))) / GB;
    vir_tol = ((double) (info.totalram + info.totalswap)) / GB;
    mem_graphics[i_sample][0]=0;
    //store them in array
    sprintf(buf, "%.2f GB / %.2f GB  -- %.2f GB / %.2f GB",
            phys_used, phys_tol, vir_used, vir_tol);
    strcat(mem_graphics[i_sample],buf);

    if (graphics) {
        char *rest = mem_graphics[i_sample] + strlen(mem_graphics[i_sample]);
        // sprintf()

        if (i_sample == 0)
            sprintf(rest, "   |o 0.00 (%.2f)", phys_used);
        else {
            diff = phys_used - *prev_used;

            sym1 = (diff >= 0) ? "#" : ":";
            sym2 = (diff >= 0) ? "*" : "@";

            char tmp[BUF_LEN] = {'\0'};
            //add the graphics in tmp
            for (double d = 0.0; d < fabs(diff); d += 0.01)
                strcat(tmp, sym1);
            strcat(tmp, sym2);

            sprintf(rest, "   |%s %.2f (%.2f)", tmp, diff, phys_used);
        }
    }
    *prev_used = phys_used;
}
typedef struct {
//    char username[USERNAME_LEN];
    struct utmp utmp;
} User;

//This function prints a list of active user sessions by parsing the contents of the utmp file.
char * get_user_usage() {

    // printf("---------------------------------------\n");
    // printf("### Sessions/users ###\n");
    char buf[512]={0};
    char *user_info;
    user_info=NULL;
    setutent();
    struct utmp *user = getutent();
    int data_size = 0;
    int num_read=0;
    char *tmp;
    while (user != NULL) {
        if (user->ut_type == USER_PROCESS) {
            memset(buf, 0, 512); // Initialize the newly allocated memory
            sprintf(buf, "%s\t%s (%s)\n", user->ut_user, user->ut_line, user->ut_host);
            num_read = strlen(buf);
            tmp = realloc(user_info, data_size + num_read+1);
            if (tmp)
                memset(tmp + data_size, 0, num_read); // Initialize the newly allocated memory
            else {
                perror("realloc");
                free(user_info);
                exit(1);
            }

            user_info = tmp;
            memcpy(user_info + data_size, buf, num_read+1);
            data_size += num_read;

        }
        user = getutent();
    }
    // Close the utmp file
    endutent();
    return user_info;
}
void show_memory(char *kilobyates,int num_samples, char mem_graphics[num_samples][BUF_LEN]) {
    printf("%s",kilobyates);
    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot\n");

    for (int j = 0; j < num_samples; j++) {
        printf("%s\n", mem_graphics[j]);
    }
}
void show_cpu(int sequential, int i,int num_samples, int graphics, char *cpu_usages, const char cpu_graphics[][BUF_LEN]) {
    printf("%s", cpu_usages);
    if (sequential) {
        for (int j = 0; j < num_samples; j++)
            printf("%s\n", j == i ? cpu_graphics[j] : "");
    }
    if (graphics){
        for (int j = 0; j < num_samples; j++)
            printf("%s\n", cpu_graphics[j]);
    }
}
void show_user_usage(char *users_usage){
    printf("---------------------------------------\n");
    printf("### Sessions/users ###\n");
    printf("%s\n", users_usage);
}
//This function retrieves and prints various system information like name, machine name, version, release, and architecture using the uname() system call.
void system_information() {
    struct utsname *system_info = malloc(sizeof(struct utsname));
    uname(system_info);
    printf("---------------------------------------\n");
    printf("### System Information ###\n");
    printf("System Name = %s\n", system_info->sysname);
    printf("Machine Name = %s\n", system_info->nodename);
    printf("Version = %s\n", system_info->version);
    printf("Release = %s\n", system_info->release);
    printf("Architecture = %s\n", system_info->machine);
    free(system_info);
}

void uptime() {

    double uptime;
    char buff[256];
    char output[256];

    FILE *fd = fopen("/proc/uptime", "r");
    /*
      /proc/uptime holds two numbers
      The first one is the total number of seconds the system has been up.
      The second one is how much of that time the machine has spent idle
      We want to deal with the first number only
    */
    fscanf(fd, "%lf", &uptime);
    fclose(fd);

    int days, hours, mins, secs;
    days = (uptime / 86400),
    hours = ((uptime - days * 86400) / 3600),
    mins = ((uptime - days * 86400 - hours * 3600) / 60),
    secs = uptime - (days * 86400) - (hours * 3600) - (mins * 60);

    /* Format output */
    output[0] = '\0';
    if (days) {
        sprintf(output, "%d day", days);
        if (days > 1) {
            strcat(output, "s");
        }
        strcat(output, " ");
    }
    if (hours>=0) {
        sprintf(output+ strlen(output), "%d:", hours);
    }
    if (mins) {
        sprintf(output+ strlen(output), "%d:", mins);
    }
    if (secs) {
        sprintf(output+ strlen(output), "%d", secs);
    }
    //System running since last reboot: 3 days 19:27:30 (91:27:30)
    sprintf(output, "%s (%d:%d:%d)",output,days*24+hours,mins,secs);
    printf("System running since last reboot:%s\n", output);
}

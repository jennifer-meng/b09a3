#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utmp.h> //user
#include <sys/utsname.h> //for system info
#include <unistd.h>
#include <sys/resource.h> //rusage for memory
#include <sys/sysinfo.h> //for memory
#include <math.h>
typedef struct cpu_info {
    //user: normal processes executing in user mode
    //nice: niced processes executing in user mode
    //system: processes executing in kernel mode
    //idle: twiddling thumbs
    //iowait: waiting for I/O to complete
    //irq: servicing interrupts
    //softirq: servicing softirqs
    long unsigned utime, ntime, stime, itime;
    long unsigned iowtime, irqtime, sirqtime;
} CPUINFO;
/* char mem_graphics[][128]: A 2D character array to store memory usage graphics.
int i_sample: The current sample index for storing the memory data.
int graphics: A flag indicating whether graphical representation should be generated (1 for yes, 0 for no).
double *prev_used: Pointer to a double that stores the previous physical memory usage value.
This function calculates and prints the physical and virtual memory usage of the system, updates the mem_graphics array accordingly, and updates the prev_used variable with the latest physical memory usage.
show_memory function:*/
void memory_usage(char mem_graphics[][128], int i_sample, int graphics, double *prev_used) {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0) return;

    printf("Memory usage: %ld kilobytes\n", usage.ru_maxrss);
    printf("---------------------------------------\n");
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
    //store them in array
    sprintf(mem_graphics[i_sample], "%.2f GB / %.2f GB  -- %.2f GB / %.2f GB",
            phys_used, phys_tol, vir_used, vir_tol);

    if (graphics) {
        char *rest = mem_graphics[i_sample] + strlen(mem_graphics[i_sample]);
        // sprintf()

        if (i_sample == 0)
            sprintf(rest, "   |o 0.00 (%.2f)", phys_used);
        else {
            diff = phys_used - *prev_used;

            sym1 = (diff >= 0) ? "#" : ":";
            sym2 = (diff >= 0) ? "*" : "@";

            char tmp[128] = {'\0'};
            //add the graphics in tmp
            for (double d = 0.0; d < fabs(diff); d += 0.01)
                strcat(tmp, sym1);
            strcat(tmp, sym2);

            sprintf(rest, "   |%s %.2f (%.2f)", tmp, diff, phys_used);
        }
    }
    *prev_used = phys_used;
}
//int tdelay: Time delay between samples (not used in this function).
//int num_samples: The total number of memory samples.
//char mem_graphics[num_samples][128]: The 2D array containing memory usage information.
//This function prints the memory usage statistics stored in the mem_graphics array.
void show_memory(int tdelay, int num_samples, char mem_graphics[num_samples][128]) {
    printf("### Memory ### (Phys.Used/Tot -- Virtual Used/Tot\n");

    for (int j = 0; j < num_samples; j++) {
        printf("%s\n", mem_graphics[j]);
    }
}

//calculation process, o refers to the previous cpuinfo, n refers to the current cpuinfo
//CPUINFO *o: Pointer to a CPUINFO structure containing the previous CPU stats.
//CPUINFO *n: Pointer to a CPUINFO structure containing the current CPU stats.
//Return Value: Returns a float representing the percentage of CPU usage.
//This function calculates the CPU usage based on the difference between old and new CPU stat readings.
float cal_cpuoccupy(CPUINFO *o, CPUINFO *n) {
    unsigned long od, nd;
    unsigned long id, sd;
    float cpu_use = 0;
    od = (unsigned long) (o->utime + o->ntime + o->stime + o->itime
                          + o->iowtime + o->irqtime + o->sirqtime);//first time
    nd = (unsigned long) (n->utime + n->ntime + n->stime + n->itime
                          + n->iowtime + n->irqtime + n->sirqtime);//second time
    id = (unsigned long) (n->itime - o->itime);    //idle

    if ((nd - od) != 0) {
        cpu_use = 100 - (float)(id * 100.00) / (nd - od);
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
    int n;
    char buff[256];

    fd = fopen("/proc/stat", "r");
    fscanf(fd, "cpu  %lu %lu %lu %lu %lu %lu %lu", &cpuinfo->utime, &cpuinfo->ntime, &cpuinfo->stime,
           &cpuinfo->itime, &cpuinfo->iowtime, &cpuinfo->irqtime, &cpuinfo->sirqtime);
    fclose(fd);
}

//int number_cpus=get_nprocs()?
//int get_cpu_core_count() {
//    FILE *fp;
//    char buffer[128];
//    int cores = 0;
//    int prev_core = -1;
//    fp = fopen("/proc/cpuinfo", "r");
//    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
//        if (strncmp(buffer, "processor", strlen("processor")) == 0) {
//            int core;
//            sscanf(buffer, "processor : %d", &core);
//            if (core != prev_core) {
//                cores++;
//                prev_core = core;
//            }
//        }
//    }
//    fclose(fp);
//    return cores;
//}

//char cpu_graphics[][128]: A 2D character array to store CPU usage graphics.
//int i_sample: The current sample index for storing the CPU usage data.
//CPUINFO *pre_cpu_stat: Pointer to a CPUINFO structure containing the previous CPU stats.
//This function calculates the CPU usage, updates the cpu_graphics array, and updates the pre_cpu_stat with the latest CPU stats.
void cpu_usage(char cpu_graphics[][128], int i_sample, CPUINFO *pre_cpu_stat){
    CPUINFO cur_cpu_stat;
    get_cpuinfo((CPUINFO *) &cur_cpu_stat);
    int core_count = get_nprocs(); //get num of cores
    printf("---------------------------------------\n");
    printf("Number of cores: %d\n", core_count);
    float cpu_usage=cal_cpuoccupy(pre_cpu_stat, (CPUINFO *) &cur_cpu_stat);
    printf("total cpu use = %2.2f\% "
           "  \n",cpu_usage);
    memcpy(pre_cpu_stat,&cur_cpu_stat,sizeof(CPUINFO));

    char bars[103] = "|||";
    int number_of_bars=(int)cpu_usage;
    for (int i = 1; i <= number_of_bars; i++)
        strcat(bars, "|");

    sprintf(cpu_graphics[i_sample], "        %s %.2lf", bars, cpu_usage);
}

//int sequential: A flag indicating whether to display the CPU graphic sequentially (1 for yes, 0 for no).
//int i: The current sample index.
//int num_samples: The total number of CPU usage samples.
//const char cpu_graphics[][128]: The 2D array containing CPU usage graphics.
//This function displays the CPU usage graphics stored in the cpu_graphics array based on the sequential flag.
void show_cpu_graphic(int sequential, int i,int num_samples, const char cpu_graphics[][128]) {

    if (sequential) {
        for (int j = 0; j < num_samples; j++)
            printf("%s\n", j == i ? cpu_graphics[j] : "");
    } else {
        for (int j = 0; j < num_samples; j++)
            printf("%s\n", cpu_graphics[j]);
    }
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

//System running since last reboot:
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
        sprintf(output, "%s%d:", output, hours);
    }
    if (mins) {
        sprintf(output, "%s%d:", output, mins);
    }
    if (secs) {
        sprintf(output, "%s%d", output, secs);
    }
    //System running since last reboot: 3 days 19:27:30 (91:27:30)
    sprintf(output, "%s (%d:%d:%d)",output,days*24+hours,mins,secs);
    printf("System running since last reboot:%s\n", output);
}

typedef struct {
//    char username[USERNAME_LEN];
    struct utmp utmp;
} User;

//int calculate_count(User *users,  int user_count, int position) {
//    int total_count=0;
//    for (int i = 0; i < user_count; i++) {
//        if ( strcmp(users[i].utmp.ut_user, users[position].utmp.ut_user) == 0) {
//            total_count++;
//        }
//    }
//    return total_count;
//}

//int calculate_index(User *users,  int position) {
//    int index=0;
//    for (int i = 0; i < position; i++) {
//        if ( strcmp(users[i].utmp.ut_user, users[position].utmp.ut_user) == 0) {
//            index++;
//        }
//    }
//    return index;
//}

//This function prints a list of active user sessions by parsing the contents of the utmp file.
void get_user_usage() {
    const int MAX_USERS=100;
    printf("---------------------------------------\n");
    printf("### Sessions/users ###\n");
    setutent();
    struct utmp *user = getutent();
    User users[MAX_USERS];
    int user_count = 0; // current user amount
    while (user != NULL) {
        if (user->ut_type == USER_PROCESS) {
            printf(" %s\t%s (%s)\n", user->ut_user, user->ut_line, user->ut_host);
            // memcpy(&users[user_count++].utmp,user,sizeof(struct utmp));
        }
        user = getutent();
    }
    // Close the utmp file
    endutent();
//    for (int i=0;i<user_count;i++)
//    {
//        int total_count=calculate_count(users,  user_count, i) ;
//        if (total_count==1)
//        {
//            printf(" %s\t%s (%s)\n", users[i].utmp.ut_user, users[i].utmp.ut_line, users[i].utmp.ut_host);
//        } else
//        {
//            int index=calculate_index(users,i) ;
//            printf(" %s\t%s/%d (%s)\n", users[i].utmp.ut_user, users[i].utmp.ut_line, index,users[i].utmp.ut_host);
//        }
//    }
}

int main(int argc, char **argv) {
    int sequential = 0;
    int tdelay = 1;
    int graphical = 0;
    int user_only = 0;
    int system_only = 0;
    int num_samples = 10;
    int set_num_sample=0;
    int set_tdelay= 0;

    //what command
    for (int i = 1; i < argc; i++) {
        if (strstr(argv[i], "--samples=") != NULL){
            set_num_sample =1;
            num_samples = strtol(argv[i] + 10, NULL, 10);
        }
        if (strstr(argv[i], "--user") != NULL) { user_only = 1; }
        if (strstr(argv[i], "--system") != NULL) {
            system_only = 1;
        }
        if (strstr(argv[i], "--sequential") != NULL) {
            sequential = 1;
        }
        if (strstr(argv[i], "--graphics") != NULL) {
            graphical = 1;
        }
        if (strstr(argv[i], "--tdelay=") != NULL) {
            set_tdelay=1;
            tdelay = strtol(argv[i] + 9, NULL, 10);
        }

    }
    if (!set_num_sample &&argc>1)
    {
        int num;
        int items_read = sscanf(argv[1], "%d", &num);
        if (items_read==1)
        {
            num_samples =num;
        }
    }
    if (!set_tdelay && argc>2){
        int num;
        int items_read = sscanf(argv[2], "%d", &num);
        if (items_read==1)
        {
            tdelay =num;
        }
    }
    char cpu_graphics[num_samples][128];
    char mem_graphics[num_samples][128];
    for (int i = 0; i < num_samples; i++) {
        cpu_graphics[i][0] = '\0';
        mem_graphics[i][0] = '\0';
    }

    CPUINFO pre_cpu_stat;
    double prev_used = 0.0;


    get_cpuinfo((CPUINFO *) &pre_cpu_stat);

    printf("\033[2J");//clear
    printf("\033[1;1H"); //move cursor to line1 1
    fflush(stdout);
    printf("Nbr of samples: %d -- every %d secs\n", num_samples, tdelay);
    for (int i = 0; i < num_samples; i++) {
        if (sequential) {
            printf(">>> Iteration %d\n", i);
        } else
        {
//            printf("\033[2J\033[1;1H");
            printf("\033[2;1H"); //move cursor to line2 1
            fflush(stdout);
        }

        sleep(tdelay);
        if(!user_only) {
            memory_usage(mem_graphics, i, graphical, &prev_used);
            show_memory(tdelay, num_samples, mem_graphics);
        }
        //if not show only user
        if (!system_only) {
            get_user_usage();
        }
        if (user_only & system_only)
        {
            memory_usage(mem_graphics, i, graphical, &prev_used);
            show_memory(tdelay, num_samples, mem_graphics);
            get_user_usage();
            cpu_usage(cpu_graphics,i,&pre_cpu_stat);
            if (graphical)
                show_cpu_graphic(sequential, i, num_samples, cpu_graphics);
        }
        if (user_only) {
            continue;
        }
        cpu_usage(cpu_graphics,i,&pre_cpu_stat);
        if (graphical)
            show_cpu_graphic(sequential, i, num_samples, cpu_graphics);


    }
    system_information();
    uptime();
    printf("---------------------------------------\n");
}




//
// Created by jennifer on 28/01/24.
//

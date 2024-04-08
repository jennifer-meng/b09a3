//
// Created by guiwen on 2024/4/8.
//

#ifndef B09_STATS_FUNCTIONS_H
#define B09_STATS_FUNCTIONS_H
#define BUF_LEN 256
#define DOUBLE_BUF_LEN BUF_LEN*2+1
//extern char *get_user_usage();
//
//extern char *WTF_cpu(long *prev_idle, long *prev_total) ;
//extern char *VC_memory(int include_graphics, double *prev_used) ;
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
float cal_cpuoccupy(CPUINFO *o, CPUINFO *n) ;
void get_cpuinfo(CPUINFO *cpuinfo);
void get_cpu_usage(char cpu_graphics[][BUF_LEN],char* cpu_usages, int i_sample, int graphics, CPUINFO *pre_cpu_stat);
void get_memory_usage(char *kilobytes, char mem_graphics[][BUF_LEN], int i_sample, int graphics, double *prev_used) ;
char * get_user_usage() ;
void show_memory(char *kilobyates,int num_samples, char mem_graphics[num_samples][BUF_LEN]) ;
void show_cpu(int sequential, int i,int num_samples, int graphics, char cpu_usages[BUF_LEN], const char cpu_graphics[][BUF_LEN]) ;
void show_user_usage(char users_usage[BUF_LEN]);
void system_information() ;
void uptime() ;
#endif //B09_STATS_FUNCTIONS_H

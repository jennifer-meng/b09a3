#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "stats_functions.h"
pid_t pid_storage(pid_t pid, int flag, int action) {
    static pid_t cpu_p = -1;
    static pid_t mem_p = -1;
    static pid_t usr_p = -1;

    // store
    if (action == 1) {
        if (flag == 0) cpu_p = pid;
        if (flag == 1) mem_p = pid;
        if (flag == 2) usr_p = pid;
    }

        // retrieve
    else {
        if (flag == 0) return cpu_p;
        if (flag == 1) return mem_p;
        if (flag == 2) return usr_p;
    }

    return -1;
}

void handler(int sig) {
    pid_t cpu_p = pid_storage(-1, 0, 0);
    pid_t mem_p = pid_storage(-1, 1, 0);
    pid_t usr_p = pid_storage(-1, 2, 0);
    if(sig==SIGTSTP){
        if (cpu_p != -1) kill(cpu_p, SIGCONT);
        if (mem_p != -1) kill(mem_p, SIGCONT);
        if (usr_p != -1) kill(usr_p, SIGCONT);
        return;
    }
    if (cpu_p != -1) kill(cpu_p, SIGSTOP);
    if (mem_p != -1) kill(mem_p, SIGSTOP);
    if (usr_p != -1) kill(usr_p, SIGSTOP);

    write(STDOUT_FILENO, "Do you want to quit? (yes/no): ", 31);

    char buf[30] = {0}; // Initialize buffer to zero
    ssize_t bytes_read = read(STDIN_FILENO, buf, sizeof(buf) - 1);
    if (bytes_read > 0) {
        buf[bytes_read - 1] = '\0'; // Replace newline with null terminator
        if (strcmp(buf, "yes") == 0) {
            if (cpu_p != -1) kill(cpu_p, SIGTERM);
            if (mem_p != -1) kill(mem_p, SIGTERM);
            if (usr_p != -1) kill(usr_p, SIGTERM);
            waitpid(cpu_p,NULL,0);
            waitpid(mem_p,NULL,0);
            waitpid(usr_p,NULL,0);
            _exit(EXIT_SUCCESS); // Use _exit in signal handler
        } else {
            if (cpu_p != -1) kill(cpu_p, SIGCONT);
            if (mem_p != -1) kill(mem_p, SIGCONT);
            if (usr_p != -1) kill(usr_p, SIGCONT);
        }
    }
}


int main(int argc, char **argv) {
    // defualt arguments
    int sequential = 0;
    int tdelay = 1;
    int graphical = 0;
    int user_only = 0;
    int system_only = 0;
    int num_samples = 10;
    int set_num_sample = 0;
    int set_tdelay = 0;

    // process arguments
    for (int i = 1; i < argc; i++) {
        if (strstr(argv[i], "--samples=") != NULL) {
            set_num_sample = 1;
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
            set_tdelay = 1;
            tdelay = strtol(argv[i] + 9, NULL, 10);
        }

    }
    if (!set_num_sample && argc > 1) {
        int num;
        int items_read = sscanf(argv[1], "%d", &num);
        if (items_read == 1) {
            num_samples = num;
        }
    }
    if (!set_tdelay && argc > 2) {
        int num;
        int items_read = sscanf(argv[2], "%d", &num);
        if (items_read == 1) {
            tdelay = num;
        }
    }
//    int cpu_i = 0, mem_i = 0;
    char cpu_graphics[num_samples][BUF_LEN];
    char mem_graphics[num_samples][BUF_LEN];
    char kilobytes[BUF_LEN]={0};
    char cpu_usages[BUF_LEN]={0};

    for (int i = 0; i < num_samples; i++) {
        cpu_graphics[i][0] = '\0';
        mem_graphics[i][0] = '\0';
    }

    //processor for cpu calculation
    int cpu_fd[2];
    pipe(cpu_fd);
    pid_t cpu_p = fork();
    if (cpu_p == -1) {}
    else if (cpu_p == 0) {
        // Child Process to run CPU
        close(cpu_fd[0]); // close read-end
        CPUINFO pre_cpu_stat;
        get_cpuinfo((CPUINFO *) &pre_cpu_stat);
        char ret[1024];
        long prev_idle, prev_total;
        for (int i = 0; i < num_samples; i++) {
            get_cpu_usage(cpu_graphics, cpu_usages, i, graphical, &pre_cpu_stat);
            sprintf(ret, "%s$%s", cpu_usages, cpu_graphics[i]);
            // TODO: 可以考慮用while 一個一個 chunk 寫過去
            write(cpu_fd[1], ret, strlen(ret) + 1);
            sleep(tdelay);
        }
        close(cpu_fd[1]);
        exit(0);
    } else {
        // Parent
        close(cpu_fd[1]);
        pid_storage(cpu_p, 0, 1);
    }

    //processor for memory usage
    int mem_fd[2];
    pipe(mem_fd);

    pid_t mem_p = fork();
    if (mem_p == -1) {}
    else if (mem_p == 0) {
        // Child Process to run MEMORY
        close(mem_fd[0]); // close read-end
        close(cpu_fd[0]); // Parent 沒有關, 不小心被copy過來了

        char ret[DOUBLE_BUF_LEN];
        double prev_used = 0.0;
        for (int i = 0; i < num_samples; i++) {
            //need to add return type to get_memory_usage
            get_memory_usage(kilobytes, mem_graphics, i, graphical, &prev_used);
            sprintf(ret,"%s$%s",kilobytes, mem_graphics[i]);
            write(mem_fd[1], ret, strlen(ret) + 1);
            sleep(tdelay);
        }
        close(mem_fd[1]);
        exit(0);
    } else {
        // Parent
        close(mem_fd[1]);
        pid_storage(mem_p, 1, 1);
    }

    //processor for current user
    int usr_fd[2];
    pipe(usr_fd);

    pid_t usr_p = fork();
    if (usr_p == -1) {}
    else if (usr_p == 0) {
        // Child Process to run MEMORY
        close(usr_fd[0]); // close read-end
        close(cpu_fd[0]); // close read-end
        close(mem_fd[0]); // close read-end

        for (int i = 0; i < num_samples; i++) {
            char *users_usage= get_user_usage();
            write(usr_fd[1], users_usage, strlen(users_usage) + 1);
            free(users_usage);
            sleep(tdelay);
        }
        close(usr_fd[1]);
        exit(0);
    } else {
        // Parent
        close(usr_fd[1]);
        pid_storage(usr_p, 2, 1);
    }

    // ==============================
    struct sigaction sa;
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask); // sigemptyset();
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL); // Ctrl C
    sigaction(SIGTSTP, &sa, NULL); // Ctrl Z
    // ==============================


    // Only Parent gets here !!!!!!!!!!!!!!!!!!
    char *user_info;
    char *tmp;
    int chunk_size = 1024;
    char buf[chunk_size];
    int num_read;
    int data_size;
    char info_all[1024];

    printf("\033[2J");//clear
    printf("\033[1;1H"); //move cursor to line1 1
    fflush(stdout);
    printf("Nbr of samples: %d -- every %d secs\n", num_samples, tdelay);
    for (int i = 0; i < num_samples; i++) {
        //read(cpu_fd[0], cpu_graphics[cpu_i++], 512);
        read(cpu_fd[0], info_all, 512);
        char *token = strtok(info_all, "$");
        strcpy(cpu_usages, token);
        token = strtok(NULL, "$");
        if (token != NULL)
            strcpy(cpu_graphics[i], token);
        read(mem_fd[0], info_all, 1024);
        token = strtok(info_all, "$");
        strcpy(kilobytes, token);
        token = strtok(NULL, "$");
        strcpy(mem_graphics[i], token);

//        printf("%s\n",mem_graphics[i]);
        user_info = NULL;
        data_size = 0;

        while ((num_read = read(usr_fd[0], buf, chunk_size)) > 0) {
            tmp = realloc(user_info, data_size + num_read);
            if (tmp == NULL) {
                perror("realloc");
                free(user_info);
                return 1;
            }
            user_info = tmp;
            memcpy(user_info + data_size, buf, num_read);
            data_size += num_read;

            // we have read all the data :)
            if (user_info[data_size - 1] == '\0') break;
        }
        if (sequential) {
            printf(">>> Iteration %d\n", i);
        } else {
            printf("\033[2;1H"); //move cursor to line2 1
            fflush(stdout);
        }
        if (!user_only) {
            show_memory(kilobytes, num_samples, mem_graphics);
        }
        //if not show only user
        if (!system_only) {
            show_user_usage(user_info);
            free(user_info);
        }
        if (user_only & system_only) {
            show_memory(kilobytes, num_samples, mem_graphics);
            show_user_usage(user_info);
            free(user_info);
            continue;
        }
        if (user_only) {
            continue;
        }
        show_cpu(sequential, i, num_samples, graphical, cpu_usages, cpu_graphics);
    }
    system_information();
    uptime();
    printf("---------------------------------------\n");
}
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "stats_functions.h"
#include <sys/signalfd.h>
#include <fcntl.h>
#include <errno.h>


char *get_user_info(const int *usr_fd);

int get_sfd();

void clearup(pid_t cpu_p, pid_t mem_p, pid_t usr_p, pid_t show_p);

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
    char kilobytes[BUF_LEN] = {0};
    char cpu_usages[BUF_LEN] = {0};

    for (int i = 0; i < num_samples; i++) {
        cpu_graphics[i][0] = '\0';
        mem_graphics[i][0] = '\0';
    }
    //processor for cpu calculation
    int cpu_fd[2];
    int mem_fd[2];
    int usr_fd[2];
    int com_fd[2]; // communicate with main process when the all the tasks are finished.
    pipe(cpu_fd);
    pid_t cpu_p = fork();
    if (cpu_p == -1) {}
    else if (cpu_p == 0) {
        signal(SIGINT, SIG_IGN); // ignore ctl-c in subprocess
        signal(SIGTSTP, SIG_IGN); // ignore ctl-z in subprocess
        // Child Process to run CPU
        close(cpu_fd[0]); // close read-end
        CPUINFO pre_cpu_stat;
        get_cpuinfo((CPUINFO *) &pre_cpu_stat);
        char ret[1024];
        long prev_idle, prev_total;
        for (int i = 0; i < num_samples; i++) {
            get_cpu_usage(cpu_graphics, cpu_usages, i, graphical, &pre_cpu_stat);
            sprintf(ret, "%s$%s", cpu_usages, cpu_graphics[i]);
            write(cpu_fd[1], ret, strlen(ret) + 1);
            sleep(tdelay);
        }
        close(cpu_fd[1]);
        exit(0);
    } else {
        // Parent
        close(cpu_fd[1]);
    }
    //processor for memory usage
    pipe(mem_fd);
    pid_t mem_p = fork();
    if (mem_p == -1) {}
    else if (mem_p == 0) {
        signal(SIGINT, SIG_IGN); // ignore ctl-c in subprocess
        signal(SIGTSTP, SIG_IGN); // ignore ctl-z in subprocess
        // Child Process to run MEMORY
        close(mem_fd[0]); // close read-end
        close(cpu_fd[0]); // Parent

        char ret[DOUBLE_BUF_LEN];
        double prev_used = 0.0;
        for (int i = 0; i < num_samples; i++) {
            //need to add return type to get_memory_usage
            get_memory_usage(kilobytes, mem_graphics, i, graphical, &prev_used);
            sprintf(ret, "%s$%s", kilobytes, mem_graphics[i]);
            write(mem_fd[1], ret, strlen(ret) + 1);
            sleep(tdelay);
        }
        close(mem_fd[1]);
        exit(0);
    } else {
        // Parent
        close(mem_fd[1]);
    }

    //processor for current user

    pipe(usr_fd);
    pid_t usr_p = fork();
    if (usr_p == -1) {}
    else if (usr_p == 0) {
        signal(SIGINT, SIG_IGN); // ignore ctl-c in subprocess
        signal(SIGTSTP, SIG_IGN); // ignore ctl-z in subprocess
        // Child Process to run MEMORY
        close(usr_fd[0]); // close read-end
        close(cpu_fd[0]); // close read-end
        close(mem_fd[0]); // close read-end

        for (int i = 0; i < num_samples; i++) {
            char *users_usage = get_user_usage();
            write(usr_fd[1], users_usage, strlen(users_usage) + 1);
            free(users_usage);
            sleep(tdelay);
        }
        close(usr_fd[1]);
        exit(0);
    }
    if (pipe(com_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // make the pipe none block
    if (fcntl(com_fd[0], F_SETFL, O_NONBLOCK) == -1 || fcntl(com_fd[1], F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    pid_t show_p = fork();
    if (show_p == -1) {}
    else if (show_p == 0) {
        signal(SIGINT, SIG_IGN); // ignore ctl-c in subprocess
        signal(SIGTSTP, SIG_IGN); // ignore ctl-z in subprocess
        close(usr_fd[1]);
        close(com_fd[0]);
        printf("\033[2J");//clear
        printf("\033[1;1H"); //move cursor to line1 1
        fflush(stdout);
        printf("Nbr of samples: %d -- every %d secs\n", num_samples, tdelay);
        for (int i = 0; i < num_samples; i++) {
            char info_all[1024];
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
            char *user_info = get_user_info(usr_fd);
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

        char message = 'E'; // 发送结束消息
        write(com_fd[1], &message, sizeof(char)); // 向父进程发送消息
        close(com_fd[1]); // 关闭写端
        exit(EXIT_SUCCESS);
    }


    struct signalfd_siginfo fdsi;
    int sfd = get_sfd();
    char message;
    close(com_fd[1]);
    // check if the display process whether is over
    while (read(com_fd[0], &message, sizeof(char)) <= 0) {
        ssize_t s;
        //read ctrl-c and ctrl-z
        s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
        if (s == -1) {
            if (errno == EAGAIN) {
                // no signal  arrived
                usleep(100);
                continue;
            } else {
                // 发生错误
                perror("read");
                exit(EXIT_FAILURE);
            }
        }

        if (s == sizeof(struct signalfd_siginfo)) {
            // 处理信号
            if (fdsi.ssi_signo == SIGINT) {
                char response[32];
                printf("Received SIGINT. Do you want to exit? (yes/no): ");
                fgets(response, sizeof(response), stdin);
                response[strcspn(response, "\n")] = '\0'; // 去除换行符
                if (strcmp(response, "yes") == 0) {
                    printf("yes\n");
                    break;

                }
            } else {
                printf("Received ctrl-z signal,ignore\n");
            }
        }

    }
    clearup(cpu_p, mem_p, usr_p, show_p);

}

void clearup(pid_t cpu_p, pid_t mem_p, pid_t usr_p, pid_t show_p) {
    kill(cpu_p, SIGTERM); // kill sub process
    kill(mem_p, SIGTERM); // kill sub process
    kill(usr_p, SIGTERM); // kill sub process
    kill(show_p, SIGTERM); // kill sub process
    int status;
    waitpid(cpu_p,&status,0);
    waitpid(mem_p,&status,0);
    waitpid(usr_p,&status,0);
    waitpid(show_p,&status,0);
}

int get_sfd() {
    int sfd;
    sigset_t mask;
    ssize_t s;

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) {
        perror("signalfd");
        exit(EXIT_FAILURE);
    }
    int flags = fcntl(sfd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl get");
        exit(EXIT_FAILURE);
    }

    if (fcntl(sfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl set");
        exit(EXIT_FAILURE);
    }
    return sfd;
}

char *get_user_info(const int *usr_fd) {
    char *user_info;
    char *tmp;
    int chunk_size = 1024;
    char buf[chunk_size];
    int num_read;
    int data_size;
    user_info = NULL;
    data_size = 0;

    while ((num_read = read(usr_fd[0], buf, chunk_size)) > 0) {
        tmp = realloc(user_info, data_size + num_read);
        if (tmp == NULL) {
            perror("realloc");
            free(user_info);
            _exit(1);
        }
        user_info = tmp;
        memcpy(user_info + data_size, buf, num_read);
        data_size += num_read;

        // we have read all the data :)
        if (user_info[data_size - 1] == '\0') break;
    }
    return user_info;
}

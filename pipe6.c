#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <string.h>
int main(void) {
    int pipefd[2];
    pid_t cpid;
    char buf;
    int status;

    if (pipe(pipefd) == -1) { // 创建管道
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    cpid = fork(); // 创建子进程
    if (cpid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0) {    // 子进程
        close(pipefd[0]); // 关闭读端
        signal(SIGINT, SIG_IGN); // 设置信号处理函数
        while (1) {
            sleep(1); // 每隔1秒发送一次消息
            int ret=write(pipefd[1], "hello", 5); // 写入"hello"
            if (ret<0)
            {
                printf("error happen!");
            }
        }
    }
    pid_t receive_pid = fork(); // 创建子进程
    if (receive_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (receive_pid==0)
    {
        signal(SIGINT, SIG_IGN); // 设置信号处理函数
        close(pipefd[1]); // 关闭写端
        while (1) {
            read(pipefd[0], &buf, 1); // 读取管道数据
                printf("from here\n");
                printf("%c", buf); // 打印字符
                fflush(stdout); // 刷新输出，确保立即显示

        }

    }
    sigset_t mask;
    int sfd;
    struct signalfd_siginfo fdsi;
    ssize_t s;

    // 阻塞信号
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    // 创建 signalfd
    sfd = signalfd(-1, &mask, 0);
    if (sfd == -1) {
        perror("signalfd");
        exit(EXIT_FAILURE);
    }

    // 循环等待信号
    for (;;) {
        s = read(sfd, &fdsi, sizeof(struct signalfd_siginfo));
        if (s == -1) {
            if (errno == EAGAIN) {
                // 没有信号到达，继续循环
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

    kill(cpid, SIGTERM); // 杀死子进程
    kill(receive_pid, SIGTERM); // 杀死子进程
    waitpid(cpid, &status, 0); // 等待子进程结束
    waitpid(receive_pid, &status, 0); // 等待子进程结束
    close(sfd);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int fd[2]; // 用于管道的文件描述符
    pid_t pid;
    char message;

    // 创建管道
    if (pipe(fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // 设置管道为非阻塞模式
    if (fcntl(fd[0], F_SETFL, O_NONBLOCK) == -1 || fcntl(fd[1], F_SETFL, O_NONBLOCK) == -1) {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    // 创建子进程
    pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // 子进程
        close(fd[0]); // 关闭读端
        printf("Child process: Running...\n");
        sleep(5); // 模拟工作
        printf("Child process: Ending and notifying parent...\n");
        message = 'E'; // 发送结束消息
        write(fd[1], &message, sizeof(char)); // 向父进程发送消息
        close(fd[1]); // 关闭写端
        exit(EXIT_SUCCESS);
    } else {
        // 父进程
        close(fd[1]); // 关闭写端
        printf("Parent process: Waiting for child process to end...\n");
        while (read(fd[0], &message, sizeof(char)) <= 0) {
            // 循环读取，直到读到数据或出错
            // 这里可以添加一些错误处理和超时逻辑
        }
        printf("Parent process: Child process has ended.\n");
        close(fd[0]); // 关闭读端
        wait(NULL); // 等待子进程结束
    }

    return 0;
}
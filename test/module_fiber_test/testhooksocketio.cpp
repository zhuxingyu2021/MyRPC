#include "fiber/fiberpool.h"
#include "logger.h"
#include <fcntl.h>

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace MyRPC;

#define NUM_THREADS 1


int main()
{
    FiberPool fp(NUM_THREADS);

    int port_server = 9999;
    char ip[16] = "127.0.0.1";

    fp.Start();

    auto client = fp.Run([&ip,port_server](){
        char s[1024];
        memset(s, 0, sizeof(s));

        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0){
            Logger::error("socket error");
            return;
        }

        struct sockaddr_in remote_addr;
        remote_addr.sin_family = AF_INET;
        remote_addr.sin_port = htons(port_server);
        remote_addr.sin_addr.s_addr = inet_addr(ip);
        if(connect(sockfd, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0){
            Logger::error("connect error");
            return;
        }
        Logger::info("connect success");
        while(true){
            int read_cnt = 0;
            while(true) {
                int input_size = read(STDIN_FILENO, &s[read_cnt], sizeof(s)-read_cnt);
                while (input_size > 0) {
                    if (s[read_cnt] == '\n') break;
                    read_cnt++;
                    input_size--;
                }
                if(s[read_cnt] == '\n') break;
            }
            s[++read_cnt] = '\0';
            send(sockfd, s, read_cnt, 0);
        }
    }, 0);

    auto server = fp.Run([&ip,port_server](){
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd < 0){
            Logger::error("socket error");
            return;
        }
        struct sockaddr_in local_addr;
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(port_server);
        local_addr.sin_addr.s_addr = inet_addr(ip);
        if(bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0){
            Logger::error("bind error");
            return;
        }
        if(listen(sockfd, 5) < 0){
            Logger::error("listen error");
            return;
        }
        struct sockaddr_in remote_addr;
        socklen_t len = sizeof(remote_addr);
        int new_sockfd = accept(sockfd, (struct sockaddr*)&remote_addr, &len);
        if(new_sockfd < 0){
            Logger::error("accept error");
            return;
        }
        Logger::info("accept success");
        char buf[1024];
        memset(buf, 0, sizeof(buf));
        while(true){
            int input_size = recv(new_sockfd, buf, sizeof(buf), 0);
            write(STDOUT_FILENO, buf, input_size);
        }
    }, 0);

    client->Join();
    server->Join();

    fp.Stop();
}


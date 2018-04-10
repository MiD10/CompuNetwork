#ifndef CLIENT_SERVER_H
#define CLIENT_SERVER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <vector>
#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <mutex>

#define SERVER_PORT 2161
#define BUF_SIZE 4096
#define QUEUE_SIZE 10

#define UNCONNECTED 0
#define CONNECTED 1

#define REQUEST 666
#define RESPONSE 777
#define INSTRUCTION 888

void fatal(char* string){
    printf("%s\n",string);
    exit(1);
}

struct message{
  int size;
  int type;
  char msg[BUF_SIZE]; //fix the size to avoid complexity in sending message
};

typedef struct client_info* pclient;
struct client_info{
    int sa;
    char IP[10];
    int port;
    int flag;
    std::thread* t;
    pclient next;
};


void sender(int num, int type, std::string word){
    //not done yet: automatically break up into frames if the size exceeds the the size of BUF_SIZE
    int sent, nleft;
    void* ptemp;
    struct message temp;
    strcpy(temp.msg, word.c_str());
    temp.size = word.size();
    temp.type = type;
    ptemp = &temp;
    nleft = sizeof(temp);
    std::cout << "ready to send message!" << std::endl;
    while(nleft > 0) {
        sent = send(num, ptemp, nleft, 0);
        if (sent < 1) {
            std::cout << "can not send msg:" << temp.msg << std::endl;
            break;
        }
        printf("sent: %d\n", sent);
        nleft -= sent;
        ptemp = (char *) ptemp + sent;
    }
    return;
}

class client{
private:
  int c, s;
  std::thread* pt;
  std::vector<std::thread> threads; //thread(const thread&) = delete
  char server_address[50];
  int port_number;
  struct hostent *h;
  struct sockaddr_in channel;

  void initialize(void);
  void main_connect(void);
  void main_close(void);
  void main_time(void);
  void main_name(void);
  void main_list(void);
  void main_send(void);
public:
  void main_process(void);
  int state;
};

class server{
private:
  int s, b, l, fd, sa, on = 1;
  char buf[BUF_SIZE];
  void initialize(void);
public:
  void main_process(void);
  pclient head, tail;
  int client_num;
  //std::vector<client_info> clients;
  std::vector<std::thread> threads; //thread(const thread&) = delete
  //std::vector<int> flags;
  struct sockaddr_in channel;
  bool flag;
  std::mutex list_mtx, close_mtx;
};

void server_subprocess(int num_sa, class server* server1);
void client_subprocess(int num_s, class client* client1);
void server_ghost(class server* server1);

#endif
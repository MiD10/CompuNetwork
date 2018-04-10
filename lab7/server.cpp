#include <client_server.h>

void server::initialize(void){
    flag = true; head = NULL; client_num = 0;
    s = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if(s < 0){
      fatal("socket failed!");
    }
    return;
}

void server::main_process(void){
    initialize();
    memset(&channel, 0, sizeof(channel));
    channel.sin_family = AF_INET;
    channel.sin_addr.s_addr = htonl(INADDR_ANY);
    channel.sin_port = htons(SERVER_PORT);
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
    b = bind(s, (struct sockaddr*)&channel, sizeof(channel));
    if(b < 0)
      fatal("bind failed!");
    l = listen(s, QUEUE_SIZE);
    if(l < 0)
      fatal("listen failed");
    std::thread ghost;

    ghost = std::thread(server_ghost, this);

    while(flag){
      sa = accept4(s, 0, 0, SOCK_NONBLOCK); //use accpet4 to set the socket to nonblock
      if((sa == -1) && (errno == EAGAIN)){
      }
      else if(sa == -1){
          std::cout << "accept failed" << std::endl;
          return;
      }
      else{
        std::cout << "connection established!" << std::endl;
        list_mtx.lock();
        if(head == NULL){
            pclient pc;
            pc = (pclient)malloc(sizeof(struct client_info));
            head = pc;
            tail = head;
            pc->sa = sa;
            sockaddr_in addrMy;
            memset(&addrMy,0,sizeof(addrMy));
            unsigned int len = sizeof(addrMy);
            getsockname(sa, (sockaddr*)&addrMy, &len);
            strcpy(pc->IP, inet_ntoa(addrMy.sin_addr));
            pc->port = ntohs(addrMy.sin_port);
            pc->flag = 1;
            pc->next = NULL;
            threads.push_back(std::thread(server_subprocess, sa, this));
            pc->t = &threads[threads.size() - 1];
            pc->t->detach();
        }
        else{
            pclient pc;
            pc = (pclient)malloc(sizeof(struct client_info));
            pc->sa = sa;
            sockaddr_in addrMy;
            memset(&addrMy,0,sizeof(addrMy));
            unsigned int len = sizeof(addrMy);
            getsockname(sa, (sockaddr*)&addrMy, &len);
            strcpy(pc->IP, inet_ntoa(addrMy.sin_addr));
            pc->port = ntohs(addrMy.sin_port);
            pc->next = NULL;
            tail->next = pc;
            pc->flag = 1;
            tail = pc;
            threads.push_back(std::thread(server_subprocess, sa, this));
            pc->t = &threads[threads.size() - 1];
            pc->t->detach();
        }
        list_mtx.unlock();
      }
    }
    pclient p = head;
    pclient pp;
    while(p != NULL){
        printf("p->sa = %d, p->flag = %d\n", p->sa, p->flag);
        close_mtx.lock();
        pp = p->next;
        p->flag = 0;
        close_mtx.unlock();
        p = pp;
    }
    close(s);
    ghost.join();
    return;
}

void server_ghost(class server* server1){
    while(1){
        std::string str;
        std::cin >> str;
        if(str == std::string("exit")){
            server1->flag = false;
            return;
        }
        else{
            std::cout << "type: \"exit\" to exit server process" << std::endl;
        }
    }
}

void server_subprocess(int num_sa, class server* server1){
    //for test
    sender(num_sa, INSTRUCTION, "hihi");

    pclient pself;
    pself = server1->head;
    while (pself != NULL) {
        if (num_sa == pself->sa) {
            break;
        }
        pself = pself->next;
    }
    //listen
    while(pself->flag){
        int rec, nleft;
        void* ptemp;
        struct message temp;
        temp.size = 0;
        ptemp = &temp;
        nleft = sizeof(temp);
        while(nleft > 0){
            rec = recv(num_sa, ptemp, nleft, 0);
            if(rec > 0){
                nleft -= rec;
                ptemp = (char*)ptemp + rec;
            }
            else if(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR){
                if(pself->flag == 0){
                    printf("break at loop\n");
                    break;
                }
            }
            else{
                break;
            }
        }
        if(pself->flag == 0)
            continue; //0v0
        if(temp.size && temp.type == REQUEST){
            std::cout << "receive " << temp.msg << " from " << num_sa << std::endl;
            if(std::string(temp.msg) == "close") {
                break;
            }
            else if(std::string(temp.msg) == "time") {
                time_t rawtime;
                time(&rawtime);
                sender(num_sa, RESPONSE, ctime(&rawtime));
                std::cout << "sent time to No. " << num_sa << std::endl;
            }
            else if(std::string(temp.msg) == "name") {
                char tmp[200];
                FILE* pp=popen("hostname", "r");
                fgets(tmp, sizeof(tmp), pp);
                sender(num_sa, RESPONSE, std::string(tmp));
                std::cout << "sent name to No. " << num_sa << std::endl;
            }
            else if(std::string(temp.msg) == "list") {
                pclient p;
                p = server1->head;
                sender(num_sa, RESPONSE, "BEGIN$");
                std::stringstream stream;
                while(p != NULL){
                    stream.clear();
                    stream.str("");
                    stream << "No.";
                    stream << p->sa;
                    stream << " IP:";
                    stream << p->IP;
                    stream << " port:";
                    stream << p->port;
                    std::cout << stream.str() << std::endl;
                    sender(num_sa, RESPONSE, stream.str());
                    p = p->next;
                }
                sender(num_sa, RESPONSE, "END$");
            }
            else if((temp.msg[0] == 's') && (temp.msg[1] == 'e') && (temp.msg[2] == 'n') && (temp.msg[3] == 'd')) {
                std::cout << std::string(temp.msg) << std::endl;
                bool flag_No = true;
                int num = 0;
                int i = 4;
                while(temp.msg[i]){
                    if(temp.msg[i] == '$')
                        break;
                    else if((temp.msg[i] >= '0') && (temp.msg[i] <= '9')){
                        num *= 10;
                        num += (temp.msg[i] - '0');
                    }
                    else{
                        std::cout << "wrong list format!" << std::endl;
                        sender(num_sa, RESPONSE, "wrong sending No. format!");
                        flag_No = false;
                        break;
                    }
                    i++;
                }
                if(flag_No){
                    std::cout << "receive num = " << num << std::endl;
                    bool flag_num = false;
                    pclient p;
                    p = server1->head;
                    while(p != NULL){
                        if(p->sa == num){
                            flag_num = true;
                            std::cout << "find it" << std::endl;
                            break;
                        }
                        p = p->next;
                    }
                    if(flag_num){
                        i++;
                        std::stringstream stream;
                        while(temp.msg[i]){
                            printf("%c\n", temp.msg[i]);
                            stream << temp.msg[i++];
                        }
                        stream << " from ";
                        stream << "No.";
                        stream << pself->sa;
                        stream << " IP:";
                        stream << pself->IP;
                        stream << " port:";
                        stream << pself->port;
                        std::cout << "received message = " << std::string(temp.msg) << std::endl;
                        std::cout << "send message = " << stream.str() << std::endl;
                        sender(num, RESPONSE, stream.str());
                        sender(num_sa, RESPONSE, "sent successfully");
                    }
                    else{
                        sender(num_sa, RESPONSE, "there is no such client");
                    }
                }
            }
            else{
                sender(num_sa, RESPONSE, "can't receive REQUEST message!");
            }
        }
    }
    close(num_sa);
    std::cout << "No." << num_sa <<" is closed!" << std::endl;
    server1->close_mtx.lock();
    server1->list_mtx.lock();
    pclient pc = server1->head;
    if(pc->sa == num_sa){
        server1->head = pc->next;
        free(pc);
        pc = NULL;
        server1->list_mtx.unlock();
        server1->close_mtx.unlock();
        return;
    }
    while(pc != NULL){
        if(pc->next->sa == num_sa){
            pclient ppc = pc->next;
            pc->next = pc->next->next;
            free(ppc);
            ppc = NULL;
            break;
        }
        else {
            pc = pc->next;
        }
    }
    server1->list_mtx.unlock();
    server1->close_mtx.unlock();
}

int main(void){
    server serv;
    serv.main_process();
    return 0;
}
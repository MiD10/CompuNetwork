#include <client_server.h>

void client::initialize(void){
    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(s < 0){
      fatal("socket failed!");
    }
    state = UNCONNECTED;
    return;
}

void client::main_process (void){
    while(1){
        int choice;
        std::cout << "======================================" << std::endl <<
        "[1] connect" << std::endl <<
        "[2] disconnect" << std::endl <<
        "[3] acquire time" << std::endl <<
        "[4] acquire name" << std::endl <<
        "[5] acquire client list" << std::endl <<
        "[6] send message" << std::endl <<
        "[7] exit" << std::endl <<
        "======================================" << std::endl;
        scanf("%d", &choice);
        switch(choice){
	        case 1:
	            if(state == CONNECTED)
	                std::cout << "already connected" << std::endl;
	            else {
                    initialize();
                    main_connect();
                }
	            break;
	        case 2:
	            if(state == CONNECTED)
	                main_close();
	            else
	                std::cout << "not connected yet" << std::endl;
	            break;
	        case 3:
                if(state == CONNECTED)
                    main_time();
                else
                    std::cout << "not connected yet" << std::endl;
	            break;
	        case 4:
                if(state == CONNECTED)
                    main_name();
                else
                    std::cout << "not connected yet" << std::endl;
	            break;
	        case 5:
                if(state == CONNECTED)
                    main_list();
                else
                    std::cout << "not connected yet" << std::endl;
	            break;
	        case 6:
	            if(state == CONNECTED)
	                main_send();
	            else
                    std::cout << "not connected yet" << std::endl;
	            break;
	        case 7:
                if(state == CONNECTED) {
                    main_close();
                    for(int i = 0; i < threads.size(); i++)
                        threads[i].join();
                    return;
                }
                else
                    for(int i = 0; i < threads.size(); i++)
                        threads[i].join();
                    return;
	        default:
	            std::cout << "wrong choice, enter 1~7 to select a operation" << std::endl;
	            break;
      }
    }
}

void client::main_connect (void){
    std::cout << "enter: server_address port" << std::endl;
    scanf("%s %d", server_address, &port_number);
    h = gethostbyname(server_address);
    if(!h)
      fatal("gethostbyname failed");
    printf("after get host\n");
    memset(&channel, 0, sizeof(channel));
    channel.sin_family = AF_INET;
    memcpy(&channel.sin_addr.s_addr, h->h_addr, h->h_length);
    channel.sin_port = htons(port_number);
    
    c = connect(s, (struct sockaddr*)&channel, sizeof(channel));
    if(c < 0)
      fatal("connect failed");
    printf("after connect\n");
    state = CONNECTED;
    //establish new receiver thread
    threads.push_back(std::thread(client_subprocess, s, this));
    pt = &threads[threads.size() - 1];
    printf("after thread now have %d threads\n", (int)threads.size());
}

void client::main_close(void){
    //ask for termination and wait for the socket to close
    sender(s, REQUEST, "close");
    state = UNCONNECTED;
    return;
}

void client::main_time(void){
    sender(s, REQUEST, "time");
    return;
}

void client::main_name(void){
    sender(s, REQUEST, "name");
    return;
}

void client::main_list(void){
    sender(s, REQUEST, "list");
    return;
}

void client::main_send(void){
    std::cout << "enter No. of the machine" << std::endl;
    int receiver;
    std::cin >> receiver;
    std::stringstream stream;
    stream << "send";
    stream << receiver;
    std::cout << "enter the message you want to send(less than 4096 bytes)" << std::endl;
    std::string temp;
    std::string sndmsg = stream.str();
    getchar();
    getline(std::cin, temp);
    std::cout << "1919:" << temp << std::endl;
    sndmsg += "$";
    sndmsg += temp;
    std::cout << stream.str() << std::endl;
    sender(s, REQUEST, sndmsg);
    return;
}

void client_subprocess(int num_s, class client* client1){
    while(1){
      struct message temp;
      int rec, nleft;
      void* ptemp;
      temp.size = 0;
      ptemp = &temp;
      nleft = sizeof(struct message);
      while(nleft > 0){
          rec = recv(num_s, ptemp, nleft, 0);
          if(rec > 0){
              nleft -= rec;
              ptemp = (char*)ptemp + rec;
          }
          else{
              close(num_s);
              client1->state = UNCONNECTED;
              std::cout << "receiver terminated" << std::endl;
              return;
          }
      }
      if(temp.size){
          if(temp.type == RESPONSE || temp.type == INSTRUCTION){
              if((std::string(temp.msg) == std::string("BEGIN$")) && temp.type == RESPONSE){
                  while(1){
                      struct message ttemp;
                      int trec, tnleft;
                      void* tptemp;
                      ttemp.size = 0;
                      tptemp = &ttemp;
                      tnleft = sizeof(struct message);
                      while(tnleft > 0){
                          trec = recv(num_s, tptemp, tnleft, 0);
                          if(trec > 0){
                              tnleft -= trec;
                              tptemp = (char*)tptemp + trec;
                          }
                          else{
                              close(num_s);
                              client1->state = UNCONNECTED;
                              std::cout << "receiver terminated" << std::endl;
                              return;
                          }
                      }
                      if(std::string(ttemp.msg) == std::string("END$"))
                          break;
                      else{
                          std::cout << ttemp.msg << std::endl;
                      }
                  }
              }
              else{
                  std::cout << temp.msg << std::endl;
              }
          }
      }
      else
          continue;
    }
    //when server terminated, subprocess will jump into the place
}

int main(int argc, char** argv){
    client cli;
    cli.main_process();
}

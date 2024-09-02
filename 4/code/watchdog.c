#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define WATCHDOG_TIMEOUT 10

int main(){

    struct sockaddr_in wdAddress, pingAddress;
    int  pingSocket = -1, bytes_received = 0;

    memset(&pingAddress, 0, sizeof(pingAddress));
    memset(&wdAddress, 0, sizeof(wdAddress));

    wdAddress.sin_family = AF_INET;
    wdAddress.sin_addr.s_addr = INADDR_ANY;
    wdAddress.sin_port = htons(3000);
    int socketfd = -1;
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        return -1;
    }
    int   s = 1;
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(s)) == -1)
    {
        perror("setsockopt");
        return -1;
    }
    //The watchdog listens for a TCP connection and waits for a better_ping connection
    if (bind(socketfd, (struct sockaddr *)&wdAddress, sizeof(wdAddress)) == -1)
    {
         if (errno != EADDRINUSE)
        {
            perror("bind");
            return -1;
         }
    }
    //The watchdog can listen to one connection at a time
    if (listen(socketfd, 1) == -1)
    {
        perror("listen");
        return -1;
    }
    socklen_t pingAddressLen = sizeof(pingAddress);
    //The watchdog receives the better_ping request to connect and the function returns the better_ping details.
    pingSocket = accept(socketfd, (struct sockaddr *) &pingAddress, &pingAddressLen);

    if (pingSocket == -1)
    {
        perror("accept");
        return -1;;
    }
    int timer = 0;
    char NotOk = '\0';
    //while 10 seconds have not passed
    while (timer < WATCHDOG_TIMEOUT)
    {   //Waiting to receive a signal '1' from the better_ping that he received a ping back.
        bytes_received= recv(pingSocket, &NotOk, sizeof(char), MSG_DONTWAIT);
        //If a signal from the better_ping was received we will start calculating the time againץ
        if (bytes_received > 0)
        {
            timer = 0;
        }
        //If the watchdog didn't get any signal ,this means no ping was received back to better_ping
        else if(bytes_received == -1)
        {      
            timer++;
        }
        //Each time sleeps for a second, so we can calculate 10 seconds.
        sleep(1);
    }
    NotOk = '0'; 
    //If we left the loop, it means that the time has passed for the ping to arrive, so we will send a signal that tells better_ping to close the connections.
     if (send(pingSocket, &NotOk, sizeof(char), MSG_DONTWAIT) == -1)
    {
        perror("send");
        return -1;
    }
    close(pingSocket);
    close(socketfd);
    return 1;
}


    
 















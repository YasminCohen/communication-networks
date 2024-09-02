#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

// IPv4 header len without options
#define IP4_HDRLEN 20
// ICMP header len for echo req
#define ICMP_HDRLEN 8
// Checksum algo
unsigned short calculate_checksum(unsigned short *paddress, int len);

#define SOURCE_IP "127.0.0.1"
#define WATCHDOG_PORT       3000
#define WATCHDOG_IP   "127.0.0.1"
#define WATCHDOG_TIMEOUT    10
#define DESTINATION_IP   "8.8.8.8"

int main(int argc, char* argv[]){
int count=1;

    struct icmp icmphdr; // ICMP-header
    char data[IP_MAXPACKET] = "This is the ping.\n"; 

     int datalen = strlen(data) + 1;

    struct sockaddr_in dest_in, watchdogAddress;
    char destaddress[IP4_HDRLEN]; 
    char OKSignal = '1';
    char *args[2]; 
   
    int  socketRaw = -1,status, pid;
    int bytes_sent;
    
    memset(&dest_in, 0, sizeof(struct sockaddr_in)); 
    dest_in.sin_family = AF_INET;

   if(argv[1]!= NULL){
    strcpy(destaddress, argv[1]); 
    printf("conecct to IP: %s \n", destaddress);
    dest_in.sin_addr.s_addr = inet_addr(destaddress); 
   }else{
   dest_in.sin_addr.s_addr = inet_addr(DESTINATION_IP); 
    printf("conecct to IP: 8.8.8.8 \n");
   }
    
    socklen_t dest_inLen = sizeof(dest_in);
    memset(&watchdogAddress, 0, sizeof(watchdogAddress));

    watchdogAddress.sin_family = AF_INET;
    watchdogAddress.sin_port = htons(WATCHDOG_PORT);
    watchdogAddress.sin_addr.s_addr = inet_addr(WATCHDOG_IP);

    //Open a raw socket to communicate with the destination IP
    int wdTcpSocket =-1;
    if ((socketRaw = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
    {
        perror("socketRaw");
        return -1;
    }
    //non blocking socket
    int sta;
    if ((sta = fcntl(socketRaw, F_SETFL, fcntl(socketRaw, F_GETFL, 0)|O_NONBLOCK)) == -1)
    {
        perror("fcntl");
        return -1;
    }
    //Open a RAW socket to send ICMP packets
    if ((wdTcpSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket of wdTcpSocket");
        return -1;
    }
    //A function that runs another program at the same time
    args[0] = "./watchdog";
    pid = fork();
    if (pid == 0)
    {
        execvp(args[0], args);
        perror("execvp");
        return -1;
    }

    else
    {
        usleep(1000000);  
    //Connect with a TCP connection to the watchdog
    if (connect(wdTcpSocket, (struct sockaddr*) &watchdogAddress, sizeof(watchdogAddress)) == -1)
    {
            perror("connect");
            return -1;
    }
    }
    
    while(1){ 
    //===================
    // ICMP header
    //===================
    // Message Type (8 bits): ICMP_ECHO_REQUEST
    icmphdr.icmp_type = ICMP_ECHO;

    // Message Code (8 bits): echo request
    icmphdr.icmp_code = 0;

    // Identifier (16 bits): some number to trace the packet.
    // It will be copied to the packet packet and used to map packet to the request sent earlier.
    // Thus, it serves as a Transaction-ID when we need to make "ping"
    icmphdr.icmp_id = 18;

    // Sequence Number (16 bits): starts at 0
    icmphdr.icmp_seq = count++;

    // ICMP header checksum (16 bits): set to 0 not to include into checksum calculation
    icmphdr.icmp_cksum = 0;

    // Combine the packet
    char packet[IP_MAXPACKET];

    // Next, ICMP header
    memcpy((packet), &icmphdr, ICMP_HDRLEN);

    // After ICMP header, add the ICMP data.
    memcpy(packet + ICMP_HDRLEN, data, datalen);

    // Calculate the ICMP header checksum
    icmphdr.icmp_cksum = calculate_checksum((unsigned short *)(packet), ICMP_HDRLEN + datalen);
    memcpy((packet), &icmphdr, ICMP_HDRLEN);

    struct timeval start, end;
    gettimeofday(&start, NULL);

        ///963852741
    
    // Send the packet using sendto() for sending datagrams.
        if ((bytes_sent = sendto(socketRaw, packet, ICMP_HDRLEN + datalen, 0, (struct sockaddr*)&dest_in, sizeof(dest_in))) == -1)
        {
            perror("send ICMP");
        return -1;
        }
        // Get the ping response
        bzero(packet, IP_MAXPACKET);
        ssize_t bytes_received = -1;

        //while no response is received
        while (!(bytes_received>0))
        {   /// A function that receives the packet
            bytes_received = recvfrom(socketRaw, packet, sizeof(packet), 0, (struct sockaddr *)&dest_in, &dest_inLen);
            // If nothing was received
            if (bytes_received == -1)
            {
                if (errno != EAGAIN)
                {
                    perror("recvfrom");
                    return -1;
                }
                char Signal = '\0';
                //Waiting for the watchdog to send  a sign-this will only happen if 10 seconds pass without receiving a ping
                size_t recvb = recv(wdTcpSocket, &Signal, sizeof(char), MSG_DONTWAIT);
                if (recvb == -1)
                {
                    if (errno != EWOULDBLOCK)
                    {
                        perror("recv");
                        return 1;
                    }
                }
                //If the watchdog sends the signal 0, it means that 10 seconds have passed and therefore should be disconnected. 
                if (Signal == '0')
                {
                        if(argv[1]!= NULL){
                            printf("Server %s cannot be reached.\n", destaddress);
                        }else{
                         printf("Server 8.8.8.8 cannot be reached.\n");
                        }
                    close(wdTcpSocket);
                    close(socketRaw);
                    //Only this command can get out of the loop of sending pings
                    return 1;
                }
            }
            //The ping arrived on time, you can continue in the main loop to send another ping
            else if (bytes_received > 0)

                break;
        } 
            
            gettimeofday(&end, NULL);
            //Calculate the times it took for the packets to arrive
            float milliseconds = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f; 
            unsigned long microseconds = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec);
            // The prints of the requested data
               if(argv[1]!= NULL){
                            printf("Successfuly received one packet  frome IP : %s ", destaddress);
                        }else{
                         printf("Successfuly received one packet  frome IP : 8.8.8.8 ");
                        }
            printf("received one packet with %ld bytes , seq: %d ,time: %f milliseconds and %ld microseconds\n",
            bytes_received, icmphdr.icmp_seq, milliseconds, microseconds);
            //Sends the watchdog that the package has arrived and you can reset the timer.
            ssize_t sentd = send(wdTcpSocket, &OKSignal, sizeof(char), MSG_DONTWAIT);
            if (sentd == -1)
            {
                    perror("send");
                    return -1;
            }
            //sleep 
            usleep(1000000);
    }

    close(wdTcpSocket);
    close(socketRaw);

    wait(&status);
    printf("child exit status is: %d\n", status);

    return 0;
}
// Compute checksum (RFC 1071).
unsigned short calculate_checksum(unsigned short *paddress, int len) {
    int nleft = len, sum = 0;
    unsigned short *w = paddress;
    unsigned short answer = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *((unsigned char *)&answer) = *((unsigned char *)w);
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff); 
    sum += (sum >> 16);                 
    answer = ~sum;                      

    return answer;
}


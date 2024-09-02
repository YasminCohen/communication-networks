#include <sys/types.h> 
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h> 
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h> 
#include <netinet/tcp.h>

#define SERVER_PORT 9999 
#define SERVER_IP_ADDRESS "127.0.0.1"
#define ID1 1242
#define ID2 3836

int main(){
    FILE *fptr; // define pointer to a file type
    fptr = fopen("1mb.txt","r"); // open the file "1mb.txt" , "r" - read
        if (fptr == NULL) 
        {
            perror("Failed open file");
            return 2;
        }
        fseek(fptr,0,SEEK_END); // Move the pointer to the end of the file 
        long fileSize = ftell(fptr); // Size of the file
        fseek(fptr,0,SEEK_SET); // Move the pointer to the start of the file
        printf("The half size of the file : %ld\n",fileSize/2);

        char bufferSend[fileSize/2]; // The half size of the message sent from the sender(file size)
        char CCcub[8]; // The size of 
        char CCren[8];

     int sockfd = -1;
     if((sockfd = socket(AF_INET , SOCK_STREAM , 0 )) == -1) { // create file descriptor socket
            perror("Could not create file descriptor socket ");
            return -1;
        }
        
    struct sockaddr_in serveraddress;             
    memset(&serveraddress,0,sizeof(serveraddress)); // reset serveraddress
    serveraddress.sin_family = AF_INET; // define address type(IPV4) 
    serveraddress.sin_port = htons(SERVER_PORT); // convert server port to network byte order

    int rval = inet_pton(AF_INET,(const char*)SERVER_IP_ADDRESS,&serveraddress.sin_addr); // Convert IP Address: binary/decimal representation to network bytes order 

    if(rval <= 0){
        perror("Inet pton failed");
        return -1;
    }
    
    printf("Connected .... \n"); 

    int connection = connect(sockfd, (struct sockaddr*)&serveraddress, sizeof(serveraddress)); // Connect to sockfd
    
    if(connection == -1){
        perror("connection failed");
        return -1;
    }
    
    int xor = (ID1^ID2); // The message the sender expects to receive (for authontication)
    int xorRecv = 0; // The message from the receiver (for authontication)
    
    
    while(1){ // while loop to resend the file 
    
    int bytesSent = 0; // The bytes the sender sent
    int bytesRecv = 0; // The bytes the sender received from the receiver

    fseek(fptr,0,SEEK_SET); // Move the pointer to the start of the file
        // Change CC 
        strcpy(CCcub,"cubic"); // Change cc algorithm to cubic
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, CCcub, strlen(CCcub)) == -1)
        {
            perror("setsockopt");
            return -1;
        }
        printf("CC algorithm changed to : cubic\n");

        while ((connection = fread(bufferSend, 1, sizeof(bufferSend), fptr)) > 0) { // Send the first half of the file
            bytesSent = send(sockfd, bufferSend, connection, 0);
            printf("LOOK HEREEEEE : %d \n" , bytesSent);
        if (bytesSent == 0) {
                printf("peer has closed the TCP connection.\n");
            }
        else if (bytesSent == -1) {
                printf("send failed.\n");
            }
        else if (connection > bytesSent) {
                printf("sent only %ld bytes from the required %d.\n", strlen(bufferSend), bytesSent);
            }
        else {
                printf("Half one of the file send seccessfully sent (Total bytes sent : %d) .\n", bytesSent);
                break;
            }
        }
        printf("Waiting For Authontication .. \n");

        bytesRecv = recv(sockfd, &xorRecv, sizeof(int), 0); // Receive a authontication from the receiver

        if (bytesRecv == 0)
        {
            printf("peer has closed the TCP connection.\n");
        }

        else if (bytesRecv == -1)
        {
            printf("Receive failed");
        }

        else
        {
            printf("Received  %d bytes from the required.\n", bytesRecv);
        }
        if (xor != xorRecv)
        {
            printf("Authontication failed\n");
            break;
        }

        else
            printf("Autontication completed.\n"); // The first half of the file seccessfully received to the receiver
        
        // Change CC
        strcpy(CCren, "reno"); // Change cc algorithm to reno
        
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, CCren, strlen(CCren)) != 0) {
			perror("setsockopt error");
			return -1;
		}
        printf("CC algorithm changed to : reno\n");
        
        while ((connection = fread(bufferSend, 1, sizeof(bufferSend), fptr)) > 0) {  // Send the second half of the file
            bytesSent = send(sockfd, bufferSend, connection, 0);
            if (bytesSent == 0) {
                printf("peer has closed the TCP connection.\n");
            }
            else if (bytesSent == -1) {
                printf("send failed.\n");
            }
            else if (connection > bytesSent) {
                printf("sent only %ld bytes from the required %d.\n", strlen(bufferSend), bytesSent);
            }
            else if(ftell(fptr) == fileSize){
                printf("Half two of the file send seccessfully sent (Total bytes sent : %ld) .\n", ftell(fptr));
                
            }
        }
    char exit = '0'; // For Scanner(exit = 'Y' --> start while loop again , exit = 'N' --> break while loop)
    int keepAlive; // Massage for the connection 
    int check = -1; // Check if the receiver close the connection
    
    recv(sockfd, &keepAlive, sizeof(int), 0); // received massage from the receiver to keep connection 
    printf("Do you want to send the file again ? :) \n Yes- press 'Y' , No- press 'N' \n");

    while(exit != 'Y' && exit != 'N'){ // While loop --> if the user type a char that is not Y or N send the ma
    scanf(" %c" , &exit);
    if(exit !='Y' && exit != 'N')
    {
    printf("Press only : Y - Send again || N - Close connection \n"); 
    }
    }
    if(exit == 'N')  // If exit = N --> close connection
    {
    printf("Closes the connection .. \n");
    break;
    }
    send(sockfd, &keepAlive, sizeof(int), 0); // send message to the receiver to keep connection(confirm)
    recv(sockfd, &keepAlive, sizeof(int), 0); // received a massage from the receiver to check if the receiver get the confirm message from the sender
    }
    
    fclose(fptr);
    close(sockfd);

}

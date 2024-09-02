#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define SERVER_PORT 9999 
#define ID1 1242
#define ID2 3836
#define fileSize 1048576


int main(){ 

	struct sockaddr_in serverAddress ,clientAddress;
    struct timeval timeStart, timeEnd;
    socklen_t clientAddressLen = sizeof(clientAddress);
    char bufferSend[fileSize]; // The half size of the message sent from the sender(file size)
    char CCcub[8]; // Cubic size
    char CCren[8]; // Reno size

    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));
     clientAddressLen = sizeof(clientAddress);


	int sockfd = -1;
    int reuse = 1; 
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
	{ 
        perror("Could not create listening socket \n"); //
        return -1;  
    }
                
	if ((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int))) == -1) 
	{ 
    	printf("setsockopt failed \n");  
    	return -1;
	} 
    
  	serverAddress.sin_family = AF_INET; // define address type(IPV4) 
	serverAddress.sin_port = htons(SERVER_PORT); // convert server port to network byte order 
    serverAddress.sin_addr.s_addr = INADDR_ANY;   
    
    //bind

    printf("Binding\n");

	if (bind(sockfd, (struct sockaddr *)&serverAddress,  sizeof(serverAddress)) == -1)  // bind to connect - (AF_INET,SOCK_STREAM)
	{ 
        perror("Bind failed");
    	close(sockfd);
    	return -1;
	} 
	    printf("Binded\n");
	
	if (listen(sockfd, 5) < 0) // Max 5 client connection requests
    {
        printf("listen failed");
    	close(sockfd);
    	return -1;
    }
    printf("Listening ..\n");

    int connection = accept(sockfd, (struct sockaddr *)&clientAddress, &clientAddressLen); // Accept sender connection to sockfd
    if (connection == -1)
	{
    	printf("accept failed");
        close(sockfd);
    	return -1;
    }
        printf("connection successful .. \n");
    
    int arrayIndex = 0;  
    int arraySize = 8;  
    
    double* HalfONE = (double*) malloc(arraySize * sizeof(double)); // HalfONE pointer about dinamic array in length 8*8 (which contains the times of the first part)
    double* HalfTWO = (double*) malloc(arraySize * sizeof(double));  // HalfTWO pointer about dinamic array in length 8*8 (which contains the times of the second part)
    
    if (HalfONE == NULL || HalfTWO == NULL ){
        perror("malloc");
        close(sockfd);
        return -1;
    }
         
    int xor = (ID1^ID2); // The message the sender expects to receive
    int xorRecv; // The message from the receiver
    int TIME = 0; // Time between the send and the receive

    while(1){ // while loop to resend the file
    int countHalfONE = 0; // Time of the first half of the file
    int countHalfTwo = 0; // TIme of the second half of the file
    int bytesSent = 0; // The bytes the sender send to the receiver
    int bytesRecv = 0; // The bytes the sender get from the receiver        
        bzero(bufferSend, sizeof(bufferSend)); // Reset buffer to zero
        // Change CC 
        strcpy(CCcub,"cubic");  // Change CC algorithm to cubic

        if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, CCcub, strlen(CCcub)) == -1) 
        {
            perror("setsockopt");
            return -1;
        }
            
        printf("CC algorithm changed to : cubic\n");


        printf("Wait for receiving the first half of the file\n");
        gettimeofday(&timeStart, NULL);  // Start counting time (How much time the receiver received the first half of the message) 

        while(countHalfONE != fileSize/2){ // While loop to received the first half of the file from the sender

            bytesRecv = recv(connection, bufferSend, sizeof(bufferSend) , 0);
            printf("Packet size : %d\n",bytesRecv);
            countHalfONE+=bytesRecv;
        }
        if (bytesRecv <= 0)
        {
            printf("Sender has closed the TCP connection.\n");
            break;
        }
        

        gettimeofday(&timeEnd, NULL); // End counting time (How much time the receiver received the first half of the message) 

        *(HalfONE + TIME) = (timeEnd.tv_sec - timeStart.tv_sec)*1000 + (((double)timeEnd.tv_usec - timeStart.tv_usec))/1000; // Total half one time to received
        printf("The time of the first half to receive : %f \n" , *(HalfONE+TIME));
        printf("The first half of the file received successfully \n");
        printf("Total bytes received: (%d bytes)\n", countHalfONE); 

        printf("Sending authontication .. \n");


        bytesSent = send(connection, &xor, sizeof(int), 0); // Send the authontication to the sender

        if (bytesSent == 0)
        {
            printf("Sender has closed the TCP connection.\n");
            return -1;
            break;
        }

        else if (bytesSent < 0)
        {
            perror("Send");
            return -1;
        }
        else{

        printf("Authontication sent .. \n");
        }

        // Change CC

        strcpy(CCren,"reno"); // Change CC algorithm to reno

        if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, CCren, strlen(CCren)) == -1)
        {
            perror("setsockopt");
            return -1;
        }
            
        printf("CC algorithm changed to : reno\n");
        printf("Receiving the second half of the file \n");

        gettimeofday(&timeStart, NULL); // Start counting time (How much time the receiver received the second half of the message) 
        
        while(countHalfTwo != fileSize/2){ // While loop to received the second half of the file from the sender

            bytesRecv = recv(connection, bufferSend, sizeof(bufferSend) , 0);
            printf("Packet size : %d\n",bytesRecv);
            countHalfTwo+=bytesRecv;
        }

       if (bytesSent == 0)
        {
            printf("Sender has closed the TCP connection.\n");
            return -1;
            break;
        }

        else if (bytesSent < 0)
        {
            perror("Receive");
            return -1;
        }

        gettimeofday(&timeEnd, NULL); // End counting time (How much time the receiver received the second half of the message)

        *(HalfTWO + TIME) = (timeEnd.tv_sec - timeStart.tv_sec)*1000 + (((double)timeEnd.tv_usec - timeStart.tv_usec))/1000; // Total half two time to received 
        printf("The time of the second half to receive : %f \n" , *(HalfTWO+TIME));
        printf("The second half of the file received successfully\n");
        printf("Total bytes received: (%d bytes)\n", countHalfTwo);
    
        if (TIME >= arraySize) // If the array full ---> Change array size
        {
            arraySize *= 2;
            HalfONE = (double*) realloc(HalfONE, arraySize * sizeof(double));
            HalfTWO = (double*) realloc(HalfTWO, arraySize * sizeof(double));
        }

        TIME++; 

        int keepAlive; // message for the connection 
        int check = -1; // Check if the receiver close the connection

        send(connection, &keepAlive, sizeof(int), 0); // Send message to the sender to check connection

        if ((check = (recv(connection, &keepAlive, sizeof(int), 0))) == 0) // received message from the sender if to keep connection(confirm)
        {
            printf("TCP Connection with the sender was closed.\n");
            break;
        }
        else 
        {
        send(connection, &keepAlive, sizeof(int), 0); // Send a massage to the sender if to keep connection(confirm) 
        }
        
    }

    double avarage_HalfONETime = 0, avarage_HalfTwoTime = 0; // Avarage time for each half of the file to received

    printf("\nCubic Total Run Times:\n");

    for (int i = 0; i < TIME; i++) // Loop for calculate avarage time for the first half (CC cubid)
    {
        avarage_HalfONETime += *(HalfONE + i);
        printf("Cubic run %d time = %0.3lf ms\n", (i+1), *(HalfONE + i));
    }
    avarage_HalfONETime = avarage_HalfONETime / TIME; 
    printf("Total avarage cubic: %0.3lf ms\n", avarage_HalfONETime);

    printf("\nReno Total Run times:\n");

    for (int i = 0; i < TIME; i++) // Loop for calculate avarage time for the second half (CC reno)
    {
        avarage_HalfTwoTime += *(HalfTWO + i);
        printf("Reno run %d time = %0.3lf ms\n", (i+1), *(HalfTWO + i));
    }
    avarage_HalfTwoTime = avarage_HalfONETime / TIME;
    printf("Total avarage reno: %0.3lf ms\n", avarage_HalfTwoTime);


    close(connection);
    close(sockfd);
    return 0;
}



#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

#define SERVER_PORT_FOR_CLIENT 10001

#define SERVER_IP_ADDRESS_FOR_PROXY "8.8.8.8"
#define SERVER_PORT_FOR_PROXY 10002


int main()
{
    char buffer[1500] = { '\0' };
	int SocketRecev  = -1;
	if ((SocketRecev = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
	{
		printf("Could not create SocketRecev\n");
		return -1;
    }

    int SocketSender = -1;
	if ((SocketSender  = socket(AF_INET, SOCK_DGRAM, 0)) == -1) 
	{
		printf("Could not create SocketSender\n");
			return -1;
	}

    struct sockaddr_in addressOfProxy;
	memset((char *)&addressOfProxy, 0, sizeof(addressOfProxy));
	addressOfProxy.sin_family = AF_INET;
	addressOfProxy.sin_port = htons(SERVER_PORT_FOR_CLIENT);
	addressOfProxy.sin_addr.s_addr = htonl(INADDR_ANY);


	struct sockaddr_in addressThatSendHim;
	memset((char *)& addressThatSendHim, 0, sizeof(addressThatSendHim));
	addressThatSendHim.sin_family = AF_INET;
	addressThatSendHim.sin_port = htons(SERVER_PORT_FOR_PROXY);
	inet_pton(AF_INET, (const char*)SERVER_IP_ADDRESS_FOR_PROXY ,&(addressThatSendHim.sin_addr));

	if (bind(SocketRecev, (struct sockaddr *)&addressOfProxy, sizeof(addressOfProxy)) == -1)
	{
		printf("bind() failed with error code\n");
		return -1;
	}

	printf("After bind(). Waiting for clients\n");
   
	struct sockaddr_in clientOfProxyAddress;
	memset((char *)& clientOfProxyAddress, 0, sizeof( clientOfProxyAddress));
    socklen_t  clientOfProxyAddressLen = sizeof( clientOfProxyAddress);

	while (1)
	{
		bzero(buffer, sizeof(buffer));

		int recv_len = -1;

		if ((recv_len = recvfrom(SocketRecev, buffer, sizeof(buffer), 0, (struct sockaddr *) &clientOfProxyAddress, &clientOfProxyAddressLen)) == -1)
		{
			printf("recvfrom() failed with error code\n");
			return 1;
		}
        
        printf("the pacta receved!\n");
        
        int x = random()%100;
        if(x <= 50){
            printf("throw the pacta!\n");
        }
        if(x>50){
	
		    if (sendto(SocketSender, buffer, recv_len, 0, (struct sockaddr*) &addressThatSendHim, sizeof(addressThatSendHim)) == -1)
		    {
			printf("sendto() failed with error code\n");
			break;
		    }

            printf("sent\n");
	    }
    }

	close(SocketRecev);
    close(SocketSender);

    return 0;
}





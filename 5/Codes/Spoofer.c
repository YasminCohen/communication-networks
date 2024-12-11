#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

unsigned short in_cksum (unsigned short *buf, int length)
{
   unsigned short *w = buf;
   int nleft = length;
   int sum = 0;
   unsigned short temp=0;

   while (nleft > 1)  {
       sum += *w++;
       nleft -= 2;
   }

   if (nleft == 1) {
        *(u_char *)(&temp) = *(u_char *)w ;
        sum += temp;
   }

   sum = (sum >> 16) + (sum & 0xffff);
   sum += (sum >> 16);
   return (unsigned short)(~sum);
}

void send_raw_ip_packet(struct ip* ip)
{
   struct sockaddr_in dest_info;
   int enable = 1;

   int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

   if (sock == -1)
   {
      perror("socket");
      exit(1);
   }

   if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &enable, sizeof(enable)) == -1)
   {
      perror("setsockopt");
      exit(1);
   }

   dest_info.sin_family = AF_INET;
   dest_info.sin_addr = ip->ip_dst;

   if (sendto(sock, ip, ntohs(ip->ip_len), 0, (struct sockaddr *)&dest_info, sizeof(dest_info)) == -1)
   {
      perror("sendto");
      exit(1);
   }

   close(sock);
}

int main() {
   char buffer[1500];

   memset(buffer, 0, 1500);

   struct ip *ip = (struct ip *) buffer;
   struct icmphdr *icmp = (struct icmphdr *) (buffer + sizeof(struct ip));

   ip->ip_v = 4;
   ip->ip_hl = 5;
   ip->ip_ttl = 20;
   ip->ip_src.s_addr = inet_addr("1.2.3.4");
   ip->ip_dst.s_addr = inet_addr("10.0.2.15");
   ip->ip_p = IPPROTO_ICMP; 
   ip->ip_len = htons(sizeof(struct ip) + sizeof(struct icmphdr));

   icmp->type = ICMP_ECHO;
   icmp->checksum = 0;
   icmp->checksum = in_cksum((unsigned short *)icmp, sizeof(struct icmphdr));

   send_raw_ip_packet(ip);

   return 0;
}
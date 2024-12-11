#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <pcap.h>

#define ID "212733836_315141242"

struct calchdr
{
	u_int32_t time;
	u_int16_t len;

	union
	{
		u_int16_t f;
		u_int16_t res:3, c_flag:1, s_flag:1, t_flag:1, status_code:10;
	};
	

	u_int16_t cache_control;
	u_int16_t padding;
};

void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
	static int count = 1;                   /* packet counter */

	/* declare pointers to packet headers */
	const struct ip *ip = (struct ip*)(packet + sizeof(struct ethhdr)); /* The IP header */
	const struct tcphdr *tcp = (struct tcphdr*)(packet + sizeof(struct ethhdr) + ip->ip_hl*4); /* The TCP header */
	struct calchdr* segment = (struct calchdr*)(packet + sizeof(struct ethhdr) + ip->ip_hl*4 + tcp->doff*4);
	const char* data = (char*)(packet + sizeof(struct ethhdr) + ip->ip_hl*4 + tcp->doff*4 + sizeof(struct calchdr));

	if(tcp->psh != 1)
		return;

	char filename[50];
    sprintf(filename, "%s.txt", ID);
    FILE *fp = fopen(filename, "a");

	if (fp == NULL)
	{
		perror("fopen");
		exit(1);
	}

	printf("\nPacket number %d:\n", count);
	count++;
	printf("       From: %s\n", inet_ntoa(ip->ip_src));
	printf("         To: %s\n", inet_ntoa(ip->ip_dst));

	printf("   Src port: %d\n", ntohs(tcp->th_sport));
	printf("   Dst port: %d\n", ntohs(tcp->th_dport));

	segment->f = ntohs(segment->f);


    fprintf(fp, "{ source_ip: %s, dest_ip: %s, source_port: %hu, dest_port: %hu, timestamp: %u, total_length: %hu, cache_flag: %hhu, steps_flag: %hhu ,type_flag: %hhu , status_code: %hu, cache_control: %hu, data: ", 
		inet_ntoa(ip->ip_src), inet_ntoa(ip->ip_dst), ntohs(tcp->th_sport), ntohs(tcp->th_dport),
		ntohl(segment->time), ntohs(segment->len), ((segment->f>>12) & 1), ((segment->f>>11) & 1),
		((segment->f>>10) & 1), segment->status_code, ntohs(segment->cache_control));
    
    for (int i = 0; i < ntohs(segment->len); i++)
    {
		if (!(i & 15)) fprintf(fp, "\n%04X: ", i);

        fprintf(fp, "%02X ", ((u_char*)data)[i]);
    }

    fprintf(fp, "}\n");

    fclose(fp);

	return;
}

int main(int argc, char **argv)
{

	char *dev = NULL;			/* capture device name */
	char *deff = "lo";
	char errbuf[PCAP_ERRBUF_SIZE];		/* error buffer */
	pcap_t *handle;				/* packet capture handle */

	char filter_exp[] = "tcp";		/* filter expression [3] */
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


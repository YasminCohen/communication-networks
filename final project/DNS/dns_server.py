from scapy.all import DNS, DNSQR, IP, sr1, UDP
from scapy.all import *
import sys


def dns_server(pkt):
# check if packet protocol is dns
    if pkt[DNS]:
# check if the packet send to the server(check the ip destination)
        if pkt[IP].dst== '127.0.0.1':
            print(f"Received message from {pkt[IP].src}")
# define qname to be the url           
            qname = pkt[DNS].qd.qname.decode('utf-8')
            print("DNS query:", qname)
# define newIP to be the url IP address                
            newIP = socket.gethostbyname(qname)
            print(f"The domain that needs to be converted to IP is:{newIP} ")
# send the packet with answer the ip of the url address            
            sendp(Ether(dst = 'ff:ff:ff:ff:ff:ff')/IP(dst=pkt[IP].src)/UDP(dport=53)/DNS(rd=1,qd=DNSQR(qname=f"{newIP}"), an=DNSRR(rrname= qname, rdata= f"{newIP}")),verbose=0)
            
           
            
# sniff packet with protocol udp and port 53                    
sniff(filter='udp and (port 53)', prn=dns_server,count = 1) 




  

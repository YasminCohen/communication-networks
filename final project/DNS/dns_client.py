from scapy.all import DNS, DNSQR, IP, sr1, UDP
from scapy.all import *
import sys


def dns_request():
#create a DNS packet with query for the ip of "www.google.com"
    request = Ether(dst = 'ff:ff:ff:ff:ff:ff')/IP(src = '0.0.0.0',dst = '127.0.0.1')/UDP(dport=53)/DNS(rd=1,qd=DNSQR(qname='www.google.com'))
#send request
    sendp(request)
    

dns_request()

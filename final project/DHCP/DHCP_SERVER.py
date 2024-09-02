from scapy.all import *


def dhcp_server(pkt):
# check if the packet protocol is DHCP 
    if pkt[DHCP]:
        if pkt[DHCP].options[0][1] == 1: # options[0][1] = discover
            offer = Ether(dst='ff:ff:ff:ff:ff:ff')/IP(src='192.168.1.1', dst='192.168.1.100')/UDP(sport=67,dport=68)/BOOTP(op=2, yiaddr='192.168.1.100', siaddr='192.168.1.1', chaddr=pkt[Ether].src)/DHCP(options=[('message-type', 'offer'), ('subnet_mask', '255.255.255.0'), ('router', '192.168.1.1'), 'end'])            
# send offer for the discover packet            
            sendp(offer)
        elif pkt[DHCP].options[0][1] == 3: # oprions[0][3] = request
            ack = Ether(dst='ff:ff:ff:ff:ff:ff')/IP(src='192.168.1.1', dst='192.168.1.100')/UDP(sport=67,dport=68)/BOOTP(op=2, yiaddr='192.168.1.100', siaddr='192.168.1.1', chaddr=pkt[Ether].src)/DHCP(options=[('message-type', 'ack'), ('subnet_mask', '255.255.255.0'), ('router', '192.168.1.1'), 'end'])            
# send ack for the request packet(server accept the request massage)
            sendp(ack)

# sniff packets udp with port 67 or 68 
sniff(filter='udp and (port 67 or 68)', prn=dhcp_server) 





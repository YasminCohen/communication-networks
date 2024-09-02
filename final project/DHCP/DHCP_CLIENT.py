from scapy.all import *
import sys


conf.checkIPaddr = False
fam, hw = get_if_raw_hwaddr(conf.iface)
# create a DHCP packet with type 'discover'(Brodcast) 
discover = Ether(dst = 'ff:ff:ff:ff:ff:ff')/IP(src='0.0.0.0', dst='255.255.255.255')/UDP(sport=68, dport=67)/BOOTP(chaddr=hw)/DHCP(options=[('message-type', 'discover'), 'end'])
# send discover packet and define ansDIS be the answer with type 'offer'
ansDis = srp1(discover)
if True:
# create a DHCP packet with type 'request' 
    request = Ether(dst = 'ff:ff:ff:ff:ff:ff')/IP(src=ansDis[BOOTP].yiaddr, dst='255.255.255.255')/UDP(sport=68, dport=67)/BOOTP(chaddr=hw,yiaddr = ansDis[BOOTP].yiaddr)/DHCP(options=[('message-type', 'request'), 'end'])
# send request packet
    ansReq = sendp(request)


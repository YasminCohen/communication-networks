import sys
import os
from socket import AF_INET, socket, SOCK_STREAM, SOCK_DGRAM
import threading
from threading import Thread, Lock
import pickle


class Client:

#    for communication with server

    if len(sys.argv) > 1:
        HOST = sys.argv[1]
    else:
        HOST = '127.0.0.1'
    PORT = 20896
    ADDR = (HOST, PORT)
    BUFSIZE = 1024

    def __init__(self):

#    constractor and send name to server

        self.isconnected = False
        self.reconnect = True
        self.exited=False
        # set a dictionary for the files to download
        self.files = {}
        self.messages = []

        self.connect_thread = Thread(target=self.connect_server)
        self.connect_thread.start()
        self.receive_thread = Thread(target=self.receive_messages)
        self.receive_thread.start()

        self.lock = Lock()

    def connect_server(self):
        flag = True
        while not self.exited:
            if self.reconnect and not self.isconnected:
                try:
                    # open tcp socket for client
                    self.client_socket = socket(AF_INET, SOCK_STREAM)
                    self.client_socket.connect(self.ADDR)

                    # open UDP with rdt for file transfer
                    self.client_socketUDP = socket(AF_INET, SOCK_DGRAM)
                    self.client_socketUDP.connect(self.ADDR)
                    self.isconnected = True
                    print("Client Connected\n")
                    print(f"---------------welcome!---------------\n")
                    print(f"You can download files from the list below:\n")
                    print(f"For get the list of the files, input <show>\n")
                    print(f"For discconnect the client, input <quit>\n")
                    print(f"Input the number of the file that you want to download.\n")
                    self.send_message("UPDATE")

                    break
                except:
                    if flag:
                        print('Waiting for HTTP Server Connection...')
                        flag = False
            else:
                flag = True
            if not self.isconnected and self.exited:
                break

    def print_files(self, sp):
        print("The list of the files:")
        self.files = {}
        for i in range(1, len(sp)):
            key = sp[i].split(':')[0]
            value = sp[i].split(':')[1]
            self.files[key] = value
            print(f"{key}: {value}")
        print()


    def download(self, port, size, path):
        try:
            with open(path, "wb") as file:
                size = (size // 1000) + 1
                ADDRESS_SERVER = (self.HOST, port)
                CLIENTUDP = socket(AF_INET, SOCK_DGRAM)
                for i in range(size):
                    done = False
                    while not done:
                        CLIENTUDP.sendto(bytes(f"part {i}", "utf-8"), ADDRESS_SERVER)
                        msg, addr = CLIENTUDP.recvfrom(self.BUFSIZE)
                        # filter messages that are not from our server
                        while (addr != ADDRESS_SERVER):
                            msg, addr = CLIENTUDP.recvfrom(self.BUFSIZE)
                        msg, msg_size = pickle.loads(msg)
                        if type(msg_size) == int and len(msg) == msg_size:
                            done = True
                        else:
                            print("lost a packet, send again.")
                    file.write(msg)
                    percent = round(i / size * 100, 2)
                    print(f"{percent}% downloaded")
            print("File was downloaded 100%\n")

        except:
            self.send_message("UPDATE")


        finally:
            CLIENTUDP.sendto(bytes("finished", "utf-8"), ADDRESS_SERVER)
            CLIENTUDP.close()

    def receive_messages(self):

#        receive messages from server

        while True:
            if self.isconnected:
                try:
                    msg = self.client_socket.recv(self.BUFSIZE).decode('utf-8')
                    sp = msg.split()
                    if sp[0] == "FILES":
                        self.print_files(sp)
                    elif sp[0] == "UPDATE":
                        self.files = {}
                        print("FILES UPDATE")
                        self.print_files(sp)
                    elif msg.split()[0] == "UDP":
                        port = int(msg.split()[1])
                        filesize = int(msg.split()[2])
                        path = os.path.join(os.getcwd(), 'Downloads')
                        try:
                            os.mkdir(path)
                        except:
                            pass
                        download_thread = threading.Thread(target=self.download, args=(
                        port, filesize, f"{os.getcwd()}/Downloads/{msg.split()[3]}"))
                        download_thread.start()
                    else:
                        print(f'client received message:{msg}')
                    # make sure memory is safe to access
                    self.lock.acquire()
                    self.messages.append(msg)
                    self.lock.release()
                except Exception as e:
                    if self.isconnected:
                        self.disconnect()
            else:
                if not self.connect_thread.is_alive():
                    self.connect_thread = Thread(target=self.connect_server)
                    self.connect_thread.start()

    def send_message(self, msg):

#        send messages to server

        try:
            self.client_socket.send(bytes(msg, "utf8"))
        except Exception as e:
            if self.isconnected:
                print('HTTP Server Crashed!')
                self.isconnected = False

    def get_messages(self):

#        returns a list of str messages

        try:
            messages_copy = self.messages[:]

            # make sure memory is safe to access
            self.lock.acquire()
            self.messages = []
            self.lock.release()
            return messages_copy
        except:
            return self.messages[:]

    def disconnect(self):
        if not self.isconnected:
            return
        print("Client Disconnected.")
        self.send_message("{quit}")
        self.client_socket.close()
        self.isconnected = False
    def exit(self):
        self.exited = True

def run():
    # Create a client object
    client = Client()

    while True:
        msg = input()
        if not client.isconnected:
            if msg == 'reconnect':
                client.reconnect = True
            else:
                print("CLIENT DISCONNECTED")
        elif msg == 'show':
            client.send_message("get_files")
        elif msg == 'quit':
            client.reconnect = False
            client.disconnect()
            print("for reconnect input <reconnect>")
        elif msg in client.files.keys():
            client.send_message(f'download_file {client.files[msg]}')
        else:
            print(f"<{msg}> not exist")
            client.send_message("UPDATE")
if __name__ == '__main__':
    run()


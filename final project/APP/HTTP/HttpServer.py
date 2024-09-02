import sys
from socket import AF_INET, socket, SOCK_STREAM, SOCK_DGRAM
from threading import Thread, Lock
import time

# user class
class User:

#    Represents a online_user, holds name, socket client and IP address

    def __init__(self, addr, client):
        self.addr = addr
        self.app_server = AppServer(client)
        self.client = client
    def disconnect(self):
        self.app_server.disconnect()
    def __repr__(self):
        return f"User({self.addr})"

class AppServer:

#   for communication with server

    if len(sys.argv) > 1:
        HOST = sys.argv[1]
    else:
        HOST = '127.0.0.1'
    PORT = 30242
    ADDR = (HOST, PORT)
    BUFSIZE = 1024

    def __init__(self, client):

#        constractor send name to server
        self.isconnected = False
        self.reconnect = True
        self.messages = []
        self.client = client
        self.connect_thread = Thread(target=self.connect_server)
        self.connect_thread.start()
        self.receive_thread = Thread(target=self.receive_messages)
        self.receive_thread.start()
        self.lock = Lock()

    def connect_server(self):
        flag = True
        while True:
            if self.reconnect and not self.isconnected:
                try:
                    # open tcp socket for client
                    self.client_socket = socket(AF_INET, SOCK_STREAM)
                    self.client_socket.connect(self.ADDR)

                    # open UDP with rdt for file transfer
                    self.client_socketUDP = socket(AF_INET, SOCK_DGRAM)
                    self.client_socketUDP.connect(self.ADDR)
                    self.isconnected = True
                    print("Connected to the App Server\n")
                    self.send_message(bytes('App Server is ready',"utf8"))
                    time.sleep(0.1)
                    self.send_to_server(bytes('get_files',"utf8"))
                    break
                except:
                    if flag:
                        print('Waiting for App Server Connection...')
                        flag = False
            else:
                flag = True


    def receive_messages(self):

#       receive messages from server

        while True:
            if self.isconnected:
                try:
                    msg = self.client_socket.recv(self.BUFSIZE)
                    print('[RECEIVE] received a message from the App Server')
                    self.send_message(msg)
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

#       send messages to server

        try:
            self.client.send(msg)
            print('[SENDING] from HTTP to the Client')
        except Exception as e:
            if self.isconnected:
                print('App Server Crashed!')
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
        print(f"{self} Disconnected.")
        self.send_to_server(bytes("{quit}", "utf8"))
        self.isconnected = False

    def send_to_server(self, msg):

#     send messages to server

        try:
            self.client_socket.send(msg)
            print(f'[SENDING] from HTTP to {self}')
        except Exception as e:
            if self.isconnected:
                print('Server Crashed!')
                self.isconnected = False

    def __repr__(self):
        return f"APP Server({self.ADDR})"


# GLOBAL CONSTANTS
HOST = ''
PORT = 20896
MAX_CONNETIONS = 10
ADDR = (HOST, PORT)
BUFSIZ = 1024
FILES_FOLDER = 'Files'

# GLOBAL VARIABLES

users = []
SERVER = socket(AF_INET, SOCK_STREAM)
SERVER.bind(ADDR)  # set up server



def client_communication(user):

#    Thread to handle all messages from client

    client = user.client
    while True:  # wait for any messages from person
        try:
            msg = client.recv(BUFSIZ)
        except:
            users.remove(user)
            break
        if msg == bytes("{quit}", "utf8"):  # if message is qut - so disconnect the client
            print(f"[DISCONNECTED] {user} disconnected from the server at {time.time()}\n")
            user.disconnect()
            users.remove(user)
            print(f'connected users: {users}')
            client.close()
            break
        else:  # otherwise send message to all other clients
            if user.app_server.isconnected:
                print(f'[RECEIVE] received a messa4ge from {user}')
                user.app_server.send_to_server(msg)
            else:
                client.send(bytes(f'NO APP_SERVER CONNECTION', "utf8"))
    print('[DISCONNECTION] Client Disconnected')


def wait_for_connection():

#    Wait for connecton from new clients, start new thread once connected

    while True:
        try:
            client, addr = SERVER.accept()  # wait for any new connections
            user = User(addr, client)  # create new person for connection
            print(f"[CONNECTION] {addr} connected to the server at {time.time()}")
            users.append(user)
            print(f'Connected Users: {users}')
            Thread(target=client_communication, args=(user,)).start()
        except Exception as e:
            print("[EXCEPTION]", e)
            break

    print("SERVER CRASHED")


def start_server():
    SERVER.listen(MAX_CONNETIONS)  # open server to listen for connections
    print("[STARTED] Waiting for connections...")
    ACCEPT_THREAD = Thread(target=wait_for_connection)
    ACCEPT_THREAD.start()
    ACCEPT_THREAD.join()
    SERVER.close()


def stop_server():
    SERVER.close()


if __name__ == "__main__":
    start_server()
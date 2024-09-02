from socket import AF_INET, socket, SOCK_STREAM, SOCK_DGRAM
from threading import Thread
import time
import os
import pickle


# user class
class User:

# Online_user, holds name, socket client and IP address


    def __init__(self, addr, client):
        self.addr = addr
        self.client = client

    def __repr__(self):
        return f"User({self.addr})"


# GLOBAL CONSTANTS
HOST = ''
PORT = 30242
MAX_CONNETIONS = 10
ADDR = (HOST, PORT)
BUFSIZ = 1024
FILES_FOLDER = 'Files'

# GLOBAL VARIABLES
users = []
SERVER = socket(AF_INET, SOCK_STREAM)
SERVER.bind(ADDR)  # set up server
# dict of all ports 55000-55015
# (if port is caught his value = False otherwise True)
ports = {}
for i in range(16):
    ports[30242 + i] = True


def download(client, filename):
    SIZE = 1000
    LOST =100
    LATENCY= 0.1
    filepath = f"{os.getcwd()}/Files/{filename}"
    filesize = os.path.getsize(filepath)
    prt = 0
    for port, flag in ports.items():
        if flag:
            prt = port
            ports[port] = False
            break
    if prt == 0:
        print("No port find")
        return

    SERVERUDP = socket(AF_INET, SOCK_DGRAM)
    # Bind to address and ip

    SERVERUDP.bind(("", prt))
    print("[UDP STARTED] Waiting for connections...")

    msg = bytes(f"UDP {prt} {filesize} {filename}", "utf8")
    client.send(msg)

    with open(filepath, "rb") as file:
        msg, addr = SERVERUDP.recvfrom(BUFSIZ)
        msg = msg.decode('utf-8')
        print(f'[SEND] sending {filename} to the Client')
        print('started', end='')
        c = 7
        while (msg != "finished"):
            msg = msg.split()
            if msg[0] == "part":
                numpart = int(msg[1])
                file.seek(numpart * SIZE)
                data = file.read(SIZE)
                data_size = len(data)
                c += 1
                if c % LOST == 0:
                    data = file.read(SIZE-10)
                    print("lost packet sent")
                    time.sleep(LATENCY)

                data = (data, data_size)
                data = pickle.dumps(data)
                SERVERUDP.sendto(data, addr)
                print('.', end='')

            msg, addr = SERVERUDP.recvfrom(BUFSIZ)
            msg = msg.decode('utf-8')
        print('finished')
        SERVERUDP.close()
        ports[addr[1]] = True


def get_files():
    path = f"{os.getcwd()}/{FILES_FOLDER}"
    return os.listdir(path)


def create_str_files(files: list):
    str_files = ''
    count = 1
    for f in files:
        str_files += f'{count}:{f} '
        count += 1
    return str_files


def check_files(client, files):
    tmp = get_files()
    if files == tmp:
        return True
    else:
        files = tmp
        client.send(bytes(f'UPDATE {create_str_files(files)}', "utf8"))
        print('[UPDATE] sending update of the list of the files.')


def client_communication(user):

# Thread to handle all messages from client


    client = user.client
    # List for the names of the files
    files = get_files()
    while True:  # wait for any messages from person
        try:
            msg = client.recv(BUFSIZ)
        except:
            print('HTTP SERVER CRASHED!')
            users.remove(user)
            break
        if msg == bytes("{quit}", "utf8"):  # if message is qut - so disconnect the client
            print(f"[HTTP DISCONNECTED] {user} disconnected from the server at {time.time()}\n")
            users.remove(user)
            print(f'connected users: {users}')
            break

        elif msg.split()[0].decode('utf-8') == "download_file":
            filename = msg.split()[1].decode('utf-8')
            try:
                download(client, filename)
            except:
                check_files(client, files)
        elif msg.split()[0].decode('utf-8') == "get_files":
            files = get_files()
            client.send(bytes(f'FILES {create_str_files(files)}', "utf8"))
            print('[GET] sending the list of the files.')
        elif msg.decode('utf-8') == "UPDATE":
            check_files(client, files)
        else:  # otherwise send message to all other clients
            print(msg.decode('utf-8'))
    print("[STARTED] Waiting for HTTP connections...")


def wait_for_connection():

#  Wait for connecton from new clients, start new thread once connected

    while True:
        try:
            client, addr = SERVER.accept()  # wait for any new connections
            user = User(addr, client)  # create new person for connection
            print(f"[HTTP CONNECTION] {addr} connected to the server at {time.time()}")
            users.append(user)
            print(f'Connected Users: {users}')
            Thread(target=client_communication, args=(user,)).start()
        except Exception as e:
            print("[EXCEPTION]", e)
            break

    print("SERVER CRASHED")


def start_server():
    SERVER.listen(MAX_CONNETIONS)  # open server to listen for connections
    print("[STARTED] Waiting for HTTP connections...")
    ACCEPT_THREAD = Thread(target=wait_for_connection)
    ACCEPT_THREAD.start()
    ACCEPT_THREAD.join()
    SERVER.close()


def stop_server():
    SERVER.close()


if __name__ == "__main__":
    start_server()
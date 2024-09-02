import os
import sys
import unittest
import time
from threading import Thread
from Client import Client

client = Client()
test_file_name="test.txt"
test_file_data= "Testing the UDP file transfer."
messages=[]


def update_messages():
    """
    updates the local list of messages
    :return: None
    """

    run = True
    while not client.exited:
        # get any new messages from bob
        messages.extend(client.get_messages())

        # update every 1/10 of a second
        time.sleep(0.5)


class MyTestCase(unittest.TestCase):
    def setUp(self) -> None:
        # start thread for receive messages
        Thread(target=update_messages).start()

    def test_client(self):
        if client.isconnected:
            time.sleep(3)
            msg=messages[-2].split()[0]
            self.assertEqual(msg,'App')
            try:
                msg=messages[-1].split()[0]
            except:
                msg=messages[-1]
            self.assertEqual(msg, 'FILES')

            # client sends message for files
            client.send_message("get_files")
            time.sleep(2)
            try:
                msg=messages[-1].split()[0]
            except:
                msg=messages[-1]
            self.assertEqual(msg, 'FILES')
            try:
                sp = messages[-1].split()
                for i in range(1,len(sp)):
                    if sp[i].split(':')[1]==test_file_name:
                        print(f'download {test_file_name}')
                        client.send_message(f'download_file {test_file_name}')
                        time.sleep(2)
                        filepath = f"{os.getcwd()}/Downloads/{test_file_name}"
                        with open(filepath, "r") as file:
                            data = file.read(1000)
                            self.assertEqual(data,test_file_data)
                        break
                    if i == len(sp)-1:
                        print(f"{test_file_name} not exist in the app server")

            except:
                pass

            #disconnect
            client.exit()
            time.sleep(2)
            client.reconnect = False
            client.disconnect()
            print('disconnected')


        else:
            self.assertEqual(messages, [])

        client.exit()
        return




if __name__ == '__main__':
    unittest.main()




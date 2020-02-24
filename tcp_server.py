#!/usr/bin/env python3


import socket
import time


HOST = "127.0.0.1"
PORT = 50003


class SimpleTcpServer(object):
    
    def __init__(self, port):
        self._port = port
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM, socket.IPPROTO_TCP)
    
    def start(self):
        try:
            self._socket.bind((HOST, PORT))
            self._socket.listen(13)
            print("[{}] Started TCP server on port [{}]. Waiting for client connections.".format(time.asctime(), PORT))
            while True:
                client_sock, client_addr = self._socket.accept()
                client_data = client_sock.recv(1024)
                print("[{}] Client {} sent: {}".format(time.asctime(), client_addr, client_data.decode().strip()))
                client_sock.sendall(client_data)
        except Exception as e:
            print("Bind failed due to exception: {}".format(e))
            self._socket.close()        

def main():
    
    tcp_server = SimpleTcpServer(PORT)
    tcp_server.start()
    

if __name__ == "__main__":
    main()

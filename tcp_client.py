#!/usr/bin/env python3

import socket
import sys

# tcp://0.tcp.ngrok.io:11298 

HOST = "0.tcp.ngrok.io"
PORT = 16309

def main():
    
    user_inp = input("Enter text to send, or q to exit: ")
    while user_inp != "q":
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            try:
                sock.connect((HOST, PORT))
                user_inp += "\n"
                sock.sendall(user_inp.encode("utf-8"))
                reply = sock.recv(1024)
                print("Server reply: {}".format(reply.decode().strip()))
                user_inp = input("Enter text to send, or q to exit: ")
            except Exception as e:
                print(e)



if __name__ == "__main__":
    main()

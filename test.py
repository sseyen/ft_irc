import socket
import time

s = socket.socket()
s.connect(("localhost", 6667))

s.sendall(b"PASS 11\r\nNICK Bob\r\nUSER bob 0 * :Bob\r\n")
time.sleep(1)

s.sendall(
    b"PRIVMSG #42 :Hello\r\n"
    b"PRIVMSG #42 :How are you?\r\n"
)

time.sleep(1)
s.close()
import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 1111))

s.sendall(b"NICK bo")
s.sendall(b"b\r\n")

s.close()
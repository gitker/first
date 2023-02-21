import http.server
import socketserver
import hashlib
import ssl
import json
from subprocess import STDOUT, check_output
import random
import socket
from threading import Timer

global cnt
global presharedkey
global sault
global t

def hello():
    global sault
    sault = ''.join(random.sample('zyxwvutsrqponmlkjihgfedcba', 25))
    t = Timer(600, hello)
    t.start()

PORT = 2000

# Store the users in a dictionary where the key is the username
# and the value is the hashed password

t = Timer(600, hello)
t.start()
# Custo request handler
cnt = 0
presharedkey = "pass"
sault = ''.join(random.sample('zyxwvutsrqponmlkjihgfedcba', 25))


class MyTCPServer(socketserver.TCPServer):
    def server_bind(self):
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(self.server_address)
        
class RequestHandler(http.server.BaseHTTPRequestHandler):

    def do_GET(self):
        global cnt
        global presharedkey
        global sault
        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.end_headers()
        self.wfile.write("hello world\n".encode())

    def do_POST(self):
        global cnt
        global presharedkey
        global sault
        content_length = int(self.headers["Content-Length"])
        request_body = self.rfile.read(content_length).decode()
        cnt = cnt +1
        if cnt>100:
            sault =''.join(random.sample('zyxwvutsrqponmlkjihgfedcba', 25))
            cnt = 0
    
        req = json.loads(request_body)
        hk  = req['hash']
        
        if hk != hashlib.md5((presharedkey+sault).encode('utf-8')).hexdigest():
            output = "hash not match \n".encode()
        else:
            output = check_output(req['cmd'].split(), stderr=STDOUT, timeout=10)
       
        self.send_response(200)
        self.send_header("Content-type", "text/plain")
        self.end_headers()
        self.wfile.write(sault.encode())
        self.wfile.write("\n".encode())
        self.wfile.write(output)


# Wrap the socket with a SSL layer

with MyTCPServer(("", PORT), RequestHandler) as httpd:
    print("Serving at port", PORT)
    
    httpd.allow_reuse_address = True
    httpd.socket = ssl.wrap_socket(httpd.socket, certfile="fullchain.pem", keyfile="privkey.pem", server_side=True)
    httpd.serve_forever()

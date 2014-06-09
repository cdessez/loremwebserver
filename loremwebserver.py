#!/usr/bin/python

import sys
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer


CONTENT_FILE_NAME = 'html_random.txt'


class LoremHTTPServer (BaseHTTPRequestHandler):

    def do_GET(self):
        self.lines = None
        with open(CONTENT_FILE_NAME, 'r') as f:
            self.lines = f.readlines()
        self.nlines = len(self.lines)
        self.cpt = 0

        try:
            self.send_response(200)
            self.send_header('Content-type', 'text/htlm')
            self.end_headers()
            self.wfile.write("<html><header><title>Random</title></header><body>\n")
            self.wfile.flush()
            while True:
                self.wfile.write(self.lines[self.cpt])
                self.wfile.flush()
                self.cpt = (self.cpt + 1) % self.nlines
        
        except IOError:
            self.send_error(404, "Content Not Found")


    def do_POST(self):
        self.do_GET()


def main(tcpport=80):
    try:
        server = HTTPServer(('', tcpport), LoremHTTPServer)
        print("Started LoremHTTPServer")
        server.serve_forever()
    except KeyboardInterrupt:
        print("Received ^C: Shutting down...")
        server.socket.close()

if __name__ == '__main__':
    if len(sys.argv) > 1:
        main(int(sys.argv[1]))
    else:
        main()


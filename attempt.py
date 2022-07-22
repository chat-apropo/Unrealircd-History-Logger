################################################################################
# This is an attempt of implementing the same feature as an irc service
# I coudln't figure out how to listen for PRIVMSG in every channel without having to join them
# which was too much of a hack for me.
################################################################################

# Simple log service for unrealircd irc server
# This will store channel messages in a sqlite database and back send to the user on join all that he missed

import datetime
import os
import logging
import socket
import sqlite3
from threading import Lock, Thread

SERVER = "127.0.0.1"
PORT = 7000
SERVER_NAME = "logserv.dot.org.es"
SID = "69F"
UID = SID + "LOGSERV"

HANDSHAKE = "\r\n".join(f'''
PASS :{os.environ['PASS']}
PROTOCTL EAUTH={SERVER_NAME} SID={SID}
PROTOCTL NOQUIT NICKv2 SJOIN SJ3 CLK TKLEXT2 NICKIP ESVID MLOCK EXTSWHOIS SJSBY MTAGS
SERVER {SERVER_NAME} 1 :2309-Mm Logserv IRC Service
EOS
MD client {SID} saslmechlist :PLAIN

'''.split("\n"))

logger = logging.getLogger("logserv")
logging.basicConfig()
logger.setLevel(logging.DEBUG)


class Service:
    def __init__(self, server, port):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((server, port))
        self.sock = sock
        logger.info("Starting handshake")
        self.send(HANDSHAKE)
        logger.info("Connected and listening for messages")
        self.init = False

    def send(self, data):
        logger.debug(f"<----- {data}")
        if not data.endswith("\r\n"):
            data += "\r\n"
        self.sock.send(data.encode())

    def data_handler(self, data):
        logger.debug(f"----> {data}")
        if data.startswith("PING"):
            self.send(data.replace("PING", "PONG", 1))
            return
        if data == ":001 EOS" and not self.init:
            self.init = True
            timestamp = int(datetime.datetime.utcnow().timestamp())
            self.send(f"UID LogServ 0 {timestamp} services irc.dot.org.es {UID} 0 +oiqBS * * * :Log service")
            self.send(f"SJOIN {timestamp-10} #romanian +nt :{UID}")
            self.send(f":{UID} PRIVMSG #romanian :Hello, im actually a service")


def main():
    service = Service(SERVER, PORT)
    while True:
        data = service.sock.recv(1024)
        if not data:
            continue
        for message in data.decode().split("\r\n"):
            service.data_handler(message)
    service.sock.close()


if __name__ == '__main__':
    main()

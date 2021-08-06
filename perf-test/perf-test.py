from socket import *
import sys
import time
import os
from datetime import datetime

def start_test(host, port, rnd, idx):
    buf_size = 2048
    addr = (host, port)
    data = ["/usr/bin/ls\r\n", "/usr/bin/df\r\n", "/usr/bin/date\r\n", "/usr/bin/du\r\n"]
    log_dir = "./output"
    log_file = "%s/test-%d.csv" % (log_dir, idx)
    time_fmt = "%Y-%m-%dT%H:%M:%S"

    f = open(log_file, "w")

    tcpCliSock = socket(AF_INET,SOCK_STREAM)
    connected = False
    while not connected:
        try:
            start = time.time()
            start_str = datetime.now().strftime(time_fmt)
            tcpCliSock.connect(addr)
            usage = time.time() - start
            connected = True
            line = "{start},connect,success,{usage}\n".format(start=start_str, usage=usage)
        except Exception as e:
            print("Connect to remote host failed, errror:%s\n" % str(e))
            line = "{start},connect,failed,{usage}\n".format(start=start_str, usage=usage)

        # f.write(line)

    for i in range(rnd): 
        cmd = data[i%len(data)]
        start = time.time()
        start_str = datetime.now().strftime(time_fmt)
        try:
            tcpCliSock.send(cmd.encode())
            data1 = tcpCliSock.recv(buf_size)
            ret = "success"
        except Exception as e:
            print("Conntion excepted, error:%s" %str(e))
            ret = "failed"

        usage = (time.time() - start) * 1000
        line = "{start},{cmd},{ret},{usage}\n".format(start=start_str, cmd=cmd.rstrip(), ret=ret, usage=usage)
        f.write(line)

        time.sleep(3)

    tcpCliSock.close()
    f.close()


if __name__ == '__main__':
    if len(sys.argv) < 4:
        print("Usage: python %s host port round index" % __file__)
        sys.exit(0)

    host = sys.argv[1]
    port = int(sys.argv[2])
    rnd = int (sys.argv[3])
    idx = int(sys.argv[4])

    start_test(host, port, rnd, idx)
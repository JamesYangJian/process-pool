import sys
import subprocess
import os
import psutil

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print("Usage: %s host port ccu rnd" % __file__)
        sys.exit(0)

    host = sys.argv[1]
    port = int(sys.argv[2])
    ccu = int(sys.argv[3])
    rnd = int(sys.argv[4])
    log_dir = "./output"
    if not os.path.exists(log_dir):
        os.mkdir(log_dir)

    pendings = []
    finished = []

    for i in range(ccu):
        cmd = "python3 ./perf-test.py {h} {p} {r} {i}".format(h=host, p=port, r=rnd, i=i)
        p = subprocess.Popen(cmd, shell=True)
        pendings.append(p.pid)

    while True:
        for pid in pendings:
            try:
                p = psutil.Process(pid)
                if p.status() == 'zombie':
                    os.waitpid(pid, 0)
                    finished.append(pid)
            except Exception as e:
                print("waitpid failed! reason:%s" % str(e))
                finished.append(pid)

        for pid in finished:
            try:
                pendings.remove(pid)
            except Exception as e:
                pass

        if not pendings:
            break

import argparse
from subprocess import *
import sys

def getParameters():
     parser = argparse.ArgumentParser(description='Run hpcstruct on a list of binaries')
     parser.add_argument("--filelist", type=str, help="a list of binary for benchmarking", required=True)
     parser.add_argument("--rep", type=int, default=5, help="the number of iterations to run on each binary")
     parser.add_argument("--thread", type=int, default=16, help="the number of threads")
     parser.add_argument("--start", type=int, default=1, help="From which line of the input file to test")
     args = parser.parse_args()
     return args

def Run(filepath, thread, i):
    cmd = "hpcstruct -j {0} -o tmp.txt {1}".format(thread, filepath)
    p = Popen(cmd, stdout=PIPE, stderr=PIPE,  shell=True)
    msg, err = p.communicate()
    if len(msg.decode()) > 0:
        print (cmd, "has stdout output")
        print (msg.decode())
    if len(err.decode()) > 0:
        print (cmd, "has stderr output")
        print (err.decode())
    if p.returncode < 0:
        print (cmd, "failed with signal", -p.returncode)

args = getParameters()
if args.filelist != None:
    bins = []
    for line in open(args.filelist, "r"):
        bins.append(line[:-1])
else:
    bins = [args.binpath]

cnt = args.start
for filepath in bins[args.start-1:]:
    print (cnt, filepath)
    cnt += 1
    for i in range(args.rep):
        Run(filepath, args.thread, i)

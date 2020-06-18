#!/usr/bin/python3
from multiprocessing import Pool
import subprocess
import sys
# cnfs=["ASG_96_len112_known_last12_2.cnf","eqspwtrc16bparrc16.cnf","Mickey_out250_known_last146_0.cnf","toughsat_23bits_0.cnf","b1904P1-8x8c6h5SAT.cnf","files.txt,MM-23-2-2-2-2-3.cnf","toughsat_25bits_1.cnf","b1904P3-8x8c11h0SAT.cnf","Grain_no_init_ver1_out200_known_last104_0.cnf","QuasiGroup-4-12_c18.cnf","toughsat_28bits_0.cnf","Bibd-sc-10-03-08_c18.cnf","Haystacks-ext-12_c18.cnf","sha1r17m145ABCD.cnf","toughsat_30bits_0.cnf","build_png.py,hcp_bij16_16.cnf","sha1r17m148ABCD_p.cnf","Trivium_no_init_out350_known_last142_1.cnf","hcp_CP20_20.cnf","sha1r17m72a.cnf","eqsparcl10bpwtrc10.cnf","hcp_CP24_24.cnf","size_4_4_4_i0418_r8.cnf","eqspdtlf14bpwtrc14.cnf","knight_20.cnf","size_5_5_5_i003_r12.cnf"]
cnfs = sys.argv[1].split()
print(cnfs)
if len(sys.argv) !=3:
    print("need two args")
    exit(1)
num_cores=int(sys.argv[2])
def run_task(command):
    subprocess.run(command, shell=True)


commands = ["./minisat_release ~/cnfs/"+c +
            " 2>&1 |tail -n 100000 > result_"+c+".txt" for c in cnfs]
print(commands)
with Pool(num_cores) as p:
    p.map(run_task, commands)

import os
import time

N = 3
i = 0
#for i in range(0, N):
while True :
    #print('Now is', i + 1, ', total is', N)
    print('Now is', i + 1)
    i += 1
    os.system('./Threes --total=1000 --play="load=weights.bin save=weights.bin alpha=0.0001" --save=stat.txt')

    print('sleep')
    time.sleep(3)
    print('wake up')

import os
import time

counter = 1
while True:
    print('Now is', counter)
    counter += 1

    os.system('./Threes --total=1000 --play="load=weights.bin save=weights.bin" --evil="load=weights.bin"')

    print('sleep')
    time.sleep(2)
    print('wake up')


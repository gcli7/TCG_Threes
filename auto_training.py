import os
import time

'''
total_game = 5
for i in range(1, total_game + 1):
    print('Now is', i, ', total is', total_game)
    i += 1

    os.system('./Threes --total=1000 --play="load=weights.bin save=weights.bin learning_rate=0.003125"')

    print('sleep')
    time.sleep(2)
    print('wake up')
'''

counter = 1
while True:
    print('Now is', counter)
    counter += 1

    os.system('./Threes --total=1000 --play="load=weights.bin save=weights.bin learning_rate=0.003125"')

    print('sleep')
    time.sleep(2)
    print('wake up')


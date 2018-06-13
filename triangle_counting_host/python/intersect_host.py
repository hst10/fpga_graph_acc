# coding: utf-8

import sys
import numpy as np 
import os
import time
from datetime import datetime
from pynq import Xlnk
from pynq import Overlay

# load our design overlay
overlay = Overlay('intersect_hw.bit')
print("intersect_hw.bit loaded")

myIP = overlay.intersect_0

xlnk = Xlnk()

t1 = time.time()

input_a = xlnk.cma_array(shape=(4096,), dtype=np.int32)
input_b = xlnk.cma_array(shape=(4096,), dtype=np.int32)

for i in range(4096):
	input_a[i] = i
	input_b[i] = i + 1

myIP.write(0x18, input_a.physical_address)
myIP.write(0x20, input_b.physical_address)

myIP.write(0x28, 2)
myIP.write(0x30, 2)


t2 = time.time()
t = t2 - t1
print("Preparing input data time: ", str(t))

isready = 0;
myIP.write(0x00, 1)

while( isready != 6 ):
    isready = myIP.read(0x00)

t3 = time.time()
t = t3 - t2
#tbatch = tbatch + t
#print("Computation finished")
print("PL Time: ", str(t))

print("Return value: ", myIP.read(0x10))


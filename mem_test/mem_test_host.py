# coding: utf-8

import sys
import numpy as np 
import os
import time
from datetime import datetime
from pynq import Xlnk
from pynq import Overlay

# load our design overlay
overlay = Overlay('mem_test.bit')
print("mem_test.bit loaded")

myIP = overlay.mem_test_0

print("myIP.mmio.base_addr = ", myIP.mmio.base_addr)
print("myIP.mmio.length = ", myIP.mmio.length)
print("myIP.mmio.virt_base = ", myIP.mmio.virt_base)
print("myIP.mmio.virt_offset = ", myIP.mmio.virt_offset)

xlnk = Xlnk()

t1 = time.time()

input_data = xlnk.cma_array(shape=(256*512,), dtype=np.int32)

for i in range(256*512):
    input_data[i] = 1

myIP.write(0x18, input_data.physical_address)
print("input_data.physical_address = ", input_data.physical_address)

print("0x00 = ", myIP.read(0x00))

t2 = time.time()
t = t2 - t1
print("Preparing input data time: ", str(t))

isready = 0;
myIP.write(0x00, 1)

while( isready != 6 ):
    print("0x00 = ", isready)
    isready = myIP.read(0x00)

print("Last 0x00 = ", myIP.read(0x00))

t3 = time.time()
t = t3 - t2
#tbatch = tbatch + t
#print("Computation finished")
print("PL Time: ", str(t))

print("Return value: ", myIP.read(0x10))


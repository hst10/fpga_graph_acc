# coding: utf-8

import sys
import numpy as np 
import os
import time
from datetime import datetime
from pynq import Xlnk
from pynq import Overlay

# load our design overlay
overlay = Overlay('triangle_counting.bit')
print("triangle_counting.bit loaded")

myIP = overlay.triangle_counting_0

t0 = time.time()

neighbor_list = []
offset_list = [0]
edge_list = []

graph_file = open("graph/soc-Epinions1_adj.tsv")
# graph_file = open("graph/test.tsv")
lines = graph_file.readlines()

degree_count = 0
prev_node = 0

for line in lines:
    node_a, node_b, _ = map(int, line.split())
    if prev_node != node_b:
        offset_list.append(degree_count)

    prev_node = node_b
    if node_a < node_b:
        edge_list.extend([node_b, node_a])
    else:
        neighbor_list.append(node_a)
        degree_count += 1

offset_list.append(degree_count)

print("neighbor_list size= ", len(neighbor_list))
print("offset_list size= ", len(offset_list))
print("edge_list size= ", len(edge_list))

t1 = time.time()

print("Finished reading graph file. ")
t = t1 - t0
print("Reading input file time: ", str(t))

xlnk = Xlnk()

neighbor = xlnk.cma_array(shape=(len(neighbor_list),), dtype=np.int32)
offset   = xlnk.cma_array(shape=(len(offset_list),), dtype=np.int32)
edge     = xlnk.cma_array(shape=(len(edge_list),), dtype=np.int32)
progress = xlnk.cma_array(shape=(5,), dtype=np.int32)

neighbor[:] = neighbor_list
offset[:] = offset_list
edge[:] = edge_list

# neighbor[:] = [2, 4, 5, 3, 4, 5, 4, 5, 5]
# offset[:]   = [0, 0, 3, 6, 8, 9, 9]
# edge[:]     = [5, 4, 5, 3, 5, 2, 5, 1, 4, 3, 4, 2, 4, 1, 3, 2, 2, 1]

myIP.write(0x18, neighbor.physical_address)
myIP.write(0x20, offset.physical_address)
myIP.write(0x28, edge.physical_address)
myIP.write(0x30, len(edge_list))
myIP.write(0x38, progress.physical_address)

# for i in range(neighbor.size):
#     print("neighbor[%d] = %d" % (i, neighbor[i]))

# for i in range(offset.size):
#     print("offset[%d] = %d" % (i, offset[i]))

# for i in range(edge.size):
#     print("edge[%d] = %d" % (i, edge[i]))

t2 = time.time()
t = t2 - t1
print("Preparing input data time: ", str(t))

isready = 0;
myIP.write(0x00, 1)

while( isready != 6 ):
#    print(progress[0], progress[1], progress[2], progress[3], progress[4])
    isready = myIP.read(0x00)

t3 = time.time()
t = t3 - t2
#tbatch = tbatch + t
#print("Computation finished")
print("PL Time: ", str(t))

print("Return value: ", myIP.read(0x10))

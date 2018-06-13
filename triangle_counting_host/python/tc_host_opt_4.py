# coding: utf-8

import sys
import numpy as np 
import os
import time
import math
from datetime import datetime
from pynq import Xlnk
from pynq import Overlay

# load our design overlay
overlay = Overlay('tc_opt_4.bit')
print("tc_opt_4.bit loaded")

acc0 = overlay.triangle_counting_0
acc1 = overlay.triangle_counting_1
acc2 = overlay.triangle_counting_2
acc3 = overlay.triangle_counting_3

t0 = time.time()

neighbor_list = []
offset_list = [0]
edge_list = []

graph_file = open("../../graph/soc-Epinions1_adj.tsv")
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

num_edge = int(len(edge_list) / 2)
num_batch = 4
num_edge_batch = int(math.floor(float(num_edge) / num_batch))
num_edge_last_batch = num_edge - (num_batch-1)*num_edge_batch

print(num_edge)
print(num_batch)
print(num_edge_batch)
print(num_edge_last_batch)

neighbor = xlnk.cma_array(shape=(len(neighbor_list),), dtype=np.int32)
offset   = xlnk.cma_array(shape=(len(offset_list),), dtype=np.int32)
edge1    = xlnk.cma_array(shape=(2*num_edge_batch,), dtype=np.int32)
edge2    = xlnk.cma_array(shape=(2*num_edge_batch,), dtype=np.int32)
edge3    = xlnk.cma_array(shape=(2*num_edge_batch,), dtype=np.int32)
edge4    = xlnk.cma_array(shape=(2*num_edge_last_batch,), dtype=np.int32)
progress = xlnk.cma_array(shape=(5,), dtype=np.int32)

neighbor[:] = neighbor_list
offset[:] = offset_list
edge1[:] = edge_list[0:2*num_edge_batch]
edge2[:] = edge_list[2*num_edge_batch:4*num_edge_batch]
edge3[:] = edge_list[4*num_edge_batch:6*num_edge_batch]
edge4[:] = edge_list[6*num_edge_batch:]

# neighbor[:] = [2, 4, 5, 3, 4, 5, 4, 5, 5]
# offset[:]   = [0, 0, 3, 6, 8, 9, 9]
# edge[:]     = [5, 4, 5, 3, 5, 2, 5, 1, 4, 3, 4, 2, 4, 1, 3, 2, 2, 1]

acc0.write(0x00018, neighbor.physical_address)
acc0.write(0x00020, offset.physical_address)
acc0.write(0x00028, edge1.physical_address)
acc0.write(0x00030, 2*num_edge_batch)
acc0.write(0x00038, progress.physical_address)

acc1.write(0x00018, neighbor.physical_address)
acc1.write(0x00020, offset.physical_address)
acc1.write(0x00028, edge2.physical_address)
acc1.write(0x00030, 2*num_edge_batch)
acc1.write(0x00038, progress.physical_address)

acc2.write(0x00018, neighbor.physical_address)
acc2.write(0x00020, offset.physical_address)
acc2.write(0x00028, edge3.physical_address)
acc2.write(0x00030, 2*num_edge_batch)
acc2.write(0x00038, progress.physical_address)

acc3.write(0x00018, neighbor.physical_address)
acc3.write(0x00020, offset.physical_address)
acc3.write(0x00028, edge4.physical_address)
acc3.write(0x00030, 2*num_edge_last_batch)
acc3.write(0x00038, progress.physical_address)

# for i in range(neighbor.size):
#     print("neighbor[%d] = %d" % (i, neighbor[i]))

# for i in range(offset.size):
#     print("offset[%d] = %d" % (i, offset[i]))

# for i in range(edge.size):
#     print("edge[%d] = %d" % (i, edge[i]))

t2 = time.time()
t = t2 - t1
print("Preparing input data time: ", str(t))

acc0.write(0x00000, 1)
acc1.write(0x00000, 1)
acc2.write(0x00000, 1)
acc3.write(0x00000, 1)

isready = 0;
while( isready != 6 ):
    isready = acc0.read(0x00000)

isready = 0;
while( isready != 6 ):
    isready = acc1.read(0x00000)

isready = 0;
while( isready != 6 ):
    isready = acc2.read(0x00000)

isready = 0;
while( isready != 6 ):
    isready = acc3.read(0x00000)

t3 = time.time()
t = t3 - t2
#tbatch = tbatch + t
#print("Computation finished")
print("PL Time: ", str(t))

result1 = acc0.read(0x00010)
result2 = acc1.read(0x00010)
result3 = acc2.read(0x00010)
result4 = acc3.read(0x00010)

print("Return value 1: ", result1)
print("Return value 2: ", result2)
print("Return value 3: ", result3)
print("Return value 4: ", result4)
print("Number of triangles: ", result1+result2+result3+result4)

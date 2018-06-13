
neighbor_list = []
offset_list = [0]
edge_list = []

graph_file = open("graph/test.tsv")
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

graph_file.close()

print("neighbor_list size = ", len(neighbor_list))
print("offset_list size = ", len(offset_list))
print("edge_list size = ", len(edge_list))

f = open("test_parsed.tsv", "w")
f.write("%d %d %d\n" % (len(neighbor_list), len(offset_list), len(edge_list)))
f.write(" ".join(str(e) for e in neighbor_list) + "\n")
f.write(" ".join(str(e) for e in offset_list) + "\n")
f.write(" ".join(str(e) for e in edge_list) + "\n")
f.close()

all:
	g++ -std=c++11 truss_decomposition.cc truss_decomposition_host.cc -o td

syn:
	vivado_hls -f run_td_hls.tcl

clean:
	rm -rf td

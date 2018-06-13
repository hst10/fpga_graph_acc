all:
	g++ -O3 tc.cpp -std=c++11 -o tc -L. -lsds_lib
	g++ -O3 tc_1pe.cpp -std=c++11 -o tc_1pe -L. -lsds_lib

cpu:
	g++ -O3 triangle_counting.cc tc_1pe.cpp -std=c++11 -o tc_cpu -L. -lsds_lib

clean:
	rm -rf tc tc_1pe tc_cpu 

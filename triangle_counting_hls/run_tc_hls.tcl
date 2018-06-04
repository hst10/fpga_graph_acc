open_project -reset tc_hls
set_top triangle_counting
add_files triangle_counting.cc
add_files -tb triangle_counting_tb.cc

open_solution -reset "solution1"
set_part {xc7z020clg400-1}
create_clock -period 10

csynth_design
export_design -format ip_catalog -version "0.001"

exit

open_project -reset td_hls
set_top truss_decomposition
add_files truss_decomposition.cc
add_files -tb truss_decomposition_host.cc

open_solution -reset "solution1"
set_part {xc7z020clg400-1}
create_clock -period 10

# csim_design

csynth_design
export_design -format ip_catalog -version "0.001"

exit

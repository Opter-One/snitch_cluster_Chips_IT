onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate /tb_bin/fix/i_snitch_cluster/clk_i
add wave -noupdate /tb_bin/fix/i_snitch_cluster/debug_req_i
add wave -noupdate /tb_bin/fix/i_snitch_cluster/meip_i
add wave -noupdate /tb_bin/fix/i_snitch_cluster/mtip_i
add wave -noupdate /tb_bin/fix/i_snitch_cluster/msip_i
add wave -noupdate /tb_bin/fix/i_snitch_cluster/mxip_i
add wave -noupdate /tb_bin/fix/i_snitch_cluster/hart_base_id_i
add wave -noupdate -divider AXI
add wave -noupdate /tb_bin/fix/i_snitch_cluster/narrow_in_req_i
add wave -noupdate /tb_bin/fix/i_snitch_cluster/narrow_in_resp_o
add wave -noupdate /tb_bin/fix/i_snitch_cluster/narrow_out_resp_i
add wave -noupdate /tb_bin/fix/i_snitch_cluster/narrow_out_req_o
add wave -noupdate /tb_bin/fix/i_snitch_cluster/wide_out_resp_i
add wave -noupdate /tb_bin/fix/i_snitch_cluster/wide_out_req_o
add wave -noupdate /tb_bin/fix/i_snitch_cluster/wide_in_req_i
add wave -noupdate /tb_bin/fix/i_snitch_cluster/wide_in_resp_o
add wave -noupdate -divider TCDM
add wave -noupdate /tb_bin/fix/i_snitch_cluster/i_cluster/tcdm_ext_req_i
add wave -noupdate /tb_bin/fix/i_snitch_cluster/i_cluster/tcdm_ext_resp_o
TreeUpdate [SetDefaultTree]
quietly WaveActivateNextPane
WaveRestoreCursors {{Cursor 1} {348 ps} 0}
quietly wave cursor active 1
configure wave -namecolwidth 150
configure wave -valuecolwidth 100
configure wave -justifyvalue left
configure wave -signalnamewidth 1
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 0
configure wave -gridperiod 1
configure wave -griddelta 40
configure wave -timeline 0
configure wave -timelineunits ps
update
WaveRestoreZoom {0 ps} {8089 ps}

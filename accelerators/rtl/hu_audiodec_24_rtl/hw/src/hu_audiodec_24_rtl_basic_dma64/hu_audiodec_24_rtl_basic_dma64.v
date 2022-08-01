`include "/software/mentor-2022/Catapult/Mgc_home/pkgs/siflibs/ccs_in_wait_v1.v"
`include "/software/mentor-2022/Catapult/Mgc_home/pkgs/siflibs/ccs_out_wait_v1.v"
`include "/software/mentor-2022/Catapult/Mgc_home/pkgs/hls_pkgs/mgc_comps_src/mgc_mul_pipe_beh.v"
`include "/software/mentor-2022/Catapult/Mgc_home/pkgs/ccs_xilinx/hdl/BLOCK_1R1W_RBW.v"
`include "/projects/yingj4/esp-spandex/esp-clean/tech/virtexup/acc/hu_audiodec_24_rtl/hu_audiodec_24_rtl_basic_dma64/rtl.v"
`include "/software/mentor-2022/Catapult/Mgc_home/pkgs/siflibs/ccs_out_buf_wait_v5.v"
`include "/software/mentor-2022/Catapult/Mgc_home/pkgs/siflibs/ccs_ctrl_in_buf_wait_v4.v"

module hu_audiodec_24_rtl_basic_dma64( clk, rst, dma_read_chnl_valid, dma_read_chnl_data, dma_read_chnl_ready,
/* <<--params-list-->> */
conf_info_cfg_regs_31,
conf_info_cfg_regs_30,
conf_info_cfg_regs_26,
conf_info_cfg_regs_27,
conf_info_cfg_regs_24,
conf_info_cfg_regs_25,
conf_info_cfg_regs_22,
conf_info_cfg_regs_23,
conf_info_cfg_regs_8,
conf_info_cfg_regs_20,
conf_info_cfg_regs_9,
conf_info_cfg_regs_21,
conf_info_cfg_regs_6,
conf_info_cfg_regs_7,
conf_info_cfg_regs_4,
conf_info_cfg_regs_5,
conf_info_cfg_regs_2,
conf_info_cfg_regs_3,
conf_info_cfg_regs_0,
conf_info_cfg_regs_28,
conf_info_cfg_regs_1,
conf_info_cfg_regs_29,
conf_info_cfg_regs_19,
conf_info_cfg_regs_18,
conf_info_cfg_regs_17,
conf_info_cfg_regs_16,
conf_info_cfg_regs_15,
conf_info_cfg_regs_14,
conf_info_cfg_regs_13,
conf_info_cfg_regs_12,
conf_info_cfg_regs_11,
conf_info_cfg_regs_10,
conf_done, acc_done, debug, acc_fence_valid, acc_fence_data, acc_fence_ready, dma_read_ctrl_valid, dma_read_ctrl_data_index, dma_read_ctrl_data_length, dma_read_ctrl_data_size, dma_read_ctrl_ready, dma_write_ctrl_valid, dma_write_ctrl_data_index, dma_write_ctrl_data_length, dma_write_ctrl_data_size, dma_write_ctrl_ready, dma_write_chnl_valid, dma_write_chnl_data, dma_write_chnl_ready);

   input clk;
   input rst;

   /* <<--params-def-->> */
   input [31:0]  conf_info_cfg_regs_31;
   input [31:0]  conf_info_cfg_regs_30;
   input [31:0]  conf_info_cfg_regs_26;
   input [31:0]  conf_info_cfg_regs_27;
   input [31:0]  conf_info_cfg_regs_24;
   input [31:0]  conf_info_cfg_regs_25;
   input [31:0]  conf_info_cfg_regs_22;
   input [31:0]  conf_info_cfg_regs_23;
   input [31:0]  conf_info_cfg_regs_8;
   input [31:0]  conf_info_cfg_regs_20;
   input [31:0]  conf_info_cfg_regs_9;
   input [31:0]  conf_info_cfg_regs_21;
   input [31:0]  conf_info_cfg_regs_6;
   input [31:0]  conf_info_cfg_regs_7;
   input [31:0]  conf_info_cfg_regs_4;
   input [31:0]  conf_info_cfg_regs_5;
   input [31:0]  conf_info_cfg_regs_2;
   input [31:0]  conf_info_cfg_regs_3;
   input [31:0]  conf_info_cfg_regs_0;
   input [31:0]  conf_info_cfg_regs_28;
   input [31:0]  conf_info_cfg_regs_1;
   input [31:0]  conf_info_cfg_regs_29;
   input [31:0]  conf_info_cfg_regs_19;
   input [31:0]  conf_info_cfg_regs_18;
   input [31:0]  conf_info_cfg_regs_17;
   input [31:0]  conf_info_cfg_regs_16;
   input [31:0]  conf_info_cfg_regs_15;
   input [31:0]  conf_info_cfg_regs_14;
   input [31:0]  conf_info_cfg_regs_13;
   input [31:0]  conf_info_cfg_regs_12;
   input [31:0]  conf_info_cfg_regs_11;
   input [31:0]  conf_info_cfg_regs_10;
   input 	 conf_done;
   input     acc_fence_ready;

   input 	 dma_read_ctrl_ready;
   output 	 dma_read_ctrl_valid;
   output [31:0] dma_read_ctrl_data_index;
   output [31:0] dma_read_ctrl_data_length;
   output [2:0]  dma_read_ctrl_data_size;

   output 	 dma_read_chnl_ready;
   input 	 dma_read_chnl_valid;
   input [63:0]  dma_read_chnl_data;

   input 	 dma_write_ctrl_ready;
   output 	 dma_write_ctrl_valid;
   output [31:0] dma_write_ctrl_data_index;
   output [31:0] dma_write_ctrl_data_length;
   output [2:0]  dma_write_ctrl_data_size;

   input 	 dma_write_chnl_ready;
   output 	 dma_write_chnl_valid;
   output [63:0] dma_write_chnl_data;

   output 	 acc_done;
   output [31:0] debug;
   output    acc_fence_valid;
   output [1:0]  acc_fence_data;

   reg 		 acc_done;   

   // assign dma_read_ctrl_valid = 1'b0;
   // assign dma_read_chnl_ready = 1'b1;
   // assign dma_write_ctrl_valid = 1'b0;
   // assign dma_write_chnl_valid = 1'b0;
   assign debug = 32'd0;
 
   assign acc_fence_valid = 1'd0;
   assign acc_fence_data = 2'd0;
   // assign acc_done = conf_done;
   
   
   Top_rtl hu_audiodec_24_Top_rtl_inst (
      .clk(clk), .rst(rst), 
      .cfg_regs_0  (conf_info_cfg_regs_0  ), 
      .cfg_regs_1  (conf_info_cfg_regs_1  ), 
      .cfg_regs_2  (conf_info_cfg_regs_2  ), 
      .cfg_regs_3  (conf_info_cfg_regs_3  ), 
      .cfg_regs_4  (conf_info_cfg_regs_4  ), 
      .cfg_regs_5  (conf_info_cfg_regs_5  ),
      .cfg_regs_6  (conf_info_cfg_regs_6  ), 
      .cfg_regs_7  (conf_info_cfg_regs_7  ), 
      .cfg_regs_8  (conf_info_cfg_regs_8  ), 
      .cfg_regs_9  (conf_info_cfg_regs_9  ), 
      .cfg_regs_10 (conf_info_cfg_regs_10 ), 
      .cfg_regs_11 (conf_info_cfg_regs_11 ), 
      .cfg_regs_12 (conf_info_cfg_regs_12 ),
      .cfg_regs_13 (conf_info_cfg_regs_13 ), 
      .cfg_regs_14 (conf_info_cfg_regs_14 ), 
      .cfg_regs_15 (conf_info_cfg_regs_15 ), 
      .cfg_regs_16 (conf_info_cfg_regs_16 ), 
      .cfg_regs_17 (conf_info_cfg_regs_17 ), 
      .cfg_regs_18 (conf_info_cfg_regs_18 ),
      .cfg_regs_19 (conf_info_cfg_regs_19 ), 
      .cfg_regs_20 (conf_info_cfg_regs_20 ), 
      .cfg_regs_21 (conf_info_cfg_regs_21 ), 
      .cfg_regs_22 (conf_info_cfg_regs_22 ), 
      .cfg_regs_23 (conf_info_cfg_regs_23 ), 
      .cfg_regs_24 (conf_info_cfg_regs_24 ),
      .cfg_regs_25 (conf_info_cfg_regs_25 ), 
      .cfg_regs_26 (conf_info_cfg_regs_26 ), 
      .cfg_regs_27 (conf_info_cfg_regs_27 ), 
      .cfg_regs_28 (conf_info_cfg_regs_28 ), 
      .cfg_regs_29 (conf_info_cfg_regs_29 ), 
      .cfg_regs_30 (conf_info_cfg_regs_30 ),
      .cfg_regs_31 (conf_info_cfg_regs_31 ), 
      .acc_start  (conf_done),
      .acc_done   (acc_done), 
      .dma_read_ctrl_val (dma_read_ctrl_valid), 
      .dma_read_ctrl_rdy (dma_read_ctrl_ready), 
      .dma_read_ctrl_msg ( {dma_read_ctrl_data_size, dma_read_ctrl_data_length, dma_read_ctrl_data_index} )  , 
      .dma_read_chnl_val (dma_read_chnl_valid),
      .dma_read_chnl_rdy (dma_read_chnl_ready), 
      .dma_read_chnl_msg ( dma_read_chnl_data ), 
      .dma_write_ctrl_val (dma_write_ctrl_valid), 
      .dma_write_ctrl_rdy (dma_write_ctrl_ready),
      .dma_write_ctrl_msg ( {dma_write_ctrl_data_size, dma_write_ctrl_data_length, dma_write_ctrl_data_index} ), 
      .dma_write_chnl_val (dma_write_chnl_valid), 
      .dma_write_chnl_rdy (dma_write_chnl_ready), 
      .dma_write_chnl_msg (dma_write_chnl_data)
   );

endmodule
module hu_audiodec_rtl_basic_dma64( clk, rst, dma_read_chnl_valid, dma_read_chnl_data, dma_read_chnl_ready,
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
conf_done, acc_done, debug, dma_read_ctrl_valid, dma_read_ctrl_data_index, dma_read_ctrl_data_length, dma_read_ctrl_data_size, dma_read_ctrl_ready, dma_write_ctrl_valid, dma_write_ctrl_data_index, dma_write_ctrl_data_length, dma_write_ctrl_data_size, dma_write_ctrl_ready, dma_write_chnl_valid, dma_write_chnl_data, dma_write_chnl_ready);

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

   reg 		 acc_done;   

   assign dma_read_ctrl_valid = 1'b0;
   assign dma_read_chnl_ready = 1'b1;
   assign dma_write_ctrl_valid = 1'b0;
   assign dma_write_chnl_valid = 1'b0;
   assign debug = 32'd0;

   assign acc_done = conf_done;
   
endmodule

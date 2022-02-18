-- Copyright 1986-2016 Xilinx, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2016.2 (win64) Build 1577090 Thu Jun  2 16:32:40 MDT 2016
-- Date        : Sat Feb 12 13:03:58 2022
-- Host        : nick-desktop running 64-bit major release  (build 9200)
-- Command     : write_vhdl -force -mode synth_stub -rename_top design_1_xlconstant_1_1 -prefix design_1_xlconstant_1_1_
--               design_1_xlconstant_0_0_stub.vhdl
-- Design      : design_1_xlconstant_0_0
-- Purpose     : Stub declaration of top-level module interface
-- Device      : xc7z010clg400-1
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity \design_1_xlconstant_1_1\ is
  Port ( 
    dout : out STD_LOGIC_VECTOR ( 0 to 0 )
  );

end \design_1_xlconstant_1_1\;

architecture stub of \design_1_xlconstant_1_1\ is
attribute syn_black_box : boolean;
attribute black_box_pad_pin : string;
attribute syn_black_box of stub : architecture is true;
attribute black_box_pad_pin of stub : architecture is "dout[0:0]";
begin
end;

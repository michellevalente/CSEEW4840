// CSEE 4840 Lab1: Display and modify the contents of memory
//
// Spring 2014
//
// By: Ariel Faria, Michelle Valente
// Uni: af2791, ma3360

module lab1(input logic       clk,
            input logic [3:0] KEY,
            output [7:0]      hex0, hex2, hex3);

   logic [3:0] 		      a;         // Address
   logic [7:0] 		      din, dout; // RAM data in and out
   logic 		      we;        // RAM write enable
   logic clk2;
	
   hex7seg h0( .a(a),         .y(hex0) ),
           h2( .a(dout[7:4]), .y(hex2) ),
           h3( .a(dout[3:0]), .y(hex3) );
			  
   controller c( .* ); // Connect everything with matching names
   memory m( .* );
	divider d( .* );
  
endmodule

module divider ( clk ,clk2 );

output clk2 ;
reg clk2 ;

input clk ;
wire clk ;

reg [3:0] m;

initial m = 0;

always @ (posedge (clk)) begin
if (m<9)
m <= m + 1;
else
m <= 0;
end

always @ (m) begin
if (m<5)
clk2 <= 1;
else
clk2 <= 0;
end


endmodule


module controller(input logic        clk2,
		  input logic [3:0]  KEY,
		  input logic [7:0]  dout,
		  output logic [3:0] a,
		  output logic [7:0] din,
		  output logic 	     we);
		  
	reg temp = 0;
	
	always_ff @(posedge clk2)
		begin		
		   if (KEY[3] == 0 & KEY[2] == 1 & temp == 0)
			begin
				a <= a + 1'b1;
				temp = 1; end
			else if(KEY[2] == 0 & KEY[3] == 1 & temp == 0)
				begin
				a <= a - 1'b1;
				temp = 1;
				end
			else if(KEY[1] == 0 & KEY[0] == 1 & temp == 0)
				begin
					din <= dout + 8'b00000001;
					temp = 1;
					we = 1;
				end
			else if(KEY[0] == 0 & KEY[1] == 1 & temp == 0)
				begin
					din <= dout - 8'b00000001;
					temp = 1;
					we = 1;
				end
			else if(KEY[0] == 1 & KEY[1] == 1 & KEY[2] == 1 & KEY[3] == 1 & temp == 1)
				begin
					we = 0;
					temp = 0;
				end			
		  end
		  
endmodule
		  
module hex7seg(input logic [3:0] a,
	       output logic [7:0] y);

   always_comb
	  case (a)
	    4'd0: y = 8'b0011_1111;
       4'd1: y = 8'b0000_0110;
		 4'd2: y = 8'b0101_1011;
		 4'd3: y = 8'b0100_1111;
		 4'd4: y = 8'b0110_0110;
		 4'd5: y = 8'b0110_1101;
		 4'd6: y = 8'b0111_1101;
		 4'd7: y = 8'b0000_0111;
       4'd8: y = 8'b0111_1111;
		 4'd9: y = 8'b0110_1111;
		 4'd10: y = 8'b0111_0111;
		 4'd11: y = 8'b0111_1100;
		 4'd12: y = 8'b0011_1001;
		 4'd13: y = 8'b0101_1110;
		 4'd14: y = 8'b0111_1001;
		 4'd15: y = 8'b0111_0001;
	  endcase	
		 
	
endmodule

// 16 X 8 synchronous RAM with old data read-during-write behavior
module memory(input logic        clk2,
	      input logic [3:0]  a,
	      input logic [7:0]  din,
	      input logic 	 we,
	      output logic [7:0] dout);
   
   logic [7:0] 			 mem [15:0];

   always_ff @(posedge clk2) begin
      if (we) mem[a] <= din;
      dout <= mem[a];
   end
        
endmodule


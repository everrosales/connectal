// This is a simple Ethernet module that will just interface with
// the AXI Ethernet Lite IP Core on the FPGA
// I dont really need a separate Eth module since I am directly interfacing with
// the axi interface
import FIFO::*;
import AxiEthLite::*;


interface EthMaster;
  (* prefix ="" *)
  interface AxiethlitePhy pins;

  method Action request(Bit#(4) wen, Bit#(32) addr, Bit#(32) data);
  method Action deq();
  method Tuple2#(Bool, Bit#(32)) first(); 
  
endinterface


module mkEthMaster(EthMaster);
  AxiEthLite ethLite <- mkAxiEthLite;

  // Make a dwire that does a thing
  Wire#(Bit#(13)) araddr <- mkDWire(0);
  Wire#(Bit#(1)) arvalid <- mkDWire(0);
  Wire#(Bit#(13)) awaddr <- mkDWire(0);
  Wire#(Bit#(1)) awvalid <- mkDWire(0);
  Wire#(Bit#(1)) bready  <- mkDWire(0);
  Wire#(Bit#(1)) rready  <- mkDWire(0);
  Wire#(Bit#(32)) wdata  <- mkDWire(0);
  Wire#(Bit#(4)) wstrb   <- mkDWire(0);
  Wire#(Bit#(1)) wvalid  <- mkDWire(0);
  
  // Add book keeping fifo
  // Check if the read
  FIFO#(Bool) bookkeeperWasWrite <- mkFIFO;

  (* no_implicit_conditions *)
  rule readPins;
    ethLite.s_axi.araddr(araddr);
    ethLite.s_axi.arvalid(arvalid);
    ethLite.s_axi.awaddr(awaddr);
    ethLite.s_axi.awvalid(awvalid);
    ethLite.s_axi.bready(bready);
    ethLite.s_axi.rready(rready);
    ethLite.s_axi.wdata(wdata);
    ethLite.s_axi.wstrb(wstrb);
    ethLite.s_axi.wvalid(wvalid);
  endrule

  method Action request(Bit#(4) wen, Bit#(32) addr, Bit#(32) data) if (ethLite.s_axi.awready == 1 && ethLite.s_axi.arready == 1);
    if (wen != 0) begin
      let mask = 32'b0;
      if (wen[3] != 0) begin
        mask[31:24] = 8'b11111111;
      end
      if (wen[2] != 0) begin
        mask[23:16] = 8'b11111111;
      end
      if (wen[1] != 0) begin
        mask[15:8] = 8'b11111111;
      end
      if (wen[0] != 0) begin
        mask[7:0]  = 8'b11111111;
      end
      // If this is a write make sure you use waddr
      // Write address to wraddr
      awaddr <= truncate(addr);
      wdata <= data & mask;
      awvalid <= 1'b1;
      //ethLite.s_axi.awready(1'b1);
      bookkeeperWasWrite.enq(True);
    end else begin
      araddr <= truncate(addr);
      arvalid <= 1'b1;
      bookkeeperWasWrite.enq(False);
    end
  endmethod
  
  method Action deq if (ethLite.s_axi.bvalid == 1 && ethLite.s_axi.rvalid == 1);
    if (ethLite.s_axi.bvalid == 1 && bookkeeperWasWrite.first()) begin
      // This was a write 
      bready <= 1'b0;
      bookkeeperWasWrite.deq;
    end else if (ethLite.s_axi.rvalid == 1 && !bookkeeperWasWrite.first()) begin
      // This was a read
      rready <= 1'b0;
      bookkeeperWasWrite.deq;
    end else begin
      // Do nothing... sorry bud
      $display("You should never be here\n");
    end
  endmethod

  method Tuple2#(Bool, Bit#(32)) first() if (ethLite.s_axi.bvalid == 1 && ethLite.s_axi.rvalid == 1);
    if (ethLite.s_axi.bvalid == 1 && bookkeeperWasWrite.first()) begin
      return tuple2(True, {30'b0, ethLite.s_axi.bresp}); 
    end else if (ethLite.s_axi.rvalid == 1 && !bookkeeperWasWrite.first()) begin
      return tuple2(True, ethLite.s_axi.rdata);
    end else begin
      return tuple2(False, 32'b0);
    end
  endmethod  
  
  interface AxiethlitePhy pins = ethLite.phy;
endmodule

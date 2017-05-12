// This is a simple Ethernet module that will just interface with
// the AXI Ethernet Lite IP Core on the FPGA
// I dont really need a separate Eth module since I am directly interfacing with
// the axi interface
import FIFO::*;
import AxiEthLite::*;


interface EthMasterPins;
  (* prefix = "phy" *)
  interface AxiethlitePhy pins;
  interface Clock deleteme_unused_clock;
  
endinterface

interface EthMaster;
  interface EthMasterPins pins;

  method Action request(Bit#(4) wen, Bit#(32) addr, Bit#(32) data);
  method Action deq();
  method Tuple2#(Bool, Bit#(32)) first(); 
  
endinterface


module mkEthMaster(EthMaster);
  let clock <- exposeCurrentClock();
  AxiEthLite ethLite <- mkAxiEthLite;

  Reg#(Bit#(1)) intc <- mkReg(0);

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
    $display("reading pins\n");
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

  rule readInt;
    intc <= ethLite.ip2.intp;
  endrule

  method Action request(Bit#(4) wen, Bit#(32) addr, Bit#(32) data) if (ethLite.s_axi.wready == 1 && ethLite.s_axi.arready == 1 && ethLite.s_axi.awready == 1);
    $display("Eth: Beginning request\n");

    if (wen != 0) begin
      // If this is a write make sure you use waddr
      // Write address to wraddr
      awaddr <= truncate(addr);
      wstrb <= wen;
      wdata <= data;
      awvalid <= 1'b1;
      wvalid <= 1'b1;
      //ethLite.s_axi.awready(1'b1);
      $display("enqueuing bookkeeper\n");
      bookkeeperWasWrite.enq(True);
    end else begin
      araddr <= truncate(addr);
      arvalid <= 1'b1;
      wstrb <= wen;
      bookkeeperWasWrite.enq(False);
    end
  endmethod
 
  method Action deq if ((ethLite.s_axi.bvalid == 1 && bookkeeperWasWrite.first) || (ethLite.s_axi.rvalid == 1 && !bookkeeperWasWrite.first));
    if (ethLite.s_axi.bvalid == 1 && bookkeeperWasWrite.first) begin
      // This was a write 
      bready <= 1'b0;
      bookkeeperWasWrite.deq;
    end else if (ethLite.s_axi.rvalid == 1 && !bookkeeperWasWrite.first) begin
      // This was a read
      rready <= 1'b0;
      bookkeeperWasWrite.deq;
    end else begin
      // Do nothing... sorry bud
      $display("You should never be here\n");
    end
  endmethod

  method Tuple2#(Bool, Bit#(32)) first() if ((ethLite.s_axi.bvalid == 1 && bookkeeperWasWrite.first) || (ethLite.s_axi.rvalid == 1 && !bookkeeperWasWrite.first));
    if (ethLite.s_axi.bvalid == 1 && bookkeeperWasWrite.first()) begin
      return tuple2(True, {30'b0, ethLite.s_axi.bresp}); 
    end else if (ethLite.s_axi.rvalid == 1 && !bookkeeperWasWrite.first()) begin
      return tuple2(True, ethLite.s_axi.rdata);
    end else begin
      return tuple2(False, 32'b0);
    end
  endmethod  
  
  interface EthMasterPins pins;
    interface AxiethlitePhy pins = ethLite.phy;
    interface Clock deleteme_unused_clock = clock;
  endinterface
endmodule

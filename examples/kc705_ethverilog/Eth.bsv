// This is a simple Ethernet module that will just interface with
// the AXI Ethernet Lite IP Core on the FPGA
// I dont really need a separate Eth module since I am directly interfacing with
// the axi interface
import FIFO::*;
import FIFOF::*;
import TriState::*;
import AxiEthLite::*;


typedef struct {
  Bit#(13) awaddr;
  Bit#(1) awvalid;
} WriteAddrChan deriving (Bits, Eq);

typedef struct {
  Bit#(32) wdata;
  Bit#(4)  wstrb;
  Bit#(1)  wvalid;
} WriteDataChan deriving (Bits, Eq);

typedef struct {
  Bit#(2) bresp;
} WriteResponseChan deriving (Bits, Eq);

typedef struct {
  Bit#(13)  araddr;
  Bit#(1)   arvalid;
} ReadAddrChan deriving (Bits, Eq);

typedef struct {
  Bit#(32)  rdata;
  Bit#(1)   rvalid;

} ReadDataChan deriving(Bits, Eq);
  

interface MDIOPins;
  method Inout#(Bit#(1)) mdio;
endinterface


interface EthMasterPins;
  (* prefix = "phy" *)
  interface AxiethlitePhy pins;

  (* prefix = "mdio" *)
  interface MDIOPins mdio_pins;
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

  TriState#(Bit#(1)) mdio <- mkTriState(ethLite.mdio.mdio_t == 0, ethLite.mdio.mdio_o);


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
  Wire#(Bit#(1)) mdio_i  <- mkDWire(0);

  // Add book keeping fifo
  // Check if the read
  FIFO#(Bool) bookkeeperWasWrite <- mkFIFO;
  // All the fifos for the axi channels
  FIFO#(WriteAddrChan) wachan <- mkFIFO;
  FIFO#(WriteDataChan) wdchan <- mkFIFO;
  FIFOF#(WriteResponseChan) wrchan <- mkFIFOF;
  FIFO#(ReadAddrChan) rachan <- mkFIFO;
  FIFOF#(ReadDataChan) rdchan <- mkFIFOF;

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
    ethLite.mdio.mdio_i(mdio._read);
  endrule

  rule readInt;
    intc <= ethLite.ip2.intp;
  endrule

  rule doWriteAddr;
    let pending_awaddr = wachan.first.awaddr;
    let pending_awvalid = wachan.first.awvalid;

    awaddr <= pending_awaddr;
    awvalid <= pending_awvalid;
  endrule

  rule deqWriteAddr (ethLite.s_axi.awready == 1);
    wachan.deq;
  endrule

  rule doWriteData;
    let pending_wdata = wdchan.first.wdata;
    let pending_wvalid = wdchan.first.wvalid;
    let pending_wstrb = wdchan.first.wstrb;

    wdata <= pending_wdata;
    wvalid <= pending_wvalid;
    wstrb <= pending_wstrb;

  endrule

  rule deqWriteData (ethLite.s_axi.wready == 1);
    wdchan.deq;
  endrule

  rule doWriteResp (ethLite.s_axi.bvalid == 1);
     wrchan.enq(WriteResponseChan{bresp: ethLite.s_axi.bresp});
    // let pending_bresp = wrchan.first.bresp;
    // let pending_bvalid = wrchan.first.bvalid;

    // bresp <= pending_bresp;
    // bvalid <= pending_bvalid;
  endrule

  rule deqWriteResp (wrchan.notFull);
    bready <= 1'b1;  
  endrule
     
  rule doReadAddr;
    let pending_araddr = rachan.first.araddr;
    let pending_arvalid = rachan.first.arvalid;

    araddr <= pending_araddr;
    arvalid <= pending_arvalid;
  endrule

  rule deqReadAddr (ethLite.s_axi.arready == 1);
    rachan.deq;
  endrule

  rule doReadData (ethLite.s_axi.rvalid == 1);
    rdchan.enq(ReadDataChan{rdata: ethLite.s_axi.rdata, rvalid: ethLite.s_axi.rvalid});
    //  let pending_rdata = rdchan.first.rdata;
    //  let pending_rvalid = rdchan.first.rvalid;
    //  rdata <= pending_rdata;
    //  rvalid <= pending_rvalid;
  endrule

  rule deqReadData (rdchan.notFull);
    rready <= 1'b1; 
  endrule


  method Action request(Bit#(4) wen, Bit#(32) addr, Bit#(32) data);
    if (wen != 0) begin
      // If this is a write make sure you use waddr
      // Write address to wraddr
      wachan.enq(WriteAddrChan{awaddr: truncate(addr), awvalid: 1'b1});
      wdchan.enq(WriteDataChan{wstrb: wen, wdata: data, wvalid: 1'b1});
      //ethLite.s_axi.awready(1'b1);
      $display("enqueuing bookkeeper\n");
      bookkeeperWasWrite.enq(True);
    end else begin
      rachan.enq(ReadAddrChan{araddr: truncate(addr), arvalid: 1'b1});
      bookkeeperWasWrite.enq(False);
    end
  endmethod
 
  method Action deq;
    if (bookkeeperWasWrite.first) begin
      // This was a write 
      //bready <= 1'b1;
      wrchan.deq;
      bookkeeperWasWrite.deq;
    end else if (!bookkeeperWasWrite.first) begin
      // This was a read
      //rready <= 1'b1;
      rdchan.deq;
      bookkeeperWasWrite.deq;
    end else begin
      // Do nothing... sorry bud
      $display("You should never be here\n");
    end
  endmethod

  method Tuple2#(Bool, Bit#(32)) first if (bookkeeperWasWrite.first || !bookkeeperWasWrite.first);
    if (bookkeeperWasWrite.first()) begin
      return tuple2(True, {30'b0, wrchan.first.bresp}); 
    end else begin
      return tuple2(False, rdchan.first.rdata);
    end
  endmethod  
  
  interface EthMasterPins pins;
    interface AxiethlitePhy pins = ethLite.phy;
    interface Clock deleteme_unused_clock = clock;
    interface MDIOPins mdio_pins;
      method Inout#(Bit#(1)) mdio;
        return mdio.io;
      endmethod
    endinterface
  endinterface
endmodule

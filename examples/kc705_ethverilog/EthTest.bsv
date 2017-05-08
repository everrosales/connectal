import Eth::*;

import MemTypes::*;
import AxiBits::*;
import AxiDma::*;

//import EthBvi::*;

interface EthTest;
  interface EthMasterPins ethMasterPins;
endinterface

interface EthRequest;
  method Action reset();
  method Action request(Bit#(4) wen, Bit#(32) addr, Bit#(32)data);
endinterface

interface EthIndication;
  method Action resetDone();
  method Action response(Bool write, Bit#(32) data);
endinterface

module mkEthTest#(EthIndication ethIndication) (EthTest);
  EthMaster eth <- mkEthMaster;
  Reg#(Bool) init <- mkReg(False);
  Reg#(Bool) active <- mkReg(False);
  Bool verbose = False;


  rule doInit(!init);
    // Do initialization if needed
  endrule

  interface EthRequest ethRequest;
    method Action reset();
      $display("recieved reset request");
    endmethod

    method Action request(Bit#(8) wen, Bit#(32) addr, Bit#(32) data);
      eth.request(truncate(wen), addr, data);
    endmethod
  endinterface


  interface EthMasterPins ethMasterPins = eth.pins;
endmodule

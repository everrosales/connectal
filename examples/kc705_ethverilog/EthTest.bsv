import Eth::*;

import MemTypes::*;
import AxiBits::*;
import AxiDma::*;
import AxiEthLite::*;
import Clocks::*;


//import EthBvi::*;

interface EthTest;
  interface EthMasterPins ethMasterPins;
  interface EthRequest ethRequest;
endinterface

interface EthRequest;
  method Action reset();
  method Action request(Bit#(8) wen, Bit#(32) addr, Bit#(32)data);
endinterface

interface EthIndication;
  method Action resetDone();
  method Action response(Bool write, Bit#(32) data);
endinterface

module mkEthTest#(EthIndication ethIndication) (EthTest);
  Reg#(Bool) init <- mkReg(False);
  Reg#(Bool) active <- mkReg(False);

  let clock <- exposeCurrentClock;
  let reset <- mkReset(10, True, clock);
  Reg#(Bool) resetSent <- mkReg(False);

  EthMaster eth <- mkEthMaster(reset_by reset.new_rst);

  Bool verbose = False;

  rule doInit(!init);
    // Do initialization if needed
  endrule


  rule connectionIndication;
     ethIndication.response(tpl_1(eth.first), tpl_2(eth.first));
     eth.deq();
  endrule

  rule finishReset(resetSent && (!reset.isAsserted));
    resetSent <= False;
    ethIndication.resetDone;

  endrule

  interface EthRequest ethRequest;
    method Action reset();
      $display("recieved reset request");
      // resets the processor
      reset.assertReset();
      // flushes the pending memory requests
      resetSent <= True;
    endmethod

    method Action request(Bit#(8) wen, Bit#(32) addr, Bit#(32) data);
      $display("recieved data request\n");
      eth.request(truncate(wen), addr, data);
      $display("finished data request\n");
    endmethod
  endinterface

  interface EthMasterPins ethMasterPins = eth.pins;
endmodule

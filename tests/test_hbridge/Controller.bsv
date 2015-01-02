
// Copyright (c) 2014 Quanta Research Cambridge, Inc.

// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

import Leds::*;
import Vector::*;
import FIFOF::*;
import Arith::*;

interface HBridge2;
   method Bit#(2) hbridge0();
   method Bit#(2) hbridge1();
endinterface

interface HBridgeCtrlRequest;
   method Action ctrl(Vector#(2,Bit#(11)) power, Vector#(2,Bit#(1)) direction);
endinterface

typedef enum {Stopped, Started} HBridgeCtrlEvent deriving (Eq,Bits);

interface HBridgeCtrlIndication;
   method Action hbc_event(Bit#(32) e);
endinterface

interface Controller;
   interface HBridgeCtrlRequest req;
   interface HBridge2 pins;
   interface LEDS leds;
endinterface

module mkController#(HBridgeCtrlIndication ind)(Controller);
   
   Vector#(2, Reg#(Bit#(1))) direction <- replicateM(mkReg(0));
   Vector#(2, Reg#(Bit#(1)))   enabled <- replicateM(mkReg(0));
   Vector#(2, Reg#(Bit#(11)))    power <- replicateM(mkReg(0));
   Vector#(2, Reg#(Bool))           pz <- replicateM(mkReg(True));
   FIFOF#(Bit#(32))         event_fifo <- mkSizedFIFOF(4);
   Bit#(8) leds_val =  extend({direction[0],direction[1]});  
   
   // frequency of design: 100 mHz  
   // frequency of PWM System: 2 kHz 
   // 2k design cycles == 1 PWM cycle
   Reg#(Bit#(11)) fcnt <- mkReg(0);
      
   rule detect_event;
      Vector#(2,Bool) npz;
      for(int i = 0; i < 2; i=i+1) 
	 npz[i] = power[i]==0;
      Bool started =  fold(booland, readVReg(pz)) && !fold(booland, npz);
      Bool stopped = !fold(booland, readVReg(pz)) &&  fold(booland, npz);
      Bit#(32) e = 0;
      e = e | (extend(pack(stopped)) << pack(Stopped));
      e = e | (extend(pack(started)) << pack(Started));
      if (e != 0 && event_fifo.notFull) 
	 event_fifo.enq(e);
      writeVReg(pz,npz);
   endrule
   
   rule report_event;
      ind.hbc_event(event_fifo.first);
      event_fifo.deq;
   endrule
   
   rule pwm;
      for(Integer i = 0; i < 2; i=i+1)
	 enabled[i] <= ((power[i] > 0) && (fcnt <= power[i])) ? 1 : 0;
      fcnt <= fcnt+1;
   endrule
   
   interface HBridgeCtrlRequest req;
      method Action ctrl(Vector#(2,Bit#(11)) p, Vector#(2,Bit#(1)) d);
	 for(Integer i = 0; i < 2; i=i+1) begin
	    direction[i] <= d[i];
	    power[i]     <= p[i];
	 end
      endmethod
   endinterface
   
   interface HBridge2 pins;
      method Bit#(2) hbridge0();
	 return {enabled[0],direction[0]};
      endmethod
      method Bit#(2) hbridge1();
	 return {enabled[1],direction[1]};
      endmethod
   endinterface
   
   interface LEDS leds;
      method Bit#(LedsWidth) leds() = leds_val;
   endinterface

endmodule

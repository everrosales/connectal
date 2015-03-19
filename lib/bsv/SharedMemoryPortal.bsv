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

import Vector::*;
import GetPut::*;
import FIFO::*;
import FIFOF::*;
import GetPut::*;
import ClientServer::*;
import CtrlMux::*;
import Gearbox::*;
import Clocks::*;

import MIFO::*;
import Pipe::*;
import Portal::*;
import MemTypes::*;
import AddressGenerator::*;
import MemTypes::*;
import MemreadEngine::*;
import MemwriteEngine::*;
import ConnectalMemory::*;

typedef enum {
   Idle,
   HeadRequested,
   TailRequested,
   RequestMessage,
   MessageHeaderRequested,
   MessageRequested,
   Drain,
   UpdateTail,
   UpdateHead,
   UpdateHead2,
   Waiting,
   SendHeader,
   SendMessage,
   SendPadding,
   Stop
   } SharedMemoryPortalState deriving (Bits,Eq);

module mkSharedMemoryRequestPortal#(PipePortal#(numRequests, numIndications, 32) portal,
				    MemreadServer#(64) readEngine,
				    MemwriteServer#(64) writeEngine)(SharedMemoryPortal#(64));
      // read the head and tail pointers, if they are different, then read a request
      Reg#(Bit#(32)) reqLimitReg <- mkReg(0);
      Reg#(Bit#(32)) reqHeadReg <- mkReg(0);
      Reg#(Bit#(32)) reqTailReg <- mkReg(0);
      Reg#(Bit#(16)) wordCountReg <- mkReg(0);
      Reg#(Bit#(16)) messageWordsReg <- mkReg(0);
      Reg#(Bit#(16)) methodIdReg <- mkReg(0);
      Reg#(SharedMemoryPortalState) reqState <- mkReg(Idle);

   Reg#(Bit#(32)) sglIdReg <- mkReg(0);
   Reg#(Bool)     readyReg   <- mkReg(False);

   let verbose = False;

      rule updateReqHeadTail if (reqState == Idle && readyReg);
	 readEngine.cmdServer.request.put(MemengineCmd { sglId: sglIdReg,
							base: 0,
							burstLen: 16,
							len: 16,
							tag: 0});
	 reqState <= HeadRequested;
      endrule

      rule receiveReqHeadTail if (reqState == HeadRequested || reqState == TailRequested);
	 let data <- toGet(readEngine.dataPipe).get();
	 let w0 = data[31:0];
	 let w1 = data[63:32];
	 let head = reqHeadReg;
	 let tail = reqTailReg;
	 if (reqState == HeadRequested) begin
	    reqLimitReg <= w0;
	    reqHeadReg <= w1;
	    head = w1;
	    reqState <= TailRequested;
	 end
	 else begin
	    if (reqTailReg == 0) begin
	       tail = w0;
	       reqTailReg <= tail;
	    end
	    if (tail != reqHeadReg)
	       reqState <= RequestMessage;
	    else
	       reqState <= Idle;
	 end
	 if (verbose)
	    $display("receiveReqHeadTail state=%d w0=%x w1=%x head=%d tail=%d limit=%d", reqState, w0, w1, head, tail, reqLimitReg);
      endrule

      rule requestMessage if (reqState == RequestMessage);
	 Bit#(32) wordCount = reqHeadReg - reqTailReg;
	 if ((reqTailReg & 1) == 1) begin
	    $display("WARNING requestMessage: reqTail=%d is odd.", reqTailReg);
	 end
	 let tail = reqTailReg + wordCount;
	 if (reqHeadReg < reqTailReg) begin
	    $display("requestMessage wrapped: head=%d tail=%d", reqHeadReg, reqTailReg);
	    wordCount = reqLimitReg - reqTailReg;
	    tail = 4;
	 end
	 if (verbose) $display("requestMessage id=%d tail=%h head=%h wordCount=%d", sglIdReg, reqTailReg, reqHeadReg, wordCount);

	 reqTailReg <= tail;
	 wordCountReg <= truncate(wordCount);
	 readEngine.cmdServer.request.put(MemengineCmd {sglId: sglIdReg,
							base: extend(reqTailReg << 2),
							burstLen: 16,
							len: wordCount << 2,
							tag: 0});
	 reqState <= MessageHeaderRequested;
      endrule

      MIFO#(4,1,4,Bit#(32)) readMifo <- mkMIFO();
      rule demuxwords if (readMifo.enqReady());
	 let data <- toGet(readEngine.dataPipe).get();
	 Vector#(2,Bit#(32)) dvec = unpack(data);
	 let enqCount = 2;
	 Vector#(4,Bit#(32)) dvec4;
	 dvec4[0] = dvec[0];
	 dvec4[1] = dvec[1];
	 dvec4[2] = 0;
	 dvec4[3] = 0;
	 readMifo.enq(enqCount, dvec4);
      endrule

      rule receiveMessageHeader if (reqState == MessageHeaderRequested);
	 let vec <- toGet(readMifo).get();
	 let hdr = vec[0];
	 let methodId = hdr[31:16];
	 let messageWords = hdr[15:0];
	 methodIdReg <= methodId;
	 if (verbose)
	    $display("receiveMessageHeader hdr=%x methodId=%x messageWords=%d wordCount=%d", hdr, methodId, messageWords, wordCountReg);
	 wordCountReg <= wordCountReg - 1;
	 messageWordsReg <= messageWords - 1;
	 if (hdr == 0) begin
	    if (wordCountReg == 1)
	       reqState <= UpdateTail;
	    else
	       reqState <= Drain;
	 end
	 else if (wordCountReg == 1)
	    reqState <= UpdateTail;
	 else if (messageWords == 1)
	    reqState <= MessageHeaderRequested;
	 else
	    reqState <= MessageRequested;
      endrule

      rule drain if (reqState == Drain);
	 let vec <- toGet(readMifo).get();
	 if (wordCountReg == 1)
	    reqState <= UpdateTail;
	 wordCountReg <= wordCountReg - 1;
      endrule

      rule receiveMessage if (reqState == MessageRequested);
	 let vec <- toGet(readMifo).get();
	 let data = vec[0];
	 if (verbose)
	    $display("receiveMessage data=%x messageWords=%d wordCount=%d", data, messageWordsReg, wordCountReg);
	 if (methodIdReg != 16'hFFFF)
	    portal.requests[methodIdReg].enq(data);

	 messageWordsReg <= messageWordsReg - 1;
	 wordCountReg <= wordCountReg - 1;
	 if (messageWordsReg == 1)
	    reqState <= MessageHeaderRequested;
	 else if (wordCountReg <= 1)
	    reqState <= UpdateTail;
      endrule

      rule updateTail if (reqState == UpdateTail);
	 if (verbose)
	    $display("updateTail: tail=%d", reqTailReg);
	 // update the tail pointer
	 writeEngine.cmdServer.request.put(MemengineCmd {sglId: sglIdReg,
							 base: 8,
							 len: 8,
							 burstLen: 8,
							 tag: 0});
	 writeEngine.dataPipe.enq(extend(reqTailReg));
	 reqState <= Waiting;
      endrule
      rule waiting if (reqState == Waiting);
	 let done <- writeEngine.cmdServer.response.get();
	 reqState <= Idle;
      endrule

      rule consumeResponse;
	 let response <- readEngine.cmdServer.response.get();
      endrule
   interface SharedMemoryPortalConfig cfg;
      method Action setSglId(Bit#(32) id);
	 sglIdReg <= id;
	 readyReg <= True;
      endmethod
   endinterface
endmodule

module mkSharedMemoryIndicationPortal#(PipePortal#(numRequests, numIndications, 32) portal,
				       MemreadServer#(64) readEngine,
				       MemwriteServer#(64) writeEngine)(SharedMemoryPortal#(64));
      // read the head and tail pointers, if they are different, then read a request
      Reg#(Bit#(16)) indLimitReg <- mkReg(0);
      Reg#(Bit#(16)) indHeadReg <- mkReg(0);
      Reg#(Bit#(16)) indTailReg <- mkReg(0);
      Reg#(Bit#(16)) wordCountReg <- mkReg(0);
      Reg#(Bit#(16)) messageWordsReg <- mkReg(0);
      Reg#(Bit#(16)) methodIdReg <- mkReg(0);
      Reg#(Bool) paddingReg <- mkReg(False);
      Reg#(SharedMemoryPortalState) indState <- mkReg(Idle);

   Reg#(Bit#(32)) sglIdReg <- mkReg(0);
   Reg#(Bool)     readyReg   <- mkReg(False);

   let verbose = True;

   function Bool pipeOutNotEmpty(PipeOut#(a) po); return po.notEmpty(); endfunction
   Vector#(numIndications, Bool) readyBits = map(pipeOutNotEmpty, portal.indications);
   Bool      interruptStatus = False;
   Bit#(16)  readyChannel = -1;
   for (Integer i = valueOf(numIndications) - 1; i >= 0; i = i - 1) begin
      if (readyBits[i]) begin
         interruptStatus = True;
         readyChannel = fromInteger(i);
      end
   end

      rule updateIndHeadTail if (indState == Idle && readyReg);
	 readEngine.cmdServer.request.put(MemengineCmd { sglId: sglIdReg,
							base: 0,
							burstLen: 16,
							len: 16,
							tag: 0});
	 indState <= HeadRequested;
      endrule

      rule receiveIndHeadTail if (indState == HeadRequested || indState == TailRequested);
	 let data <- toGet(readEngine.dataPipe).get();
	 let w0 = data[31:0];
	 let w1 = data[63:32];
	 let head = indHeadReg;
	 let tail = indTailReg;
	 if (indState == HeadRequested) begin
	    indLimitReg <= truncate(w0);
	    indHeadReg <= truncate(w1);
	    head = truncate(w1);
	    indState <= TailRequested;
	 end
	 else begin
	    if (indTailReg == 0) begin
	       tail = truncate(w0);
	       indTailReg <= tail;
	    end
	    //if (tail != indHeadReg)
	       indState <= SendHeader;
	    //else
	       //indState <= Idle;
	 end
	 if (verbose)
	    $display("receiveIndHeadTail state=%d w0=%x w1=%x head=%d tail=%d limit=%d", indState, w0, w1, head, tail, indLimitReg);
      endrule

   let defaultClock <- exposeCurrentClock;
   let defaultReset <- exposeCurrentReset;
   Gearbox#(1,2,Bit#(32)) gb <- mk1toNGearbox(defaultClock, defaultReset, defaultClock, defaultReset);
   rule send64bits;
      let v = gb.first;
      gb.deq();
      writeEngine.dataPipe.enq(pack(v));
   endrule
   rule sendHeader if (indState == SendHeader && interruptStatus);
      Bit#(16) messageBits = portal.messageSize(readyChannel);
      Bit#(16) roundup = messageBits[4:0] == 0 ? 0 : 1;
      Bit#(16) numWords = (messageBits >> 5) + roundup;
      Bit#(16) totalWords = numWords + 1;
      if (numWords[0] == 0)
	 totalWords = numWords + 2;
      paddingReg <= numWords[0] == 0;
      Bit#(32) hdr = extend(readyChannel) << 16 | extend(numWords + 1);
      $display("sendHeader hdr=%h messageBits=%d numWords=%d totalWords=%d paddingReg=%d indHeadReg=%h", hdr, messageBits, numWords, totalWords, paddingReg, indHeadReg);

      indHeadReg <= indHeadReg + totalWords;
      messageWordsReg <= numWords;
      methodIdReg <= readyChannel;
      gb.enq(replicate(hdr));
      writeEngine.cmdServer.request.put(MemengineCmd { sglId: sglIdReg,
						      base: extend(indHeadReg) << 2,
						      burstLen: 8,
						      len: extend(totalWords) << 2, tag: 0 });
      indState <= SendMessage;
   endrule
   rule sendMessage if (indState == SendMessage);
      messageWordsReg <= messageWordsReg - 1;
      let v = portal.indications[methodIdReg].first;
      portal.indications[methodIdReg].deq();
      gb.enq(replicate(v));
      $display("sendMessage v=%h messageWords=%d", v, messageWordsReg);

      if (messageWordsReg == 1) begin
	 if (paddingReg)
	    indState <= SendPadding;
	 else
	    indState <= UpdateHead;
      end
   endrule
   rule sendPadding if (indState == SendPadding);
      $display("sendPadding");
      gb.enq(replicate(32'hffff0001));
      indState <= UpdateHead;
   endrule
   rule updateHead if (indState == UpdateHead);
      $display("updateIndHead limit=%d head=%d", indLimitReg, indHeadReg);
      gb.enq(replicate(extend(indLimitReg)));
      writeEngine.cmdServer.request.put(MemengineCmd { sglId: sglIdReg,
						      base: 0 << 2,
						      burstLen: 8,
						      len: 2 << 2, tag: 0 });
      indState <= UpdateHead2;
   endrule
   rule updateHead2 if (indState == UpdateHead2);
      $display("updateIndHead2");
      gb.enq(replicate(extend(indHeadReg)));
      indState <= SendHeader;
   endrule
   rule done;
      let done <- writeEngine.cmdServer.response.get();
   endrule
   interface SharedMemoryPortalConfig cfg;
      method Action setSglId(Bit#(32) id);
	 sglIdReg <= id;
	 readyReg <= True;
      endmethod
   endinterface

endmodule

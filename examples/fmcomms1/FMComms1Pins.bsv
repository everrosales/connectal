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
//import Vector::*;
//import GetPut::*;
//import Connectable :: *;
//import Clocks :: *;
//import FIFO::*;
//import Portal::*;
//import ConnectalConfig::*;
//import CtrlMux::*;
//import MemTypes::*;
//import MemServer::*;
//import MMU::*;
//import PS7LIB::*;
//import PPS7LIB::*;
//import FMComms1Request::*;
//import FMComms1Indication::*;
//import MemServerRequest::*;
//import MMURequest::*;
//import MemServerIndication::*;
//import MMUIndication::*;
//import BlueScopeEventPIORequest::*;
//import BlueScopeEventPIOIndication::*;
//import XilinxCells::*;
//import ConnectalXilinxCells::*;
//import ConnectalClocks::*;
//import BlueScopeEventPIO::*;
import FMComms1ADC::*;
import FMComms1DAC::*;
//import FMComms1::*;
//import extraXilinxCells::*;

/* Duplicate def here, this is also in ZynqTop */
interface I2C_Pins;
   interface Inout#(Bit#(1)) scl;
   interface Inout#(Bit#(1)) sda;
endinterface

interface FMComms1Pins;
   interface FMComms1ADCPins adcpins;
   interface FMComms1DACPins dacpins;
   interface I2C_Pins         i2c1;
   method Bit#(1) ad9548_ref_p();
   method Bit#(1) ad9548_ref_n();
//   (* prefix="" *)
endinterface

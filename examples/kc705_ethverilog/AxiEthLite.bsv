
/*
   ./../../generated/scripts/importbvi.py
   -P
   AxiEthLite
   -I
   AxiEthLite
   -c
   s_axi_aclk
   -r
   s_axi_aresetn
   -o
   AxiEthLite.bsv
   ethernetlite_stub.v
*/

import Clocks::*;
import DefaultValue::*;
import XilinxCells::*;
import GetPut::*;
//import AxiBits::*;

(* always_ready *)
interface AxiethliteIp;
  method Bit#(1) intp;
endinterface

(* always_ready, always_enabled *)
interface AxiethlitePhy;
    method Action      col(Bit#(1) v);
    method Action      crs(Bit#(1) v);
    method Action      dv(Bit#(1) v);
    method Bit#(1)     rst_n();
    method Action      rx_clk(Bit#(1) v);
    method Action      rx_data(Bit#(4) v);
    method Action      rx_er(Bit#(1) v);
    method Action      tx_clk(Bit#(1) v);
    method Bit#(4)     tx_data();
    method Bit#(1)     tx_en();
endinterface
(* always_ready, always_enabled *)
interface AxiethliteS_axi;
    method Action      araddr(Bit#(13) v);
    method Bit#(1)     arready();
    method Action      arvalid(Bit#(1) v);
    method Action      awaddr(Bit#(13) v);
    method Bit#(1)     awready();
    method Action      awvalid(Bit#(1) v);
    method Action      bready(Bit#(1) v);
    method Bit#(2)     bresp();
    method Bit#(1)     bvalid();
    method Bit#(32)     rdata();
    method Action      rready(Bit#(1) v);
    method Bit#(2)     rresp();
    method Bit#(1)     rvalid();
    method Action      wdata(Bit#(32) v);
    method Bit#(1)     wready();
    method Action      wstrb(Bit#(4) v);
    method Action      wvalid(Bit#(1) v);
endinterface
(* always_ready, always_enabled *)
interface AxiEthLite;
    interface AxiethliteIp     ip2;
    interface AxiethlitePhy     phy;
    interface AxiethliteS_axi     s_axi;
endinterface
import "BVI" ethernetlite =
module mkAxiEthLite (AxiEthLite);
    default_clock s_axi_aclk();
    default_reset s_axi_aresetn();
        //input_clock s_axi_aclk(s_axi_aclk) = s_axi_aclk;
        //input_clock (phy_rx_clk) = phy_rx_clk;
        //input_clock (phy_tx_clk) = phy_tx_clk; 
        //input_reset s_axi_aclk_reset() = s_axi_aclk_reset; /* from clock*/
        //input_reset s_axi_aresetn(s_axi_aresetn) = s_axi_aresetn;
    interface AxiethliteIp     ip2;
      // Do the interupt thing  
      method ip2intc_irpt intp();
    endinterface
    interface AxiethlitePhy     phy;
        method col(phy_col) enable((*inhigh*) EN_phy_col);
        method crs(phy_crs) enable((*inhigh*) EN_phy_crs);
        method dv(phy_dv) enable((*inhigh*) EN_phy_dv);
        method phy_rst_n rst_n();
        method rx_clk(phy_rx_clk) enable((*inhigh*) EN_phy_rx_clk) clocked_by(no_clock);
        method rx_data(phy_rx_data) enable((*inhigh*) EN_phy_rx_data) clocked_by(no_clock);
        method rx_er(phy_rx_er) enable((*inhigh*) EN_phy_rx_er);
        method tx_clk(phy_tx_clk) enable((*inhigh*) EN_phy_tx_clk);
        method phy_tx_data tx_data();
        method phy_tx_en tx_en();
    endinterface
    interface AxiethliteS_axi     s_axi;
        method araddr(s_axi_araddr) clocked_by (s_axi_aclk) enable((*inhigh*) EN_s_axi_araddr);
        method s_axi_arready arready() clocked_by (s_axi_aclk);
        method arvalid(s_axi_arvalid) clocked_by (s_axi_aclk) enable((*inhigh*) EN_s_axi_arvalid);
        method awaddr(s_axi_awaddr) clocked_by (s_axi_aclk) enable((*inhigh*) EN_s_axi_awaddr);
        method s_axi_awready awready() clocked_by (s_axi_aclk);
        method awvalid(s_axi_awvalid) clocked_by (s_axi_aclk) enable((*inhigh*) EN_s_axi_awvalid);
        method bready(s_axi_bready) clocked_by (s_axi_aclk) enable((*inhigh*) EN_s_axi_bready);
        method s_axi_bresp bresp() clocked_by (s_axi_aclk) ;
        method s_axi_bvalid bvalid() clocked_by (s_axi_aclk);
        method s_axi_rdata rdata() clocked_by (s_axi_aclk);
        method rready(s_axi_rready) clocked_by (s_axi_aclk) enable((*inhigh*) EN_s_axi_rready);
        method s_axi_rresp rresp() clocked_by (s_axi_aclk);
        method s_axi_rvalid rvalid() clocked_by (s_axi_aclk);
        method wdata(s_axi_wdata) clocked_by (s_axi_aclk) enable((*inhigh*) EN_s_axi_wdata);
        method s_axi_wready wready() clocked_by (s_axi_aclk);
        method wstrb(s_axi_wstrb) clocked_by (s_axi_aclk) enable((*inhigh*) EN_s_axi_wstrb);
        method wvalid(s_axi_wvalid) clocked_by (s_axi_aclk) enable((*inhigh*) EN_s_axi_wvalid);
    endinterface
    schedule (phy.col, phy.crs, phy.dv, phy.rst_n, phy.rx_data, phy.rx_er, phy.rx_clk, phy.tx_clk, phy.tx_data, phy.tx_en, s_axi.araddr, s_axi.arready, s_axi.arvalid, s_axi.awaddr, s_axi.awready, s_axi.awvalid, s_axi.bready, s_axi.bresp, s_axi.bvalid, s_axi.rdata, s_axi.rready, s_axi.rresp, s_axi.rvalid, s_axi.wdata, s_axi.wready, s_axi.wstrb, s_axi.wvalid) CF (phy.col, phy.crs, phy.dv, phy.rst_n, phy.rx_clk, phy.tx_clk, phy.rx_data, phy.rx_er, phy.tx_data, phy.tx_en, s_axi.araddr, s_axi.arready, s_axi.arvalid, s_axi.awaddr, s_axi.awready, s_axi.awvalid, s_axi.bready, s_axi.bresp, s_axi.bvalid, s_axi.rdata, s_axi.rready, s_axi.rresp, s_axi.rvalid, s_axi.wdata, s_axi.wready, s_axi.wstrb, s_axi.wvalid);
endmodule

/*
instance ToAxi4SlaveBits#(Axi4SlaveLiteBits#(12,32), AxiethliteS_axi);
  function Axi4SlaveLiteBits#(12,32) toAxi4SlaveBits(AxiuartS_axi s);
    return (interface  Axi4SlaveLiteBits#(12, 32);
    method Action araddr(Bit#(12) addr); s.araddr(extend(addr)); endmethod
   method arready = s.arready;
   method arvalid = s.arvalid;
   method Action awaddr(Bit#(12) addr); s.awaddr(extend(addr)); endmethod
   method awready = s.awready;
   method awvalid = s.awvalid;
   method bready = s.bready;
   method bresp = s.bresp;
   method bvalid = s.bvalid;
   method rdata = s.rdata;
   method rready = s.rready;
   method rresp = s.rresp;
   method rvalid = s.rvalid;
   method wdata = s.wdata;
   method wready = s.wready;
   method Action      wvalid(Bit#(1) v);
      s.wvalid(v);
      s.wstrb(pack(replicate(v)));
   endmethod
   endinterface);
    endfunction
  endinstance
*/

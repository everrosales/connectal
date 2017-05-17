source "board.tcl" 
source "$connectaldir/../fpgamake/tcl/ipcore.tcl"

fpgamake_ipcore axi_ethernetlite 3.0 ethernetlite [list \
                            CONFIG.C_INCLUDE_MDIO {1} \
                            CONFIG.AXI_ACLK_FREQ_MHZ {100} \
                            ]


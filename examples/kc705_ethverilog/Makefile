
CONNECTALDIR?=../..

BSVFILES = EthTest.bsv
CPPFILES = eth.cpp

S2H_INTERFACES = EthRequest:EthTest.ethRequest
H2S_INTERFACES = EthTest:EthIndication
MEM_READ_INTERFACES =
MEM_WRITE_INTERFACES =

PINOUT_FILE += $(PROJDIR)/pin_translation.json
PIN_TYPE = AxiethlitePhy
PIN_TYPE_INCLUDE = AxiEthLite
AUTOTOP = --interface pins:EthTest.ethMasterPins

include $(CONNECTALDIR)/Makefile.connectal

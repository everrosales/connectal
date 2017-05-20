
CONNECTALDIR?=../..

BSVFILES = EthTest.bsv
CPPFILES = eth.cpp

S2H_INTERFACES = EthRequest:EthTest.ethRequest
H2S_INTERFACES = EthTest:EthIndication
MEM_READ_INTERFACES =
MEM_WRITE_INTERFACES =

PINOUT_FILE +=pin_translation.json
PIN_TYPE = EthMasterPins
PIN_TYPE_INCLUDE = Eth
AUTOTOP = --interface pins:EthTest.ethMasterPins


CONNECTALFLAGS += --xci=$(IPDIR)/$(BOARD)/ethernetlite/ethernetlite.xci

include $(CONNECTALDIR)/Makefile.connectal


typedef enum {IfcNamesNone=0,
PlatformIfcNames_MemServerRequestS2H=1,
PlatformIfcNames_MMURequestS2H=2,
PlatformIfcNames_MemServerIndicationH2S=3,
PlatformIfcNames_MMUIndicationH2S=4,
IfcNames_EthIndicationH2S=5,
IfcNames_EthRequestS2H=6
} IfcNames deriving (Eq,Bits);

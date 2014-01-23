
1. source embedded/environment-c6x
3. make

usage: ./mainstream <mainstream.ini>

mainstream.ini content:

[VARIABLE]

fpgaNumber=0x2
dmaChannel=0x0
adcMask=0x1
adcFreq=500000000
dmaBlockSize=0x10000
dmaBlockCount=0x4
dmaBuffersCount=16
testMode=0x0

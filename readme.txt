
1. source embedded/environment-c6x
2. create dirs ./libs_x86 and ./libs_c6x and place libgipcy.a and 
   libncurses.a (for c6x only) for respective platform
3. make clean && make

usage: ./mainstream <mainstream.ini>

mainstream.ini content:

[VARIABLE]

fpgaNumber=0x0
dmaChannel=0x0
adcMask=0xf
adcFreq=500000000
dmaBlockSize=0x10000
dmaBlockCount=0x4
dmaBuffersCount=16
testMode=0x0

sync_mode=0x3
sync_fd=488.72
sync_fo=56.0
sync_selclkout=0x0

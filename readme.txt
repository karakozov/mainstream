
1. source embedded/environment-c6x
2. create dirs ./libs_x86 and ./libs_c6x and place libgipcy.a and 
   libncurses.a (for c6x only) for respective platform
3. make clean && make

4. For setting up RX destination for TX channel use file tx.channels.
   * - mean that entry not used
   - - mean delay
   0..3 - mean destination RX FPGA number

   tx.channels format

 BRD0TX0 BRD0RX BRD1RX BRD2RX BRD3RX
 BRD0TX1 BRD0RX BRD1RX BRD2RX BRD3RX
 BRD1TX0 BRD1RX BRD2RX BRD3RX BRD0RX
 BRD1TX1 BRD1RX BRD2RX BRD3RX BRD0RX
 BRD2TX0 BRD2RX BRD3RX BRD0RX BRD1RX
 BRD2TX1 BRD2RX BRD3RX BRD0RX BRD1RX
 BRD3TX0 BRD3RX BRD0RX BRD1RX BRD2RX
 BRD3TX1 BRD3RX BRD0RX BRD1RX BRD2RX

usage: ./mainstream <mainstream.ini>

mainstream.ini content:

[VARIABLE]

boardMask=0x3F        - bitmask of using boards in slots: [slot6 slot5 slot4 slot3 slot2 slot1]
fpgaMask=0x3          - used FPGA on board [0x1 - FPGA0, 0x2 - FPGA1]
dmaChannel=0x0        - used DMA channel on FPGA [0x1 or 0x2]
adcMask=0xf           - used ADC channels
adcFreq=250000000     - not used and will be romoved [backward capabilities]
dmaBlockSize=0x10000  - size of memory block for DMA chain in bytes
dmaBlockCount=0x4     - total DMA blocks in DMA chain
dmaBuffersCount=16    - for DDR3 data storage - number of DMA buffers stored in DDR3.
                        Total amount of memory is: dmaBlockSize*dmaBlockCount*dmaBuffersCount
testMode=0x3          - type of system test.
                        0x0 -> DATA FROM STREAM ADC
                        0x1 -> DDR3 FPGA AS MEMORY
                        0x2 -> DDR3 FPGA AS FIFO
                        0x3 -> PCIE TEST
syncMode=0x3          - sync working mode
syncSelClkOut=0x1     - sync param
syncFd=10000000       - sync param
syncFo=10000000       - sync param

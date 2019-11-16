This branch currently contains the IO from Network integrated on the Petalinux platform. 
It also supports fully software implementation of the compression process.
Also supports a HW function which does the LZW functionality on FPGA. 

#define HWIMPL ----> define this MACRO for enabling all HW implementations.
#define USING_LZW_HW ---> defines the LZW HW
#define MEASURING LATENCY ---> enables profiling and prints values to stdout 
#if READING_FROM_SERVER ---> To enable Network as IO, else uses Filesystem as IO(only for Baremetal platform)

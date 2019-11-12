The following code passes RTL/Co-simulation test with latency of 16386 cycles. 
This is due to an II of 2 and 8192 max loop iterations.

However there are a couple of problems with data transfer lengths.

Input Buffer length should be variable of the form buf[0:len], however len value can vary at runtime, which makes HLS angry. Need to ask TA's. 

Also since the top function main loop can break when mask value has been hit, it should exit, however this creates a cyclic data dependency for the loop condition giving an II=5. This also needs to be discussed with TAs. As of now, I am forcefully doing NOPs even after the mask has been hit to create a consistent loop iteration of 8192 every single time. 

Also there seems to be a limit to the array size that can be malloces as RTL/Co-sim was throwing an SDS::bad_alloc error for 270MB allocation. 

learning stuff and trying things out

TAP must be started with:  
`sudo ip link del tap0 2>/dev/null`  
`sudo ip tuntap add dev tap0 mode tap` to create tap0  
`sudo ip link set tap0 up` "turn on" the tap0  
`sudo ip route add 10.0.0.0/24 dev tap0` route traffic by ip to tap0  

afterwards `ping 10.0.0.1` works to test it  

for compiling  
`cmake -S . -B build`  
`cmake --build build`  

>It works on my machine...  
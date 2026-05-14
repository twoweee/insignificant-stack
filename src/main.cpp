#include <iostream>
#include <array>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <algorithm>
#include <vector>
#include <memory>
#include <iomanip>

#include <sys/epoll.h>

#include "Args.hpp"
#include "PacketReader.hpp"

int main(int argc, char* argv[]) {
    Args passedArgs; 
    passedArgs.parse(argc, argv);
    std::unique_ptr<PacketReader> packetReader;
    try {
        packetReader = std::make_unique<PacketReader>(passedArgs);
    } 
    catch (const std::runtime_error& e) {
        std::cerr << "Caught runtime_error: " << e.what() << "\n";
        return 1; 
    }

    std::array<uint8_t, 2048> buffer;

    if (packetReader->getVerbose()) std::cout << packetReader->toString();

    while (1) {
        std::cout << "Waiting... \n";
        int readyEvents = packetReader->epollWait();

        if (readyEvents < 0) {
            std::cerr << "epoll_wait failed: " << strerror(errno) << "\n"; 
            break;
        }  

        std::cout << "Events inbound: " << readyEvents << "\n";

        while(1) {
            int n = packetReader->readPacket(buffer.data(), buffer.size());

            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                std::cerr << "Read error: " << strerror(errno) << "\n";
                break;
            }

            std::cout << "Received packet: " << n << " bytes\n";

            std::ostringstream log;

            for (int j = 0; j < n; j++) {
                log << std::hex
                    << std::setw(2)
                    << std::setfill('0')
                    << (int)buffer[j] << " ";

                if ((j + 1) % 8 == 0)
                    log << "\n";
            }
            std::string result = log.str();
            std::cout << result << "\n\n";
        }
    }
    
}
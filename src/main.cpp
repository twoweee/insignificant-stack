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
#include <span>
#include <arpa/inet.h>

#include <sys/epoll.h>

#include "Args.hpp"
#include "PacketReader.hpp"

// this is just types
#include "Frames.hpp"

int appendBytesToStringStream(std::ostringstream& stream, uint8_t* buf, int len, int lineLen){
    for (int j = 0; j < len; j++) {
        stream << std::hex
            << std::setw(2)
            << std::setfill('0')
            << (int)buf[j] << " ";

        if ((j + 1) % lineLen == 0)
            stream << "\n"; 
    }
    return 0;
}

uint16_t checksum16(const void *data, size_t len)
{
    const uint8_t *ptr = static_cast<const uint8_t *>(data);
    uint32_t sum = 0;

    while (len > 1) {
        uint16_t word;
        std::memcpy(&word, ptr, sizeof(word));
        sum += word;
        ptr += 2;
        len -= 2;
    }

    if (len == 1) {
        sum += (*ptr) << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return static_cast<uint16_t>(~sum);
}

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

    if (packetReader->getVerbose()) std::cout << packetReader->toString();

    std::array<uint8_t, 6> myMac = packetReader->getMyMac();



    while (1) {
        if (packetReader->getVerbose()) {
            std::cout << "Waiting... \n";
        }
        int readyEvents = packetReader->epollWait();

        if (readyEvents < 0) {
            std::cerr << "epoll_wait failed: " << strerror(errno) << "\n"; 
            break;
        }  

        std::cout << "Events inbound: " << readyEvents << "\n";

        while(1) {
            std::array<uint8_t, 2048> buffer;
            int packetSize = packetReader->readPacket(buffer.data(), buffer.size());

            if (packetSize < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                std::cerr << "Read error: " << strerror(errno) << "\n";
                break;
            }

            std::ostringstream log;

            ethernet_header_t ethernetHeader = {};
            std::memcpy(&ethernetHeader, buffer.data(), sizeof(ethernet_header_t));

            bool printed = false;

            if (ethernetHeader.etherType == 0x0608){ // ARP
                eth_arp_frame_t frame = {};
                std::memcpy(&frame, buffer.data(), std::min(buffer.size(), sizeof(eth_arp_frame_t)));
                
                log << "Received ARP packet: " << packetSize << " bytes\n";
                if (packetReader->getVerbose()) {
                    appendBytesToStringStream(log, buffer.data(), packetSize, 8);
                } else {
                    appendBytesToStringStream(log, buffer.data(), ((packetSize > 4) ? 4 : packetSize), 8);
                    log << "...";
                }
                log << "\n";
                printed = true;

                eth_arp_frame_t responseFrame = {};
                std::memcpy(&responseFrame, &frame, sizeof(eth_arp_frame_t));

                responseFrame.arp.opcode = 0x0200;

                // ETH MAC 
                std::memcpy(&responseFrame.eth.dstMac, &frame.eth.srcMac, 6);
                std::memcpy(&responseFrame.eth.srcMac, myMac.data(), 6);

                // ARP MAC
                std::memcpy(&responseFrame.arp.dstMac, &frame.arp.srcMac, 6);
                std::memcpy(&responseFrame.arp.srcMac, &frame.arp.srcMac, 6); 

                // ARP IP
                std::memcpy(&responseFrame.arp.srcIP, &frame.arp.dstIP, 4);
                std::memcpy(&responseFrame.arp.dstIP, &frame.arp.srcIP, 4);

                int sentSize = packetReader->writePacket((unsigned char*)&responseFrame, sizeof(eth_arp_frame_t));
                log << "Sent " << sentSize << " bytes\n";
            } else if (ethernetHeader.etherType == 0x0008 ) {
                ipv4_header_t ipv4header = {};
                std::memcpy(&ipv4header, buffer.data() + sizeof(ethernet_header_t), sizeof(ipv4_header_t));

                if (ipv4header.protocol == 0x01 ) { 
                    eth_ipv4_icmp_frame_t frame = {};
                    std::memcpy(&frame, buffer.data(), std::min(buffer.size(), sizeof(eth_ipv4_icmp_frame_t)));
                    
                    if (frame.icmp.type == 0x08){ // PING
                        int icmpPayloadSize = packetSize - sizeof(eth_ipv4_icmp_frame_t);

                        log << "Received PING packet: " << packetSize << " bytes\n";
                        if (packetReader->getVerbose()) {
                            appendBytesToStringStream(log, buffer.data(), packetSize, 8);
                        } else {
                            appendBytesToStringStream(log, buffer.data(), ((packetSize > 4) ? 4 : packetSize), 8);
                            log << "...";
                        }
                        log << "\n";
                        printed = true;

                        // some checks
                        // uint16_t inIpv4Check = checksum16(&frame.ipv4, sizeof(ipv4_header_t));
                        // uint16_t inIcmpCheck = checksum16(&frame.ipv4, sizeof(ipv4_header_t));
                    
                        eth_ipv4_icmp_frame_t responseFrame = {};
                        std::memcpy(&responseFrame, &frame, sizeof(eth_ipv4_icmp_frame_t));
                        // ETH MAC
                        std::memcpy(&responseFrame.eth.dstMac, &frame.eth.srcMac, 6);
                        std::memcpy(&responseFrame.eth.srcMac, myMac.data(), 6); 

                        // IPv4 IP
                        std::memcpy(&responseFrame.ipv4.srcIP, &frame.ipv4.dstIP, 4);
                        std::memcpy(&responseFrame.ipv4.dstIP, &frame.ipv4.srcIP, 4);

                        // IPv4 ID
                        responseFrame.ipv4.flagsFrag = 0x0000;

                        // type response
                        responseFrame.icmp.type = 0x00;

                        // checksums
                        responseFrame.ipv4.checksum = 0x0000;
                        responseFrame.ipv4.checksum = checksum16(&responseFrame.ipv4, sizeof(ipv4_header_t));
                        responseFrame.icmp.checksum = 0x0000;

                        std::array<uint8_t, 2048> outputBuffer;

                        std::memcpy(outputBuffer.data(), &responseFrame, sizeof(eth_ipv4_icmp_frame_t));
                        std::memcpy(outputBuffer.data() + sizeof(eth_ipv4_icmp_frame_t), buffer.data() + sizeof(eth_ipv4_icmp_frame_t), icmpPayloadSize);

                        uint16_t checksumIcmp = checksum16(
                            outputBuffer.data() + (sizeof(ethernet_header_t) + sizeof(ipv4_header_t)), 
                            sizeof(icmp_header_t) + icmpPayloadSize
                        );

                        std::memcpy(outputBuffer.data() + (((uint8_t*)&responseFrame.icmp.checksum) - ((uint8_t*)&responseFrame)), 
                            &checksumIcmp, sizeof(checksumIcmp));

                        if (packetReader->getVerbose()) {
                            log << "Sending: 0x" << sizeof(eth_ipv4_icmp_frame_t) << " header, 0x" << icmpPayloadSize << " payload\n";
                            log << "Total: 0x" << sizeof(eth_ipv4_icmp_frame_t) + icmpPayloadSize << " \n";
                        }
                        if (packetReader->getVerbose()) {
                            appendBytesToStringStream(log, outputBuffer.data(), sizeof(eth_ipv4_icmp_frame_t) + icmpPayloadSize, 8);
                            log << "\n";
                        } 
                        int sentSize = packetReader->writePacket(outputBuffer.data(), sizeof(eth_ipv4_icmp_frame_t) + icmpPayloadSize);
                        log << "Sent " << sentSize << " bytes\n";
                    
                    }  
                } 
            } 

            if(!printed) {
                    log << "Received unrecognized packet: " << packetSize << " bytes\n";
                        if (packetReader->getVerbose()) {
                            appendBytesToStringStream(log, buffer.data(), packetSize, 8);
                        } else {
                            appendBytesToStringStream(log, buffer.data(), ((packetSize > 4) ? 4 : packetSize), 8);
                            log << "...";
                        }
                    log << "...\n";
            }
            std::string result = log.str();
            std::cout << result << "\n";
        }
    }
    
}
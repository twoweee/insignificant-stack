#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <cstring>
#include <algorithm>

typedef struct {
    uint8_t  dstMac[6];
    uint8_t  srcMac[6];
    uint16_t etherType;
} __attribute__((packed)) ethernet_header_t;

typedef struct {
    uint16_t htype;       // hardware type
    uint16_t ptype;       // protocol type
    uint8_t  hlen;        // hardware addr length
    uint8_t  plen;        // protocol addr length
    uint16_t opcode;      // request=1, reply=2

    uint8_t  srcMac[6];
    uint32_t srcIP;

    uint8_t  dstMac[6];
    uint32_t dstIP;
} __attribute__((packed)) arp_header_t;

typedef struct {
    ethernet_header_t eth;
    arp_header_t arp;
} __attribute__((packed)) eth_arp_frame_t;

typedef struct {
    uint8_t  ihl:4;        // header length (in 32-bit words)
    uint8_t  version:4;    // IPv4 = 4

    uint8_t  tos;          // type of service
    uint16_t totalLength;  // full packet size
    uint16_t id;           // identification
    uint16_t flagsFrag;    // flags
    uint8_t  ttl;          
    uint8_t  protocol;     // ICMP=1, TCP=6, UDP=17
    uint16_t checksum;     // header checksum
    uint32_t srcIP;
    uint32_t dstIP;
} __attribute__((packed)) ipv4_header_t;

typedef struct {
    uint8_t  type;        // echo request = 8, echo reply = 0
    uint8_t  code;        
    uint16_t checksum;

    uint16_t identifier;
    uint16_t sequence;
} __attribute__((packed)) icmp_header_t;

typedef struct {
    ethernet_header_t eth;
    ipv4_header_t ipv4;
    icmp_header_t icmp;
} __attribute__((packed)) eth_ipv4_icmp_frame_t;
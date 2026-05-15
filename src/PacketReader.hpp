#pragma once

#include <string>
#include <vector>
#include <array>
#include <stdexcept>
#include <sstream>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/epoll.h>
#include <algorithm>
#include <cctype>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cerrno>
#include <cstdio>

#include "Args.hpp"

class SafeDescriptor {
private:
    int value = -1;
public:
    explicit SafeDescriptor(int val = -1);
    ~SafeDescriptor();
    SafeDescriptor(const SafeDescriptor&) = delete;
    SafeDescriptor& operator=(const SafeDescriptor&) = delete;

    void set(int val = -1);
    int get() const;
    explicit operator bool() const;
};

class PacketReader {
private:
    const std::string tapName;
    const char* chTapName;
    const bool verbose;
    const int epollMax;
    const int epollTimeout;

    SafeDescriptor descriptor;
    SafeDescriptor epollInstance;
    struct ifreq interfReq {};
    epoll_event ev {};

    std::vector<epoll_event> events;
    std::array<uint8_t, 2048> buffer;

    static bool checkValidTapName(const std::string& name);
    static int toInt(const std::string& s, const int def);

public:
    PacketReader(const std::string& tap,
                const std::string& verb,
                const std::string& max,
                const std::string& timeout);

    PacketReader(const Args& args);
    ~PacketReader();

    int init();

    std::string toString() const;

    int epollWait();
    int readPacket(uint8_t* data, std::size_t size);

    int writePacket(const uint8_t* data, std::size_t size);

    int getEpollMax() const;
    bool getVerbose() const;

    std::array<uint8_t, 6> getMyMac() const;

    PacketReader(const PacketReader&) = delete;
    PacketReader& operator=(const PacketReader&) = delete;
};
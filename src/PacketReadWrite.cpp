#include "PacketReader.hpp"

SafeDescriptor::SafeDescriptor(int val) : value(val) {}
SafeDescriptor::~SafeDescriptor() { if (value >= 0) close(value); }
void SafeDescriptor::set(int val) { if (value >= 0) close(value); value = val; }
int SafeDescriptor::get() const { return value; }
SafeDescriptor::operator bool() const { return value >= 0; }

bool PacketReader::checkValidTapName(const std::string& name) {
    if (name.empty() || name.size() >= IFNAMSIZ) return false;
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) &&
            c != '_' && c != '-' && c != '.')
            return false;
    }
    return true;
}

int PacketReader::toInt(const std::string& s, const int def) {
    if (s.empty()) return def;
    if (!std::all_of(s.begin(), s.end(), [](unsigned char c){ return std::isdigit(c); }))
        return def;
    if (s[0] == '0') return def;
    try { return std::stoi(s); } catch (...) { return def; }
}

PacketReader::PacketReader(const std::string& tap,
                           const std::string& verb,
                           const std::string& max,
                           const std::string& timeout)
    :   tapName(checkValidTapName(tap) ? tap : "tap0"),
        chTapName(tapName.c_str()),
        verbose((verb == "true")),
        epollMax(toInt(max, 1)),
        epollTimeout(toInt(timeout, -1)),
        events(epollMax)
{
    if (init()) throw std::runtime_error("initialization failed");
}

PacketReader::PacketReader(const Args& args)
    :   PacketReader(args["--tap_name"],
                   args["--verbose"],
                   args["--epoll_max"],
                   args["--epoll_timeout"])
{}

PacketReader::~PacketReader() {}

int PacketReader::init() {
    descriptor.set(open("/dev/net/tun", O_RDWR));
    if (descriptor.get() < 0) return 1;

    int flags = fcntl(descriptor.get(), F_GETFL, 0);
    if (flags < 0) return 1;
    if (fcntl(descriptor.get(), F_SETFL, flags | O_NONBLOCK) < 0) return 1;

    interfReq.ifr_flags = IFF_TAP | IFF_NO_PI;
    std::snprintf(interfReq.ifr_name, IFNAMSIZ, "%s", chTapName);
    if (ioctl(descriptor.get(), TUNSETIFF, &interfReq) < 0) return 1;

    epollInstance.set(epoll_create1(0));
    if (epollInstance.get() < 0) return 1;

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = descriptor.get();
    if (epoll_ctl(epollInstance.get(), EPOLL_CTL_ADD, descriptor.get(), &ev) < 0) return 1;

    return 0;
}

std::string PacketReader::toString() const {
    std::ostringstream out;
    out << "PacketReader:\n";
    out << "TAP name: " << chTapName << "\n";
    out << "verbose: " << (verbose ? "true" : "false") << "\n";
    out << "epoll max events: " << epollMax << "\n";
    out << "epoll timeout ms: " << epollTimeout << "\n\n";
    return out.str();
}

int PacketReader::epollWait() {
    return epoll_wait(epollInstance.get(), events.data(), epollMax, epollTimeout);
}

int PacketReader::readPacket(uint8_t* data, std::size_t size) {
    return read(descriptor.get(), data, size);
}

// TODO: i dont trust this non blocking write, make sure its safe or fix it
int PacketReader::writePacket(const uint8_t* data, std::size_t size) {
    if (!data || size == 0) return -1;

    ssize_t n = write(descriptor.get(), data, size);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        return -1;
    }
    return static_cast<int>(n);
}

int PacketReader::getEpollMax() const { return epollMax; }
bool PacketReader::getVerbose() const { return verbose; }

std::array<uint8_t, 6> PacketReader::getMyMac() const
{
    std::array<uint8_t, 6> mac = {0};

    SafeDescriptor sock(socket(AF_INET, SOCK_DGRAM, 0));
    if (sock.get() < 0) {
        return mac;
    }

    struct ifreq macIfReq;
    std::memset(&macIfReq, 0, sizeof(macIfReq));

    std::snprintf(macIfReq.ifr_name, IFNAMSIZ, "%s", chTapName);

    if (ioctl(sock.get(), SIOCGIFHWADDR, &macIfReq) == 0) {
        std::memcpy(mac.data(),
                    macIfReq.ifr_hwaddr.sa_data,
                    6);
    }

    return mac;
}
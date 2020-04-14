#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <chrono>

#include <iostream>
using namespace std::chrono;
using std::cerr;
using std::cout;
using std::endl;

// header
typedef struct icmp_header
{
    static uint16_t num;
    uint8_t type;
    uint8_t code;
    uint16_t chksum;
    uint32_t data;
} icmp_header;

// response
typedef struct icmp_response
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence_number;
} icmp_response;

uint16_t icmp_header::num = 0;

int ping(sockaddr_in &addr, int s)
{
    int sent = 0;
    int received = 0;
    while (true)
    {
        icmp_header packet;
        packet.type = 8;
        packet.code = 0;
        // 1's complement
        packet.data = 0 + icmp_header::num; // set sequence number
        packet.chksum = ~(uint16_t(packet.type << 8) + uint16_t(packet.data >> 16) + uint16_t(packet.data & 0xFFFF));

        int result = sendto(s, &packet, sizeof(packet),
                            0, (struct sockaddr *)&addr, sizeof(addr));
        auto start = system_clock::now();
        if (result < 0)
        {
            cerr << "Ping error" << endl;
        }
        unsigned int resAddressSize;
        unsigned char res[30] = "";
        struct sockaddr resAddress;

        int response = recvfrom(s, res, sizeof(res), 0, &resAddress,
                                &resAddressSize);
        auto end = system_clock::now();
        if (response > 0)
        {
            icmp_response *echo;
            echo = (icmp_response *)&res[20]; // throw away header
            if (echo->sequence_number != icmp_header::num)
            {
                cerr << "Sequence numbers do not match" << endl;
            }
            else if (echo->type == 11 && echo->code == 0)
            {
                cerr << "TTL exceeded" << endl;
            }
            else
            {
                ++received;
                auto latency = end - start;
                cout << "Latency: " << latency.count() << endl;
            }
        }
        else
        {
            cerr << "response error" << endl;
        }
        cout << "Sent " << sent << "packets, received " << received
             << "packets" << endl;
        usleep(500000); // sleep for 0.5 seconds
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int ttl = 64;

    if (argc == 2)
    {
    }
    else if (argc == 3)
    {
        ttl = std::atoi(argv[2]);
    }
    else
    {
        cerr << "Usage is ./" << argv[0] << " [hostname/ip] [TTL]. TTL is optional." << endl;
        exit(1);
    }

    int sock = socket(PF_INET, SOCK_RAW, 1);
    setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    struct sockaddr_in addr; // ttl is measured in seconds
    struct hostent *hostEntity;
    if ((hostEntity = gethostbyname(argv[1])) == nullptr)
    {
        cerr << "DNS lookup failed" << endl;
        exit(1);
    }
    addr.sin_family = hostEntity->h_addrtype;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = inet_addr(hostEntity->h_addr);

    ping(addr, sock);
    return 0;
}
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>
#include <string>
#include <string.h>
#include <chrono>

#include <iostream>
using std::cout;
using std::endl;
using std::string;

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
        packet.chksum = 0xfff7;             // 1's complement
        packet.data = 0 + icmp_header::num; // set sequence number

        int result = sendto(s, &packet, sizeof(packet),
                            0, (struct sockaddr *)&addr, sizeof(addr));
        auto start = std::chrono::system_clock::now();
        if (result < 0)
        {
            cout << "ping error" << endl;
        }
        unsigned int resAddressSize;
        unsigned char res[30] = "";
        struct sockaddr resAddress;

        int response = recvfrom(s, res, sizeof(res), 0, &resAddress,
                                &resAddressSize);
        auto end = std::chrono::system_clock::now();
        if (response > 0)
        {
            icmp_response *echo;
            echo = (icmp_response *)&res[20]; // throw away header
            if (echo->sequence_number != icmp_header::num)
            {
                cout << "sequence numbers do not match" << endl;
            }
            else
            {
                ++received;
                auto latency = end - start;
                cout << "latency: " << latency.count() << endl;
            }
        }
        else
        {
            cout << "response error" << endl;
        }
        cout << "Sent " << sent << "packets, received " << received 
             << "packets" << endl;
        
    }
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
        cout << "Usage is ./" << argv[0] << " [hostname/ip] [TTL]. TTL is optional." << endl;
        exit(1);
    }

    int sock = socket(PF_INET, SOCK_RAW, 1);
    setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    struct sockaddr_in addr; // ttl is measured in seconds
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = inet_addr(argv[1]);

    return 0;
}
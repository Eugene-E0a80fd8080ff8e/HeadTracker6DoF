

#include "udpsender.hpp"
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <stdint.h>
#include <chrono>




using namespace std;

int64_t currentTimeMillis()
{
    std::chrono::time_point<std::chrono::steady_clock> t = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count();
}


UDPSender::UDPSender(uint16_t port)
{
    int iResult;
    WSADATA wsaData;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        cout << "WSAStartup failed with error: " << iResult << endl;
        exit(13);
    }

    fd = (__int64)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == -1) {
        cout << "socket failed with error: " << WSAGetLastError() << endl;
        WSACleanup();
        exit(13);
    }
}

/*
void UDPSender::send(float x, float y)
{
    int c = 0;

    *(uint8_t*)(buf + c) = 61; // packet id
    c += sizeof(uint8_t);

    *(int64_t*)(buf + c) = currentTimeMillis(); 
    c += sizeof(int64_t);

    *(float*)(buf + c) = x;
    c += sizeof(float);

    *(float*)(buf + c) = y;
    c += sizeof(float);

    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    //remote.sin_addr.s_addr = 0xc0a80165; //inet_addr("192.168.1.255");
    //remote.sin_addr.s_addr = 0x0b01a8c0;
    remote.sin_addr.s_addr = 0x0100007f; // 127.0.0.1
    remote.sin_port = htons(62731);

    sendto(fd, buf, c, 0, (const sockaddr*)&remote, sizeof(remote));
}
*/
/*
void UDPSender::send_rt(std::vector<double> data)
{
    int c = 0;
    int totalsize = sizeof(uint8_t) + sizeof(int16_t) + sizeof(int64_t) + sizeof(double)  * data.size();

    *(uint8_t*)(buf + c) = 67; // packet id
    c += sizeof(uint8_t);

    *(uint16_t*)(buf + c) = totalsize; 
    c += sizeof(uint16_t);

    *(int64_t*)(buf + c) = currentTimeMillis(); 
    c += sizeof(int64_t);


    for (int i = 0;i < data.size();i++)
    {
        *(double*)(buf + c) = data[i];
        c += sizeof(double);
    }

    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = 0x0100007f; // 127.0.0.1
    remote.sin_port = htons(62731);

    sendto(fd, buf, c, 0, (const sockaddr*)&remote, sizeof(remote));
}*/

/*
void UDPSender::send(int x, int y, std::vector<std::pair<int, int> > data)
{
    int c = 0;

    *(uint8_t*)(buf + c) = 62; // packet id
    c += sizeof(uint8_t);

    *(int64_t*)(buf + c) = currentTimeMillis(); 
    c += sizeof(int64_t);

    *(short*)(buf + c) = x;
    c += sizeof(short);

    *(short*)(buf + c) = y;
    c += sizeof(short);


    *(short*)(buf + c) = (short)data.size();
    c += sizeof(short);

    for (std::pair<int, int> p : data)
    {
        *(short*)(buf + c) = p.first;
        c += sizeof(short);
        *(short*)(buf + c) = p.second;
        c += sizeof(short);
    }

    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = 0x0100007f; // 127.0.0.1
    remote.sin_port = htons(this->port);

    sendto(fd, buf, c, 0, (const sockaddr*)&remote, sizeof(remote));
}
*/

char* write_packet0(char* buf)
{
    buf[0] = 0; // empty packet
    *(short*)(&buf[1]) = 4; // packet length
    buf[3] = 0; // just nothing
    return buf + 4;
}

char* write_packet67(char* buf, vector<double> data)
{
    int c = 0;
    int totalsize = sizeof(uint8_t) + sizeof(int16_t) + sizeof(int64_t) + sizeof(double) * data.size();

    *(uint8_t*)(buf + c) = 67; // packet id
    c += sizeof(uint8_t);

    *(uint16_t*)(buf + c) = totalsize; 
    c += sizeof(uint16_t);

    *(int64_t*)(buf + c) = currentTimeMillis(); 
    c += sizeof(int64_t);

    for (int i = 0;i < data.size();i++)
    {
        *(double*)(buf + c) = data[i];
        c += sizeof(double);
    }

    return buf + c;
}

char* write_packet73(char* buf, array< pair<short, short> ,68> data)
{
    buf[0] = 73;

    int c = sizeof(char) + sizeof(short);

    *(int64_t*)(buf + c) = currentTimeMillis();
    c += sizeof(int64_t);

    for (auto datum : data) {
        *(short*)(&buf[c]) = datum.first;
        c += sizeof(short);
        *(short*)(&buf[c]) = datum.second;
        c += sizeof(short);
    }
    *(short*)(&buf[1]) = c;

    return buf + c;

}


void UDPSender::send_rt_and_68points(vector<double> data, array< pair<short, short> , 68 > data68)
{
    char* p = buf;

    p = write_packet0(p);
    p = write_packet67(p,data);
    p = write_packet73(p, data68); // 68 stands for 68-point face recognition from dlib

    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = 0x0100007f; // 127.0.0.1
    remote.sin_port = htons(62731);

    sendto(fd, buf, p - buf, 0, (const sockaddr*)&remote, sizeof(remote));

}
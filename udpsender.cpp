
#include "stdafx.h"
#include "after_stdafx.h"

//#define _WINSOCKAPI_
//#define WIN32_LEAN_AND_MEAN 
//#include <Windows.h>


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
    this->port = port;

    int iResult;
    WSADATA wsaData;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        cout << "WSAStartup failed with error: " << iResult << endl;
        exit(13);
    }

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

/*

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

char* write_hands_packet(char* buf,uint8_t packetId, vector<float> const& handLocation)
{
    if (handLocation.size() == 6)
    {
        buf[0] = packetId;
        int c = sizeof(char) + sizeof(short);
        *(int64_t*)(buf + c) = currentTimeMillis();
        c += sizeof(int64_t);
        for (auto datum : handLocation) {
            *(float*)(&buf[c]) = datum;
            c += sizeof(float);
        }
        *(short*)(&buf[1]) = c;

        return buf + c;
    }
    else
        return buf;
}

void UDPSender::send_hands_location(vector<float> const& left, vector<float> const& right)
{
    char* p = buf;

    p = write_packet0(p);
    p = write_hands_packet(p, 81, left);
    p = write_hands_packet(p, 82, right);

    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = 0x0100007f; // 127.0.0.1
    remote.sin_port = htons(62731);

    sendto(fd, buf, p - buf, 0, (const sockaddr*)&remote, sizeof(remote));

}


void UDPSender::send_rt_and_68points(vector<double> data, array<pair<short, short>,68> data68)
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

void UDPSender::send_rt(vector<double> data)
{
    char* p = buf;

    p = write_packet0(p);
    p = write_packet67(p, data);

    struct sockaddr_in remote;
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = 0x0100007f; // 127.0.0.1
    remote.sin_port = htons(62731);

    sendto(fd, buf, p - buf, 0, (const sockaddr*)&remote, sizeof(remote));
}
*/


void UDPSender::begin()
{
    bufc = 0;
}

void UDPSender::add_empty() 
{
    push<uint8_t>(0); // empty packet
    push<uint16_t>(4);  // packet length
    push<uint8_t>(0); // just nothing
}

void UDPSender::add_head(cv::Mat rvec, cv::Mat tvec)
{
    int oldc = bufc;
    push<uint8_t>(57);
    uint16_t & size = getRef<uint16_t>();
    push<uint64_t>(currentTimeMillis());
    push<double>(tvec.at<double>(0));
    push<double>(tvec.at<double>(1));
    push<double>(tvec.at<double>(2));
    push<double>(rvec.at<double>(0));
    push<double>(rvec.at<double>(1));
    push<double>(rvec.at<double>(2));
    size = bufc - oldc;
}

void UDPSender::add_left_hand(cv::Mat rvec, cv::Mat tvec)
{
    int oldc = bufc;
    push<uint8_t>(81);
    uint16_t& size = getRef<uint16_t>();
    push<uint64_t>(currentTimeMillis());
    push<float>((float)rvec.at<double>(0));
    push<float>((float)rvec.at<double>(1));
    push<float>((float)rvec.at<double>(2));
    push<float>((float)tvec.at<double>(0));
    push<float>((float)tvec.at<double>(1));
    push<float>((float)tvec.at<double>(2));
    size = bufc - oldc;
}

void UDPSender::add_right_hand(cv::Mat rvec, cv::Mat tvec)
{
    int oldc = bufc;
    push<uint8_t>(82);
    uint16_t& size = getRef<uint16_t>();
    push<uint64_t>(currentTimeMillis());
    push<float>((float)rvec.at<double>(0));
    push<float>((float)rvec.at<double>(1));
    push<float>((float)rvec.at<double>(2));
    push<float>((float)tvec.at<double>(0));
    push<float>((float)tvec.at<double>(1));
    push<float>((float)tvec.at<double>(2));
    size = bufc - oldc;
}

void UDPSender::add_68points(std::array< std::pair<short, short>, 68> const & data68)
{
    int oldc = bufc;
    push<uint8_t>(73);
    uint16_t& size = getRef<uint16_t>();
    push<uint64_t>(currentTimeMillis());
    for (auto datum : data68) {
        push<short>(datum.first);
        push<short>(datum.second);
    }
    size = bufc - oldc;
}

/*
void UDPSender::add_packet66(vector<double> data)
{
    int oldc = bufc;
    push<uint8_t>(66);
    uint16_t& size = getRef<uint16_t>();
    push<uint64_t>(currentTimeMillis());
    for (int i = 0;i < data.size();i++)
        push<double>(data[i] );
    size = bufc - oldc;
}
*/

void UDPSender::send() 
{
    if(bufc>0)
    {
        struct sockaddr_in remote;
        memset(&remote, 0, sizeof(remote));
        remote.sin_family = AF_INET;
        remote.sin_addr.s_addr = 0x0100007f; // 127.0.0.1
        remote.sin_port = htons(this->port);

        sendto(fd, buf, bufc, 0, (const sockaddr*)&remote, sizeof(remote));
    }
}

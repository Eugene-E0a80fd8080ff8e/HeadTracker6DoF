#pragma once


#include <vector>
#include <array>
#include <utility>

class UDPSender {

private:
    __int64 fd; // It is SOCKET really. I don't want the definitions from Windows.h leak outside of udpsender
    char buf[1000];
    uint16_t port;

    UDPSender(const UDPSender&) = delete;
    UDPSender& operator=(UDPSender const&) = delete;

public:
    UDPSender(uint16_t port);

    //void send(float x, float y);
    //void send(int x, int y, std::vector<std::pair<int, int> > data);
    //void send_rt(std::vector<double> data);
    void send_rt_and_68points(std::vector<double> data,std::array< std::pair<short,short> , 68> data68);

};

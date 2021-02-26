#pragma once



class UDPSender {

private:
    SOCKET fd;
    char buf[2000];
    int bufc;
    uint16_t port;

    UDPSender(const UDPSender&) = delete;
    UDPSender& operator=(UDPSender const&) = delete;

    template <class T> void push(T x) 
    {
        *(T*)(&buf[bufc]) = x; 
        bufc += sizeof(T);
    }

    template <class T> T & getRef()
    {
        T* res = (T*)(&buf[bufc]);
        bufc += sizeof(T);
        return *res;
    }


public:
    UDPSender(uint16_t port);

    //void send(float x, float y);
    //void send(int x, int y, std::vector<std::pair<int, int> > data);
    //void send_rt(std::vector<double> data);
    //void send_rt_and_68points(std::vector<double> data,std::array< std::pair<short,short> , 68> data68);
    //void send_hands_location(std::vector<float> const& left, std::vector<float> const& right);


    void begin();
    void add_empty();
    void add_head(cv::Mat rvec, cv::Mat tvec);
    void add_left_hand(cv::Mat rvec, cv::Mat tvec);
    void add_right_hand(cv::Mat rvec, cv::Mat tvec);
    void add_68points(std::array< std::pair<short, short>, 68> const & data68);
    void add_packet66(vector<double> data);
    void send();

};

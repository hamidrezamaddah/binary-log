#include <fstream>
#include <span>
#include <iostream>

std::ostream &write_frame(std::ostream &ofs, std::span<const char> buffer, unsigned char id, int sequence)
{
    char header[2]{};
    ofs.write(header, 2);

    unsigned char checksum{id};
    ofs.write(reinterpret_cast<char *>(&id), 1);

    ofs.write(reinterpret_cast<char *>(&sequence), 2);

    checksum += sequence & 0xFF;
    checksum += (sequence >> 8) & 0xFF;

    char content_size = buffer.size();
    ofs.write(&content_size, 1);
    checksum += content_size;

    for (auto &&c : buffer)
    {
        checksum += c;
        ofs.write(&c, 1);
    }

    ofs.write(reinterpret_cast<char *>(&checksum), 1);

    return ofs;
}

struct __attribute__((__packed__)) oxygen_level
{
    static const unsigned char ID = 89;

    unsigned int timestamp;
    double level;
    int other_data;
};

struct __attribute__((__packed__)) system_state
{
    static const unsigned char ID = 88;
    char arg1;
    int state2;
    unsigned int timestamp;
};

struct __attribute__((__packed__)) sensor_data
{
    static const unsigned char ID = 80;
    double data1;
    float data2;
    int data3;
    unsigned int time;
};

struct __attribute__((__packed__)) diag_pkt
{
    static const unsigned char ID = 81;
    unsigned short diag_value;
    unsigned int time;
};



template <typename PKT_T>
std::ostream &send_frame(std::ostream &ofs, const PKT_T &pkt, int sequence)
{
    auto ptr = reinterpret_cast<const char *>(&pkt);
    write_frame(ofs, {ptr, sizeof(pkt)}, PKT_T::ID, sequence);
    return ofs;
}

int main()
{
    std::ofstream out_file{"pkts.bin", std::ios::binary};

    int seq{900};
    unsigned int time = 23456;

    for (int i = 0; i < 5; i++)
    {
        sensor_data sd{45.23, 8.9f, 0x90, time};
        send_frame(out_file, sd, seq++);

        time += 10;
        send_frame(out_file, system_state{12, 14, time}, seq++);

        time += 20;
        send_frame(out_file, oxygen_level{time, 0.89, 59}, seq++);
        send_frame(out_file, system_state{23, 1223, time}, seq++);

        time += 5;
        send_frame(out_file, diag_pkt{67, time}, seq++);
    }

    out_file.close();

    return 0;
}
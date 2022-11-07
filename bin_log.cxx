#include <fstream>
#include <iostream>
#include <vector>
#include <iterator>
#include <cstring>
#include <sstream>
#include <typeinfo>
#include <variant>
#include <any>
#include <map>

// oxygen_level 89 timestamp-U32 level-D other_data-I32
// system_state 88 arg1-I8 state2-I32 timestamp-U32
// sensor_data 80 data1-D data2-f data3-I32 timestamp-U32

std::vector<std::string> string_split(const std::string &str, char delemiter)
{
    std::vector<std::string> strings;
    unsigned int start = 0;
    while (true)
    {
        auto idx = str.find_first_of(delemiter, start);
        idx = std::min(idx, str.length());
        auto len = idx - start;
        std::string sub = str.substr(start, len);
        strings.push_back(sub);
        if (idx == str.length())
        {
            break;
        }
        start = idx + 1;
    }
    return strings;
}

enum param_types_t
{
    none,
    U8,
    I8,
    U16,
    I16,
    U32,
    I32,
    D,
    F
};

std::map<std::string, param_types_t> param_types{
    {"U8", param_types_t::U8},
    {"I8", param_types_t::I8},
    {"U16", param_types_t::U16},
    {"I16", param_types_t::I16},
    {"U32", param_types_t::U32},
    {"I32", param_types_t::I32},
    {"F", param_types_t::F},
    {"D", param_types_t::D}};

struct param_t
{
    std::string name;
    param_types_t type;
};

struct pkt_descriptor_t
{
    std::string name;
    unsigned int id;
    std::vector<param_t> params;
};

std::vector<pkt_descriptor_t> parse_descriptor()
{
    std::vector<pkt_descriptor_t> descriptors;
    std::ifstream file("pktinfo.txt");
    std::string line;
    while (std::getline(file, line))
    {
        pkt_descriptor_t descriptor;
        std::istringstream iss(line);
        iss >> descriptor.name;
        iss >> descriptor.id;

        while (true)
        {
            std::string param_str;
            iss >> param_str;
            std::vector<std::string> split_res = string_split(param_str, '-');
            if (split_res.size() != 2)
            {
                break;
            }
            param_t param{split_res[0], param_types[split_res[1]]};
            descriptor.params.push_back(param);
        }
        descriptors.push_back(descriptor);
    }
    return descriptors;
}

struct pkt_t
{
    uint8_t id;
    uint8_t cs;
    uint16_t seq;
    std::vector<uint8_t> content;
};

// void print_pkts(const std::vector<pkt_t> &pkts, std::vector<pkt_descriptor_t> descriptors)
// {
//     unsigned int num = 1;
//     bool first = true;
//     unsigned int init_sec;
//     unsigned int num_lost = 0;
//     for (auto &pkt : pkts)
//     {
//         std::cout << "PKT" << std::dec << num++ << " ID=" << std::hex << (int)pkt.id << " Seq=0x" << pkt.seq << " Content=" << pkt.content << "\n";
//         if (first)
//         {
//             first = false;
//         }
//         else
//         {
//             if (pkt.seq != init_sec + 1)
//             {
//                 num_lost += (pkt.seq - init_sec - 1);
//             }
//         }
//         init_sec = pkt.seq;
//     }
//     std::cout << "PktLoss=" << std::dec << num_lost;
// }

long long int get_file_size(std::ifstream &fin)
{
    fin.seekg(0, std::ios::beg); // move to the first byte
    std::istream::pos_type start_pos = fin.tellg();
    fin.seekg(0, std::ios::end); // move to the last byte
    std::istream::pos_type end_pos = fin.tellg();
    return end_pos - start_pos; // position difference
}

enum class state_t
{
    h1,
    h2,
    id,
};

std::ostream &operator<<(std::ostream &os, const std::vector<uint8_t> &items)
{
    for (auto item : items)
    {
        os << (int)item;
    }
    return os;
}

void print_pkts(const std::vector<pkt_t> &pkts)
{
    unsigned int num = 1;
    bool first = true;
    unsigned int init_sec;
    unsigned int num_lost = 0;
    for (auto &pkt : pkts)
    {
        std::cout << "PKT" << std::dec << num++ << " ID=" << std::hex << (int)pkt.id << " Seq=0x" << pkt.seq << " Content=" << pkt.content << "\n";
        if (first)
        {
            first = false;
        }
        else
        {
            if (pkt.seq != init_sec + 1)
            {
                num_lost += (pkt.seq - init_sec - 1);
            }
        }
        init_sec = pkt.seq;
    }
    std::cout << "PktLoss=" << std::dec << num_lost;
}

uint8_t check_sum(uint8_t *buff, unsigned int len)
{
    unsigned int sum{0};
    for (unsigned int i = 0; i < len; i++)
    {
        sum += buff[i];
    }
    return sum;
}

int main(int argc, char **argv)
{
    std::ifstream fin("pkts.bin", std::ios::binary);
    fin.unsetf(std::ios::skipws);
    unsigned int file_size = get_file_size(fin);
    std::cout << "file size: " << file_size << "\n";
    fin.seekg(0, std::ios::beg);
    std::vector<uint8_t> buff;
    buff.reserve(file_size);

    std::copy(std::istream_iterator<uint8_t>(fin),
              std::istream_iterator<uint8_t>(),
              std::back_inserter(buff));

    std::cout << "buff size: " << buff.size() << "\n";

    int ptr = 0;
    unsigned int pkt_corrupt{0};
    state_t state = state_t::h1;

    std::vector<pkt_t> pkts;
    pkt_t pkt;
    while (ptr < buff.size())
    {

        unsigned int size;
        switch (state)
        {
        case state_t::h1:
            if (buff[ptr++] == 0)
            {
                state = state_t::h2;
            }
            break;

        case state_t::h2:
            if (buff[ptr++] == 0)
            {
                state = state_t::id;
            }
            break;

        case state_t::id:
            pkt.id = buff[ptr++];
            pkt.seq = *reinterpret_cast<uint16_t *>(&buff[ptr]);
            ptr += 2;
            size = buff[ptr++];
            pkt.content.resize(size);
            pkt.content.assign(buff.data() + ptr, buff.data() + ptr + size);
            ptr += size;
            if (buff[ptr] == check_sum(&buff[ptr - (4 + size)], 4 + size))
            {
                pkts.push_back(pkt);
                ptr++;
            }
            else
            {
                pkt_corrupt++;
                ptr -= (size + 5);
            }
            state = state_t::h1;

            break;
        }
    }
    print_pkts(pkts);
    std::cout << " PktCorrupt=" << pkt_corrupt << "\n";
}

#include <fstream>
#include <iostream>
#include <vector>
#include <iterator>

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

struct pkt_t
{
    uint8_t id;
    uint8_t cs;
    uint16_t seq;
    std::vector<uint8_t> content;
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
    std::ifstream fin("sample.bin", std::ios::binary); 
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

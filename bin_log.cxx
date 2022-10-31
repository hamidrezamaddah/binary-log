#include <fstream>
#include <iostream>
#include <vector>


long long int get_file_size(std::ifstream& fin)
{

    fin.seekg(0, std::ios::beg);       // move to the first byte
    std::istream::pos_type start_pos = fin.tellg();
    // get the start offset
    fin.seekg(0, std::ios::end); // move to the last byte
    std::istream::pos_type end_pos = fin.tellg();
    // get the end offset
    return end_pos - start_pos; // position difference
}

enum class state_t
{
    h1, h2, id, sec, size, content, checksum
};

struct pkt_t
{
    uint8_t id;
    uint16_t seq;
    std::vector<uint8_t> content;
    uint8_t cs;
};

uint8_t check_sum(uint8_t* buff, unsigned int len)
{
    unsigned int sum{ 0 };
    for (unsigned int i = 0; i < len; i++)
    {
        sum += buff[i];
    }
    return sum;
}

int main(int argc, char** argv)
{
    std::ifstream fin("sample.bin"); // open the file
    unsigned int file_size = get_file_size(fin);
    std::cout << "file size: " << file_size;
    uint8_t* buff = new uint8_t[file_size];
    fin.read((char*)buff, file_size);

    int ptr = 0;
    unsigned int pkt_corrupt{ 0 };
    state_t state = state_t::h1;

    std::vector<pkt_t> pkts;

    while (true)
    {
        pkt_t pkt;
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
            state = state_t::sec;
            break;

        case state_t::sec:
            pkt.seq = *((uint16_t*)(&buff[ptr]));// static_cast<uint16_t*>()
            ptr += 2;
            state = state_t::size;
            break;

        case state_t::size:
            size = buff[ptr++];
            state = state_t::content;
            break;

        case state_t::content:
            for (int i = 0; i < size; i++)
            {
                pkt.content.push_back(buff[ptr++]);
            }
            state = state_t::checksum;
            break;

        case state_t::checksum:
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

    delete(buff);

}

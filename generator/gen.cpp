#include <fstream>
#include <span>


std::ostream& write_frame(std::ostream& ofs, std::span<const char> buffer, unsigned char id, int sequence)
{
    char header[2] {};
    ofs.write(header, 2);

    unsigned char checksum {id};
    ofs.write(reinterpret_cast<char*>(&id), 1);
    
    ofs.write(reinterpret_cast<char*>(&sequence), 2);

    checksum += sequence & 0xFF;
    checksum += (sequence>>8) & 0xFF;

    char content_size = buffer.size();
    ofs.write(&content_size, 1);
    checksum += content_size;

    for (auto &&c : buffer)
    {
        checksum += c;
        ofs.write(&c,1); 
    }

    ofs.write(reinterpret_cast<char*>(&checksum), 1);

    return ofs;
}

int main()
{
    std::ofstream out_file {"sample.bin", std::ios::binary};

    int seq = 280;

    write_frame(out_file, "Packet1", 0x89, seq++);
    write_frame(out_file, "Data2", 0x90, seq++);

    seq++; // Lost frame!
    write_frame(out_file, "BeforeLoss", 0x45, seq++);

    write_frame(out_file, "OtherData3", 0x91, seq++);


    // Corrupt checksum
    write_frame(out_file, "Sample1", 9, seq++);
    out_file.seekp(-1, std::ios_base::cur);
    char invalid_checksum = 0xAA;
    out_file.write(&invalid_checksum, 1);

    write_frame(out_file, "Sample2", 9, seq++);

    // Insert some dummy data
    char dummy_data[]{ 2, 23, 89, 45, 90};
    out_file.write(dummy_data, sizeof(dummy_data));

    write_frame(out_file, "Some_value", 0x91, seq++);

    char buffer[2] {0x10, 0x30};
    write_frame(out_file, buffer, 0x80, seq++);


    out_file.close();

    return 0;
}
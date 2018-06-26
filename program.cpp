#include <iostream>
#include <bitset>
#include <iomanip>
#include <fstream>
#include <list>
#include <vector>
#include <utility>
#include <ctime>

std::string filename;
std::fstream file;
std::list<std::bitset<8>> record;
std::list<std::pair<std::string, std::time_t>> result;

unsigned short int binary_value(std::bitset<8> & bs);
unsigned int loaddigit(unsigned int bs, unsigned int count_byte, unsigned int skip, unsigned int offset);
std::list<std::bitset<8>> & loaddata(unsigned int bs, unsigned int count, unsigned int skip, unsigned int offset);
unsigned int getroot(unsigned int parition);
std::tm calctm(unsigned int date, unsigned int time);
std::vector<std::pair<std::string, std::time_t>> getnmtm(unsigned int root);
void show();

int main() {
    using namespace std;
    bitset<8> bs;

    unsigned int mbr, vbr;
    unsigned parition1, parition2, parition3, parition4;

    cout << hex;
    cin >> filename;
    file.open(filename, ios::in | ios::binary);
    if (!file) {
        printf("open failed");
        return 0;
    }

    // to MBR
    parition1 = loaddigit(512, 4, 0, 454);
    parition2 = loaddigit(512, 4, 0, 454 + 32);
    parition3 = loaddigit(512, 4, 0, 454 + 64);
    parition4 = loaddigit(512, 4, 0, 454 + 128);

    // to VBR

    unsigned int rootdirect1 = getroot(parition1);

    cout << loaddigit(512, 1, rootdirect1, 0xb) << endl;
    cout << loaddigit(512, 1, rootdirect1, 0xb + 32) << endl;


    return 0;


}

inline unsigned short int binary_value(std::bitset<8> & bs) {
	return 
		bs[0] * 1 + bs[1] * 2 + bs[2] * 4 + bs[3] * 8 +
		bs[4] * 16 + bs[5] * 32 + bs[6] * 64 + bs[7] * 128;
}

std::list<std::bitset<8>> & loaddata(unsigned int bs, unsigned int count_byte, unsigned int skip, unsigned int offset) {
    std::bitset<8> data;

    record.clear();
    file.seekg(skip * bs + offset, std::ios::beg);

    for (int i = 0; i < count_byte && !file.eof(); ++i) {
        file.read((char*)&data, 1);
        record.insert(record.end(), data);
    }
    return record;
}

unsigned int loaddigit(unsigned int bs, unsigned int count_byte, unsigned int skip, unsigned int offset) {
    unsigned int sectors = 0;
    loaddata(bs, count_byte, skip, offset);
    unsigned int j = 0;
    for (auto i = record.begin(); i != record.end(); ++i, ++j)
        sectors += binary_value(*i) << (8 * j);
    return sectors;
}

unsigned int getroot(unsigned int parition) {
    unsigned int reserve1 = loaddigit(512, 2, parition, 14);
    unsigned int num_of_fats = loaddigit(512, 1, parition, 16);
    unsigned int size_of_fat = loaddigit(512, 2, parition, 0x24);
    unsigned int root_begin = loaddigit(512, 4, parition, 0x2c);
    unsigned int sectors_per_cluster = loaddigit(512, 1, parition, 0xd);
    unsigned int rootdirect = parition + reserve1 + num_of_fats * size_of_fat + (root_begin - 2) * sectors_per_cluster;
    return rootdirect;
}

std::tm calctm(unsigned int date, unsigned int time) {
    std::tm filetime;
    // calulate date
    filetime.tm_mday = date % 32;
    filetime.tm_mon = ((date % 512) - filetime.tm_mday) / 32;
    filetime.tm_year = (date - filetime.tm_mday - filetime.tm_mon * 32) / 512;
    // calculate time;
    filetime.tm_sec = (time % 32) * 2;
    filetime.tm_min = ((time % 2048) - filetime.tm_sec / 2) / 32;
    filetime.tm_hour = (time - 32 * filetime.tm_min -filetime.tm_sec / 2) / 2048;
    return filetime;
}

std::vector<std::pair<std::string, std::time_t>> getnmtm(unsigned int directory) {
    unsigned int fileflag = loaddigit(512, 1, directory, 0xb);                                       // 长文件名标识位置0xb
    unsigned int filesize;
    unsigned int filedate, filetime;
    for (int i = 0; fileflag != 0; ++i) {
        if (fileflag != 0xb && fileflag != 0)    // it's a short file name. Use it!             // 长文件名标识位置0xb
            if ((filesize = loaddigit(512, 4, directory, 0x1f + 32 * i)) == 0) {   // if is a directory  // 文件大小0x1c-0x1f 数值为0为目录
                // if not '.' or '..'
                    // read and calculate date and time
                    // read back the full filename
                    // recursion
                // else continue
            }
            else {   // if is a file
                filedate = loaddigit(512, 2, directory, 0xe + 32 * i);   // 创建时间0xe-0xf      修改时间0x16-0x17
                filetime = loaddigit(512, 2, directory, 0x10 + 32 * i);  // 创建日期0x10-0x11    修改日期0x18-0x19
                calctm(filedate, filetime);       // calculate the date and time
                // read back the full filename
            }
            
        fileflag = loaddigit(512, 1, directory, 0xb + 32 * i);
    }
}

void show() {
    unsigned int j = 0;
    for (auto i = record.begin(); i != record.end(); ++i, ++j) {
        if (j % 2 == 0 && i != record.begin())
            std::cout << ' ';
        if (j % 16 == 0 && i != record.begin())
            std::cout << std::endl;
        std::cout << std::setw(2) << std::setfill('0') << std::hex << binary_value(*i);
    }
    std::cout << std::endl;
}
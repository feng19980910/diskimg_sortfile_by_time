#include <iostream>
#include <bitset>
#include <iomanip>
#include <fstream>
#include <list>
#include <vector>
#include <utility>
#include <ctime>

struct Fileinfo {
	std::string filename;
	std::tm time;
};

std::string filename;
std::fstream file;
std::list<std::bitset<8>> record;
std::list<Fileinfo> result;

unsigned short int binary_value(std::bitset<8> & bs);
unsigned int loaddigit(unsigned int bs, unsigned int count_byte, unsigned int skip, unsigned int offset);
std::list<std::bitset<8>> & loaddata(unsigned int bs, unsigned int count, unsigned int skip, unsigned int offset);
unsigned int getroot(unsigned int parition);
std::tm calctm(unsigned int date, unsigned int time);
std::vector<std::pair<std::string, std::tm>> getnmtm(unsigned int directory);
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
	cout << loaddigit(512, 1, rootdirect1, 0xb + 32 * 1) << endl;
	cout << loaddigit(512, 1, rootdirect1, 0xb + 32 * 2) << endl;
	cout << loaddigit(512, 1, rootdirect1, 0xb + 32 * 3) << endl;
	cout << loaddigit(512, 1, rootdirect1, 0xb + 32 * 4) << endl;
	cout << loaddigit(512, 1, rootdirect1, 0xb + 32 * 5) << endl;

	cout << endl << endl;

	for (auto i = result.begin(); i != result.end(); ++i)
		cout << i->first << ' ' << i->second << endl;


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

// TESTED, PASS!!
std::tm calctm(unsigned int date, unsigned int time) {
	std::tm filetime;
	// calulate date
	filetime.tm_mday = date % 32;
	filetime.tm_mon = ((date % 512) - filetime.tm_mday) / 32;
	filetime.tm_year = (date - filetime.tm_mday - filetime.tm_mon * 32) / 512;
	// calculate time;
	filetime.tm_sec = (time % 32) * 2;
	filetime.tm_min = ((time % 2048) - filetime.tm_sec / 2) / 32;
	filetime.tm_hour = (time - 32 * filetime.tm_min - filetime.tm_sec / 2) / 2048;
	return filetime;
}

// to get the long filename
std::string getfilename(unsigned int bs, unsigned int directory, unsigned int setoff) {
	unsigned int fileflag = loaddigit(bs, 1, directory, setoff - 32 + 0xb);
	unsigned int filenamestart = loaddigit(bs, 1, directory, setoff);
	unsigned int tempchar;
	std::string filefullname;
	int i, j;

	if (fileflag = 0xf) {
		// long filename
		for (i = 1; loaddigit(bs, 1, directory, setoff - 32 * i) < 0x41; ++i) {
			// loaddigit(bs, 1, skip, setoff - 32 * i) > 0x40 // the long filename end flag
			filefullname += loaddigit(bs, 1, directory, setoff - 32 * i + 2 * j) / 0x100;
			for (j = 1; j <= 15; ++j) { // 0-4, 7-12, 14-15 is the long file name
				tempchar = loaddigit(bs, 2, directory, setoff - 32 * i + 2 * j);
				if (tempchar = 0)
					break;
				if (((i > 4) && (i < 7)) || ((i > 12) && (i < 14)))
					continue;
				filefullname += tempchar / 0x100;
			}
		}

		filefullname += loaddigit(bs, 1, directory, setoff - 32 * i + 2 * j) / 0x100;
		for (j = 1; j <= 15; ++j) { // 0-4, 7-12, 14-15 is the long file name
			tempchar = loaddigit(bs, 2, directory, setoff - 32 * i + 2 * j);
			if (tempchar = 0)
				break;
			if (((i > 4) && (i < 7)) || ((i > 12) && (i < 14)))
				continue;
			filefullname += tempchar / 0x100;
		}
	}
	else {
		// short filename
		// or NO SHORT FILENAME!!



	}

	// judge the start of 32 bytes to determine if the long filename end
	// judge the 0xb to determin if this is short filename
	// or add an hash function to make sure the short filename come from the long one??
	return filefullname;
}

std::vector<std::pair<std::string, std::tm>> getnmtm(unsigned int directory) {
	unsigned int fileflag = loaddigit(512, 1, directory, 0xb);                                       // 长文件名标识位置0xb
	unsigned int filesize, filedate, filetime;
	std::tm fulltime;
	std::string filefullname;
	std::pair<std::string, std::tm> temp;
	for (int i = 0; loaddigit(512, 1, directory, 0 + 32 * i) != 0; ++i) {          // there still directory exist
		if (fileflag != 0xb && fileflag != 0)    // it's a short file name. Use it!             // 长文件名标识位置0xb
			if ((filesize = loaddigit(512, 4, directory, 0x1f + 32 * i)) == 0) {   // if is a directory  // 文件大小0x1c-0x1f 数值为0为目录
				if (!loaddigit(512, 1, directory, 0x0 + 32 * i) == 0x2e) {
					// if not '.' or '..' or hidden file
					// recursion
					filetime = loaddigit(512, 2, directory, 0xe + 32 * i);  // 创建时间0xe-0xf      修改时间0x16-0x17
					filedate = loaddigit(512, 2, directory, 0x10 + 32 * i); // 创建日期0x10-0x11    修改日期0x18-0x19
					fulltime = calctm(filedate, filetime);                  // calculate the date and time
					filefullname = getfilename(512, directory, 32 * i);
					temp.first = filefullname, temp.second = fulltime;
					result.emplace_back(temp);
				}
				// else continue
			}
			else {   // if is a file
				filedate = loaddigit(512, 2, directory, 0xe + 32 * i);   // 创建时间0xe-0xf      修改时间0x16-0x17
				filetime = loaddigit(512, 2, directory, 0x10 + 32 * i);  // 创建日期0x10-0x11    修改日期0x18-0x19
				fulltime = calctm(filedate, filetime);       // calculate the date and time
				filefullname = getfilename(512, directory, 32 * i);
				result.emplace_back(std::pair<std::string, std::tm>("", fulltime));

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
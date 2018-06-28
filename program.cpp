// TAKE CARE!
// maybe some bug when reading pure short filename (no long filename, only short filename)
// maybe some messy code after print the result, but tired of debugging it
// file and folder should not be to much beyond one cluster 
// it's better to use a disk image without any deleted file

#include <iostream>
#include <bitset>
#include <iomanip>
#include <fstream>
#include <list>
#include <vector>
#include <utility>
#include <ctime>
#include <algorithm>
#include <cstdlib>

// the information of file in a structure
struct Fileinfo {
	std::string filename;
	std::tm time;
	Fileinfo(const std::string & fn, const std::tm & tm) : filename(fn), time(tm) {}
	bool operator<(const Fileinfo & f);
};

bool Fileinfo::operator<(const Fileinfo & f) {
	unsigned da, db, ta, tb;	// date a, date b, time a, time b
	da = time.tm_mday + time.tm_mon * 32 + time.tm_year * 32 * 367;
	db = f.time.tm_mday + f.time.tm_mon * 32 + f.time.tm_year * 32 * 367;
	ta = time.tm_sec + time.tm_min * 61 + time.tm_hour * 61 * 61;
	tb = f.time.tm_sec + f.time.tm_min * 61 + f.time.tm_hour * 61 * 61;

	return da < db || (da == db && ta < tb);
}

// the necessary variable used
std::string filename;				// the name of disk image file
std::fstream file;					// object of file stream to read disk image file
std::list<std::bitset<8>> record;	// global variable used by function binary_value & loaddata & loaddigit
std::vector<Fileinfo> result;		// the filename and create time we get at last

// the functions used
unsigned short int binary_value(std::bitset<8> & bs);				// translate from binary to unsigned integer
std::list<std::bitset<8>> & loaddata(unsigned int bs, unsigned int count, unsigned int skip, unsigned int offset);	// load binary number from file
unsigned int loaddigit(unsigned int bs, unsigned int count_byte, unsigned int skip, unsigned int offset);			// load binary number from file and translate it to integer
unsigned int getroot(unsigned int parition);			// get where the root directory is
std::tm calctm(unsigned int date, unsigned int time);	// calculate time
std::string getfilename(unsigned int bs, unsigned int directory, unsigned int setoff);	// get file name, if short name, maybe some wrong
std::vector<Fileinfo> & getnmtm(unsigned int bs, unsigned int directory, unsigned int root, unsigned int cluster);	//get name and time from root directory and other direcotry by recursion
std::ostream & operator<<(std::ostream & os, const std::tm & t);	// to show create time neatly
void show();		// to show the binary value in record now	// had been used while debugging, but useless in normal

int main() {
	using namespace std;

	bitset<8> bs;
	unsigned parition1, parition2, parition3, parition4;

	// get the disk image file and open it
	cout << "Please inpress the name of disk image file : ";
	cin >> filename;
	file.open(filename, ios::in | ios::binary);
	if (!file) {
		printf("open failed");
		return 0;
	}

	// in MBR		get where the paritions is
	parition1 = loaddigit(512, 4, 0, 454);
	parition2 = loaddigit(512, 4, 0, 454 + 32);
	parition3 = loaddigit(512, 4, 0, 454 + 64);
	parition4 = loaddigit(512, 4, 0, 454 + 128);

	// in VBR		get where the root directory is
	unsigned int rootdirect1 = getroot(parition1);
	unsigned int rootdirect2 = getroot(parition2);
	unsigned int rootdirect3 = getroot(parition3);
	unsigned int rootdirect4 = getroot(parition4);

	unsigned int cluster1, bs1;
	unsigned int cluster2, bs2;
	unsigned int cluster3, bs3;
	unsigned int cluster4, bs4;

	cluster1 = cluster2 = cluster3 = cluster4 = 0;
	bs1 = bs2 = bs3 = bs4 = 0;

	if (rootdirect1)
		cluster1 = loaddigit(512, 1, rootdirect1, 0xd), bs1 = loaddigit(512, 2, parition1, 0xb), getnmtm(bs1, rootdirect1, rootdirect1, cluster1);
	if (rootdirect2)
		cluster2 = loaddigit(512, 1, rootdirect2, 0xd), bs2 = loaddigit(512, 2, parition2, 0xb), getnmtm(bs2, rootdirect2, rootdirect2, cluster2);
	if (rootdirect3)
		cluster3 = loaddigit(512, 1, rootdirect3, 0xd), bs3 = loaddigit(512, 2, parition3, 0xb), getnmtm(bs3, rootdirect3, rootdirect3, cluster3);
	if (rootdirect4)
		cluster4 = loaddigit(512, 1, rootdirect4, 0xd), bs4 = loaddigit(512, 2, parition4, 0xb), getnmtm(bs4, rootdirect4, rootdirect4, cluster4);

	// as you see, just sort by time
	std::sort(result.begin(), result.end());

	// show in a neat way
	cout << dec << endl;
	for (auto i = result.begin(); i != result.end(); ++i) {
		cout << setw(50) << setfill(' ') << i->filename << "\t\t\t";
		cout << setw(40) << i->time << endl;
	}

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
	filetime.tm_year = (date - filetime.tm_mday - filetime.tm_mon * 32) / 512 + 1980;
	// calculate time;
	filetime.tm_sec = (time % 32) * 2;
	filetime.tm_min = ((time % 2048) - filetime.tm_sec / 2) / 32;
	filetime.tm_hour = (time - 32 * filetime.tm_min - filetime.tm_sec / 2) / 2048;
	return filetime;
}

// to get the long filename
std::string getfilename(unsigned int bs, unsigned int directory, unsigned int setoff) {
	unsigned int fileflag = loaddigit(bs, 1, directory, setoff - 32 + 0xb);	// the last line, that is, the long file name exist
	unsigned int filenamestart = loaddigit(bs, 1, directory, setoff);
	unsigned int tempchar;
	std::string filefullname;
	int i, j;

	if (fileflag = 0xf) {
		// long filename
		for (i = 1; loaddigit(bs, 1, directory, setoff - 32 * i) < 0x41; ++i) {
			// loaddigit(bs, 1, skip, setoff - 32 * i) > 0x40 // the long filename end flag
			filefullname += loaddigit(bs, 2, directory, setoff - 32 * i + 2 * 0) / 0x100;
			for (j = 1; j <= 15; ++j) { // 0-4, 7-12, 14-15 is the long file name
				tempchar = loaddigit(bs, 2, directory, setoff - 32 * i + 2 * j);
				if (tempchar == 0 && j != 13)
					break;
				if (((j > 4) && (j < 7)) || ((j > 12) && (j < 14)))
					continue;
				filefullname += (j < 7) ? (tempchar / 0x100) : (tempchar);
			}
		}
		filefullname += loaddigit(bs, 2, directory, setoff - 32 * i + 2 * 0) / 0x100;
		for (j = 1; j <= 15; ++j) { // 0-4, 7-12, 14-15 is the long file name
			tempchar = loaddigit(bs, 2, directory, setoff - 32 * i + 2 * j);
			if (tempchar == 0 && j != 13)
				break;
			if (((j > 4) && (j < 7)) || ((j > 12) && (j < 14)))
				continue;
			filefullname += (j < 7) ? (tempchar / 0x100) : (tempchar);
		}
	}
	else {
		// short filename
		// sometimes think it couldn't be used forever
		for (j = 0; j <= 0x7; ++j)
			if ((tempchar = loaddigit(bs, 1, directory, setoff + j)) != 0x20)
				filefullname += tempchar;
		// remove the spaces at the end of the filename
		for (auto i = filefullname.end(); i != filefullname.begin();)
			if (*(--i) == ' ')
				filefullname.erase(i);
			else
				break;
		for (; j <= 0xa; ++j)
			if ((tempchar = loaddigit(bs, 1, directory, setoff + j)) == 0x20)
				break;
			else {
				if (j == 0x8)
					filefullname += '.';
				filefullname += tempchar;
			}
	}

	// judge the start of 32 bytes to determine if the long filename end
	// judge the 0xb to determin if this is short filename
	// or add an hash function to make sure the short filename come from the long one?? NO! NEVER!
	return filefullname;
}

std::vector<Fileinfo> & getnmtm(unsigned int bs, unsigned int directory, unsigned int root, unsigned int cluster) {
	unsigned int fileflag = loaddigit(bs, 1, directory, 0xb);                                       // 长文件名标识位置0xb
	unsigned int filesize, filedate, filetime;
	unsigned int subfolder;
	unsigned int filenamefirstchar;
	std::tm fulltime;
	std::string filefullname;
	for (int i = 0; loaddigit(bs, 1, directory, 0 + 32 * i) != 0 && i * 32 < bs * cluster; ++i) {          // there still directory exist && do not step over a cluster
		if (fileflag != 0xf)    // it's a short file name. Use it!             // 长文件名标识位置0xb
			if ((filesize = loaddigit(bs, 4, directory, 0x1b + 32 * i)) == 0) {   // if is a directory  // 文件大小0x1c-0x1f 数值为0为目录
				if ((filenamefirstchar = loaddigit(bs, 1, directory, 0x0 + 32 * i)) != 0x2e && filenamefirstchar != 0xe5) {	// if not '.' or '..' or hidden file or deleted file
					filetime = loaddigit(bs, 2, directory, 0xe + 32 * i);		// 创建时间0xe-0xf      修改时间0x16-0x17
					filedate = loaddigit(bs, 2, directory, 0x10 + 32 * i);		// 创建日期0x10-0x11    修改日期0x18-0x19
					fulltime = calctm(filedate, filetime);                  // calculate the date and time
					filefullname = getfilename(bs, directory, 32 * i);
					result.emplace_back(Fileinfo(filefullname, fulltime));
					subfolder = loaddigit(bs, 2, directory, 0x1a + 32 * i) + 0x100 * loaddigit(bs, 2, directory, 0x14 + 32 * i);
					getnmtm(bs, (subfolder -2) * cluster + root, root, cluster);
				}
				// else continue
			}
			else {   // if is a file
				filetime = loaddigit(bs, 2, directory, 0xe + 32 * i);   // 创建时间0xe-0xf      修改时间0x16-0x17
				filedate = loaddigit(bs, 2, directory, 0x10 + 32 * i);  // 创建日期0x10-0x11    修改日期0x18-0x19
				fulltime = calctm(filedate, filetime);       // calculate the date and time
				filefullname = getfilename(bs, directory, 32 * i);
				result.emplace_back(Fileinfo(filefullname, fulltime));
			}

			fileflag = loaddigit(bs, 1, directory, 0xb + 32 * (i + 1));
	}
	return result;
}

std::ostream & operator<<(std::ostream & os, const std::tm & t) {
	os << std::setw(2) << std::setfill('0') << t.tm_mon << '/' << std::setw(2) << std::setfill('0') << t.tm_mday << '/' << std::setw(4) << std::setfill('0') << t.tm_year << ' '
		<< std::setw(2) << std::setfill('0') << t.tm_hour << ':' << std::setw(2) << std::setfill('0') << t.tm_min << ':' << std::setw(2) << std::setfill('0') << t.tm_sec;
	return os;
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
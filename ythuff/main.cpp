int UserPreVerified = 0;

#include "compress.h"
using namespace compress;

#include "sys/stat.h"

const int _unit_size[] = { 0, 4, 1, 0, 2 }; // 1-S[2,4,8] 2-N[1-7] 4-F[2-7]

void encode(IO &, struct _stati64 &, const int = 2);
void decode(IO &, const int = 2);
void args(int argc, char *argv[], int &mode, string &infile, string &outfile);

#include <ctime>
clock_t global_time, time_start;
inline void interval(const string prompt = NULL)
{
	if (!prompt.empty())
	{
		std::cout << prompt << std::endl;
	}
	clock_t t = time_start;
	time_start = clock();
	t = time_start - t;
	std::cout << "Time used: " << t / 1000 << '.' << t % 1000 << "s." << std::endl;
	return;
}

int main(int argc, char *argv[])
{
	int mode = -1;
	string infile_name, outfile_name;
	args(argc, argv, mode, infile_name, outfile_name);

	global_time = clock();
	IO io(infile_name, outfile_name);
	if (mode)
	{
		if (mode < 0)
		{
			exit_p("Error: Unknown error");
		}

		struct _stati64 stat_in;
		_stati64(infile_name.c_str(), &stat_in);
		std::cout << "File size: " << stat_in.st_size << " Bytes." << std::endl;

		time_start = clock();
		if ((mode & 4) && (stat_in.st_size > 2e6))
		{
			encode(io, stat_in, 4); //Fast
		}
		else if ((mode & 1) && stat_in.st_size <= UINT16_MAX)
		{
			encode(io, stat_in, 1); //Semi
		}
		else
		{
			encode(io, stat_in);
		}

		std::cout << "Compression done successfully!" << std::endl;
	}
	else
	{
		vector<char> buf;

		std::cout << "Checking file..." << std::endl;

		time_start = clock();
		io.get(buf, 4);
		if (buf.size() < 4 || buf[0] != 'Y' || buf[1] != 'T' || buf[3])
		{
			exit_p("Error: This file cannot be unzipped as it is not generated by this program.");
		}

		switch (buf[2])
		{
		case 4:
		case 2:
		case 1:
			decode(io, buf[2]);
			break;
		default:
			exit_p("Error: This file cannot be unzipped as it is not generated by this program.");
		}

		std::cout << "Extraction done successfully!" << std::endl;
	}
	io.flush();

	global_time = clock() - global_time;
	std::cout << "Total time used: " << global_time / 1000 << '.' << global_time % 1000 << "s." << std::endl;

	return 0;
}
void encode(IO &io, struct _stati64 &stat_in, const int mode)
{
	switch (mode)
	{
	case 4:
		std::cout << "Fast mode avaliable!" << std::endl;
	case 2:
		break;
	case 1:
		std::cout << "Semi mode avaliable!" << std::endl;
		break;
	default:
		exit_p("Error: Unknown error");
	}

	vector<char> buf;

	std::cout << "Scanning file..." << std::endl;

	int64_t unit_size = _unit_size[mode], stat_size = 1LL << (unit_size << 3);
	if (mode == 1)
	{
		stat_size = 1LL << unit_size;
	}
	vector<int64_t> sum(stat_size, 0);

	for (int64_t x; !io.eof();)
	{
		uint64_t tmp;
		if (mode != 1)
		{
			tmp = io.get(buf, unit_size);
		}
		else
		{
			tmp = io.getbit(buf, unit_size);
		}

		x = 0;
		for (auto it = buf.rbegin();it != buf.rend();it++)
		{
			x <<= 8;
			x |= *it & 0xff;
		}
		if (tmp < unit_size)
		{
			if (!tmp)
			{
				break;
			}
			if (mode != 1)
			{
				tmp <<= 3;
			}
			x &= (1LL << tmp) - 1;
		}

		sum.at(x)++;
	}

	interval("Done!");
	std::cout << "Building dictionary..." << std::endl;

	HuffmanTree ht(sum, stat_size, _unit_size[mode], (mode == 4) ? 8 : 1);

	vector<vector<char>> map(stat_size);
	vector<int64_t> len(stat_size);
	ht.hash(map, len);

	string seq;
	vector<int64_t> data;
	ht.sequence(seq, data);

	interval("Done!");
	std::cout << "Starting compression..." << std::endl;

	buf.clear();
	buf.resize(4, 0);
	buf.at(0) = 'Y';
	buf.at(1) = 'T';
	buf.at(2) = mode;
	io.put(buf, 4);

	{
		buf.clear();
		uint64_t tmp = stat_in.st_size;
		for (int i = 0;i < sizeof(uint64_t);i++)
		{
			buf.push_back(tmp & 0xff);
			tmp >>= 8;
		}
		if (mode != 1)
		{
			io.put(buf, sizeof(uint64_t));
		}
		else
		{
			io.put(buf, sizeof(uint16_t));
		}
	}

	{
		vector<char> HT_seq_bit;
		oz2bit(seq, HT_seq_bit);
		io.put(HT_seq_bit, HT_seq_bit.size());
	}

	buf.clear();
	if (mode != 1)
	{
		buf.reserve(stat_size * unit_size);
		for (auto i : data)
		{
			for (int64_t j = 0;j < unit_size;j++)
			{
				buf.push_back(i & 0xff);
				i >>= 8;
			}
		}
		io.put(buf, buf.size());
	}
	else
	{
		int64_t temp_size = (unit_size + 7) >> 3;
		buf.reserve(temp_size);
		for (auto i : data)
		{
			buf.clear();
			for (int64_t j = 0;j < temp_size;j++)
			{
				buf.push_back(i & 0xff);
				i >>= 8;
			}
			io.putbit(buf, unit_size);
		}
	}

	io.reset();
	for (int64_t x; !io.eof();)
	{
		uint64_t tmp;
		if (mode != 1)
		{
			tmp = io.get(buf, unit_size);
		}
		else
		{
			tmp = io.getbit(buf, unit_size);
		}
		x = 0;
		for (auto it = buf.rbegin();it != buf.rend();it++)
		{
			x <<= 8;
			x |= *it & 0xff;
		}
		if (tmp < unit_size)
		{
			if (mode != 1)
			{
				tmp <<= 3;
			}
			x &= (1LL << tmp) - 1;
		}

		if (mode == 4)
		{
			io.put(map.at(x), map.at(x).size());
		}
		else
		{
			io.putbit(map.at(x), len.at(x));
		}
	}

	interval("Done!");
	return;
}

void decode(IO &io, const int mode)
{
	switch (mode)
	{
	case 4:
		std::cout << "Fast mode detected!" << std::endl;
	case 2:
		break;
	case 1:
		std::cout << "Semi mode detected!" << std::endl;
		break;
	default:
		exit_p("Error: Unknown error");
	}

	vector<char> buf;

	uint64_t size = 0;
	if (mode != 1)
	{
		io.get(buf, sizeof(uint64_t));
	}
	else
	{
		io.get(buf, sizeof(uint16_t));
	}
	for (auto it = buf.rbegin();it != buf.rend();it++)
	{
		size <<= 8;
		size |= *it & 0xff;
	}

	int sn_size = (mode == 4) ? 8 : 1;
	int64_t unit_size = _unit_size[mode];
	int64_t node_sum = (1LL << sn_size) * (1LL << (unit_size << 3)) / ((1LL << sn_size) - 1);
	if (mode == 1)
	{
		node_sum = (1LL << sn_size) * (1LL << unit_size) / ((1LL << sn_size) - 1);
	}

	string seq;
	{
		vector<char> HT_seq_bit;
		HT_seq_bit.resize((node_sum + 7) >> 3);
		io.get(HT_seq_bit, HT_seq_bit.size());
		bit2oz(HT_seq_bit, seq, node_sum);
	}
	if (mode == 1)
	{
		unit_size = -unit_size;
	}
	HuffmanTree ht(seq, io, unit_size, sn_size);

	interval("Done!");
	std::cout << "Start extracting..." << std::endl;

	if (mode == 1)
	{
		unit_size = -unit_size;
		size <<= 3;
	}
	for (unsigned char x = 0; size;)
	{
		if (mode != 4)
		{
			io.getbit(buf, sn_size);
		}
		else
		{
			io.get(buf);
		}
		x = buf.front();

		ht.trans(x);

		if (ht.status())
		{
			ht.flush(buf);

			uint64_t tmp = (size < unit_size) ? size : unit_size;
			if (mode != 1)
			{
				tmp = io.put(buf, tmp);
			}
			else
			{
				tmp = io.putbit(buf, tmp);
			}
			size -= tmp;
		}
	}
	interval("Done!");
	return;
}

#include "getopt.h"
#include <iterator>

void args(int argc, char *argv[], int &mode, string &infile, string &outfile)
{
	if (argc > 1)
	{
		for (int flag; (flag = getopt(argc, argv, "hkDdEeFfSsi:o:YyNn")) != -1;)
		{
			switch (flag)
			{
			case 'k':
				goto KEYBOARD;
			case 'h':
				std::cout << "usage: ythuff [-hkf] -ed -i IN [-o OUT]" << std::endl
					<< "-h Print this help message" << std::endl
					<< "-k Using keyboard input with hints" << std::endl
					<< "-e encode mode" << std::endl
					<< "-d decode mode" << std::endl
					<< "-i IN specify input file" << std::endl
					<< "-o OUT specify output file" << std::endl;
			case '?':
				exit(0);
			case 'D':
			case 'd':
				if (mode != -1)
				{
					exit_p("Error: You may not specify more than one `-ed' option");
				}
				mode = 0;
				break;
			case 'E':
			case 'e':
				if (mode != -1)
				{
					exit_p("Error: You may not specify more than one `-ed' option");
				}
				mode = 2;
				break;
			}
		}
		if (mode < 0)
		{
			exit_p("Error: Working mode unselected");
		}
		optind = 1;
		for (int flag; (flag = getopt(argc, argv, "hkDdEeFfSsi:o:YyNn")) != -1;)
		{
			switch (flag)
			{
			case 'F':
			case 'f':
				mode |= (mode != 0) << 2;
				break;
			case 'S':
			case 's':
				mode |= (mode != 0) << 0;
				break;
			case 'I':
			case 'i':
				infile.assign(optarg);
				break;
			case 'O':
			case 'o':
				outfile.assign(optarg);
				break;
			case 'Y':
			case 'y':
				UserPreVerified = 1;
				break;
			case 'N':
			case 'n':
				UserPreVerified = -1;
				break;
			}
		}
		if (outfile.empty())
		{
			outfile = infile + ".ytc";
			std::cout << outfile << " will be used as the output filename." << std::endl;
		}
	}
	else
	{
	KEYBOARD:
		string buffer;
		for (;buffer.empty();)
		{
			line_input(buffer, "Please select mode, (e)ncode or (d)ecode:");
		}
		switch (buffer.front())
		{
		case 'D':
		case 'd':
			mode = 0;
			break;
		case 'E':
		case 'e':
			mode = 2;
			break;
		default:
			exit_p("Error: Unknown mode code");
		}
		line_input(buffer, "Please input the source filename:");
		infile = buffer;
		line_input(buffer, "Please input the output filename or press <Enter> to use the default:");
		if (buffer.empty())
		{
			outfile = infile + ".ytc";
			std::cout << outfile << " will be used as the output filename." << std::endl;
		}
		else
		{
			outfile = buffer;
		}
	}
	return;
}
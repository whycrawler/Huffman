#include "compress.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <queue>

extern int UserPreVerified;

namespace compress
{
	inline void exit_p(const string &prompt, const int _Code)
	{
		std::cerr << prompt << std::endl;
		exit(_Code);
	}
	inline void line_input(string &buf, const string &prompt)
	{
		if (!prompt.empty())
		{
			std::cout << prompt << std::endl; //输入提示
		}
		std::getline(std::cin, buf);          //从键盘输入行
		if (std::cin.fail())
		{
			exit_p("Error: cin fail");
		}
		buf.erase(0, buf.find_first_not_of(' '));
		return;
	}
	int64_t oz2bit(const string &src, vector<char> &dst)
	{
		dst.clear();
		int64_t res = 0;
		for (auto ch : src)
		{
			int bit = res & 7;
			if (!bit)
			{
				dst.push_back(0);
			}
			dst.back() |= (ch != '0') << bit;
			res++;
		}
		return res;
	}
	int64_t bit2oz(const vector<char> &src, string &dst, int64_t size)
	{
		dst.clear();
		int64_t res = 0;
		for (; res < size; res++)
		{
			dst.push_back('1' ^ !(src.at(res >> 3) & (1 << (res & 7))));
		}
		return res;
	}
	HuffmanTree::HuffmanTree(vector<int64_t> &sum, int64_t _stat_size, int64_t unit_size, int sn_size)
	{
		using std::make_pair;
		using std::pair;

		std::vector<node> pool;
		std::priority_queue<pair<int64_t, int64_t>, vector<pair<int64_t, int64_t>>, std::greater<pair<int64_t, int64_t>>> q;

		_unit_size = unit_size;
		_sn_size = sn_size;
		sn_size = 1 << sn_size;
		_size = sn_size * _stat_size / (sn_size - 1);
		pool.reserve(_size);
		for (int64_t i = 0; i < _stat_size; i++)
		{
			pool.push_back(node(i, sum.at(i)));
			q.push(make_pair(sum.at(i), i));
		}
		for (; q.size() > 1;)
		{
			node tmp = node();
			tmp.sn.reserve(sn_size);
			for (int i = 0; i < sn_size; i++)
			{
				tmp.sn.push_back(pool.at(q.top().second));
				if (tmp.sn.back().sn.empty())
				{
					tmp.data++;
				}
				else
				{
					tmp.data += tmp.sn.back().data;
				}
				tmp.weight += tmp.sn.back().weight;
				q.pop();

			}
			q.push(make_pair(tmp.weight, pool.size()));
			pool.push_back(tmp);
		}
		root = pool.back();
		now_node = &root;
		return;
	}
	void HuffmanTree::build_dfs(node &rt, string &seq, IO &io, string::iterator  &now)
	{
		if (*(now++) == '0')
		{
			vector<char> buf;
			rt.data = 0;
			if (_unit_size < 0)
			{
				io.getbit(buf, -_unit_size);
				rt.data = buf.at(0);
				return;
			}

			io.get(buf, _unit_size);
			for (auto it = buf.rbegin();it != buf.rend();it++)
			{
				rt.data <<= 8;
				rt.data |= *it & 0xff;
			}
			return;
		}

		int sn_size = 1 << _sn_size;
		rt.sn.clear();
		rt.sn.resize(sn_size, node());
		for (auto &i : rt.sn)
		{
			build_dfs(i, seq, io, now);
		}
		return;
	}
	HuffmanTree::HuffmanTree(string &seq, IO &io, int64_t unit_size, int sn_size)
	{
		_sn_size = sn_size;
		sn_size = 1 << sn_size;
		_unit_size = unit_size;
		root = node();
		string::iterator now = seq.begin();
		build_dfs(root, seq, io, now);
		now_node = &root;
		return;
	}
	void HuffmanTree::hash_dfs(const node &now, vector<char> &seq, vector<vector<char>>&map, vector<int64_t>&len, int bit)
	{
		if (now.sn.empty())
		{
			map.at(now.data) = seq;
			len.at(now.data) = (seq.size() << 3) + bit - 8;
			return;
		}
		if (bit == _hash_size)
		{
			seq.push_back(0);
			bit = 0;
		}
		int sn_size = 1 << _sn_size;
		for (int i = 0; i < sn_size; i++)
		{
			seq.back() &= ((1 << bit) - 1);
			seq.back() |= i << bit;
			hash_dfs(now.sn.at(i), seq, map, len, bit + _sn_size);
		}
		if (!bit)
		{
			seq.pop_back();
		}
		return;
	}
	void HuffmanTree::hash(vector<vector<char>>&map, vector<int64_t>&len, int hash_size)
	{
		vector<char> seq;
		seq.clear();
		_hash_size = hash_size;
		hash_dfs(root, seq, map, len, _hash_size);
		return;
	}
	void HuffmanTree::sequence_dfs(const node &now, string &seq, vector<int64_t> &data)
	{
		if (now.sn.empty())
		{
			data.push_back(now.data);
			seq.push_back('0');
			return;
		}
		seq.push_back('1');
		int sn_size = 1 << _sn_size;
		for (auto &i : now.sn)
		{
			sequence_dfs(i, seq, data);
		}
		return;
	}
	void HuffmanTree::sequence(string &seq, vector<int64_t> &data)
	{
		seq.clear();
		seq.reserve(size());
		data.clear();
		sequence_dfs(root, seq, data); //构建复原序列及对应数据
		return;
	}
	uint64_t HuffmanTree::size()
	{
		return _size;
	}
	void HuffmanTree::trans(unsigned char dst)
	{
		now_node = &(now_node->sn.at(dst));
		return;
	}
	int HuffmanTree::status()
	{
		return now_node->sn.empty();
	}
	void HuffmanTree::flush(vector<char> &dst)
	{
		dst.clear();
		auto tmp = now_node->data;
		for (int i = 0;i < sizeof(tmp);i++)
		{
			dst.push_back(tmp & 0xff);
			tmp >>= 8;
		}
		now_node = &root;
		return;
	}
	HuffmanTree::node::node(int64_t _data, int64_t _weight)
	{
		data = _data;
		weight = _weight;
		sn.clear();
	}



	IO::IO(const string &infile, const string &outfile)
	{
		using std::fstream;
		_in.file.open(infile, fstream::in | fstream::binary);
		if (!_in.file.is_open())
		{
			exit_p("Error: Error while opening source file");
		}
		_in.now = batch_size;
		_in.bit = 8;
		_in.cache.resize(batch_size, 0);
		_out.file.open(outfile, fstream::in | fstream::binary);
		if (_out.file.is_open())
		{
			_out.file.close();
			std::cout << "Warning: The output file already exists." << std::endl;
			if (UserPreVerified)
			{
				if (UserPreVerified < 0)
				{
					exit_p("As defination by user, exit without modification.");
				}
				std::cout << "As defination by user, overwrite the file." << std::endl;
			}
			else
			{
				string buffer;
				for (; buffer.empty();)
				{
					line_input(buffer, "Do you want to overwrite it? (y or [n])");
					if (buffer.empty())
					{
						exit_p("Exit without modification.");
					}
					switch (buffer.front())
					{
					case 'N':
					case 'n':
						exit_p("Exit without modification.");
					case 'Y':
					case 'y':
						break;
					default:
						buffer.clear();
					}
				}
			}
		}
		_out.file.open(outfile, fstream::out | fstream::binary);
		if (!_out.file.is_open())
		{
			exit_p("Error: Unknown error while creating destination file");
		}
		_out.bit = 8;
		_out.cache.clear();
		_out.cache.reserve(batch_size);
		return;
	}
	int IO::eof()
	{
		return _in.file.eof() && (_in.now >= _in.cache.size());
	}
	void IO::flush()
	{
		_out.file.write(_out.cache.data(), _out.cache.size());
		_out.bit = 0;
		_out.cache.clear();
		return;
	}
	uint64_t IO::get(vector<char> &dst, uint64_t size, int warn)
	{
		if (eof())
		{
			exit_p("Error: Reading with EOF");
		}
		if ((_in.bit & 7) && warn)
		{
			std::cout << "Warning: Getting a byte with " << _in.bit << "bits read.\n" << std::endl;
		}
		dst.clear();
		uint64_t res = 0;
		for (; res < size;)
		{
			if (_in.now >= _in.cache.size())
			{
				if (_in.cache.size() < batch_size)
				{
					break;
				}
				_in.cache.resize(batch_size, 0);
				_in.file.read(_in.cache.data(), batch_size);
				_in.cache.resize(_in.file.gcount());
				_in.now = 0;
				continue;
			}
			uint64_t len = std::min(size - res, _in.cache.size() - _in.now);
			dst.insert(dst.end(), _in.cache.begin() + _in.now, _in.cache.begin() + _in.now + len);
			_in.now += len;
			res += len;
		}
		if (res)
		{
			_in.bit = 0;
		}
		return res;
	}
	uint64_t IO::getbit(vector<char> &dst, uint64_t size)
	{
		if (eof())
		{
			return 0;
			exit_p("Error: Reading with EOF");
		}
		dst.clear();
		uint64_t res = 0;
#ifdef USE_BYTE_FOR_BITS
		if ((_in.bit & 7) == 0)
		{
			res = get(dst, size >> 3) << 3;
		}
#endif
		for (int bit = 8; res < size;)
		{
			if (bit == 8)
			{
				bit = 0;
				dst.push_back(0);
			}
			if (_in.bit == 8)
			{
				_in.bit = 0;
				_in.now++;
			}
			if (_in.now >= _in.cache.size())
			{
				if (_in.bit & 7)
				{
					exit_p("Error: Something ridiculous happend while get bits");
				}
				if (_in.cache.size() < batch_size)
				{
					return res;
				}
				_in.cache.resize(batch_size);
				_in.file.read(_in.cache.data(), batch_size);
				_in.cache.resize(_in.file.gcount());
				_in.now = 0;
				continue;
			}
			uint64_t len = std::min(size - res, (uint64_t)8 - std::max(_in.bit, bit));
			for (uint64_t i = 0; i < len; i++)
			{
				char dst_mask = 1 << bit, src_mask = 1 << _in.bit;
				dst.back() = (dst.back() & (~dst_mask)) | (((_in.cache.at(_in.now) & src_mask) != 0) << bit);
				bit++;
				_in.bit++;
			}
			res += len;
		}
		return res;
	}
	uint64_t IO::put(const vector<char> &src, uint64_t size, int warn)
	{
		if (_out.bit < 8)
		{
			if (warn)
			{
				std::cout << "Warning: Putting a byte with " << 8 - _out.bit << "bits unset." << std::endl;
			}
			_out.bit = 8;
		}
		uint64_t res = 0;
		for (; res < size;)
		{
			uint64_t len = std::min(size - res, batch_size - _out.cache.size());
			_out.cache.insert(_out.cache.end(), src.begin() + res, src.begin() + res + len);
			res += len;
			if (_out.cache.size() == batch_size)
			{
				_out.file.write(_out.cache.data(), _out.cache.size());
				_out.cache.clear();
			}
		}
		return res;
	}
	uint64_t IO::putbit(const vector<char> &src, uint64_t size)
	{
		uint64_t res = 0;
#ifdef USE_BYTE_FOR_BITS
		if (_out.bit == 0)
		{
			res = put(src, size >> 3) << 3;
		}
#endif
		auto now = src.begin() + (res >> 3);
		for (int bit = 0; res < size && now != src.end();)
		{
			if (_out.bit == 8)
			{
				if (_out.cache.size() == batch_size)
				{
					_out.file.write(_out.cache.data(), _out.cache.size());
					_out.cache.clear();
				}
				_out.cache.push_back(0);
				_out.bit = 0;
				continue;
			}
			uint64_t len = std::min(size - res, (uint64_t)8 - std::max(_out.bit, bit));
			for (int i = 0; i < len; i++)
			{
				char dst_mask = 1 << _out.bit, src_mask = 1 << bit;
				_out.cache.back() = (_out.cache.back() & (~dst_mask)) | ((((*now) & src_mask) != 0) << _out.bit);
				bit++;
				_out.bit++;
			}
			if (bit == 8)
			{
				bit = 0;
				now++;
			}
			res += len;
		}
		return res;
	}
	void IO::reset()
	{
		_in.file.clear();
		_in.file.seekg(0, _in.file.beg);
		_in.now = batch_size;
		_in.bit = 8;
		_in.cache.resize(batch_size, 0);
		return;
	}
} // namespace compress

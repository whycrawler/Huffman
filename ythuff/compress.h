#ifndef SELF_FILE_COMPRESS
#define SELF_FILE_COMPRESS

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using std::string;
using std::vector;

namespace compress
{
	void exit_p(const string &, const int = 0);
	void line_input(string &, const string & = "");
	int64_t oz2bit(const string &, vector<char> &);
	int64_t bit2oz(const vector<char> &, string &, int64_t);
	class IO
	{
	public:
		IO(const string &, const string &);
		int eof();
		void flush();
		uint64_t get(vector<char> &, uint64_t = 1, int = 1);
		uint64_t getbit(vector<char> &, uint64_t = 1);
		uint64_t put(const vector<char> &, uint64_t = 1, int = 1);
		uint64_t putbit(const vector<char> &, uint64_t = 1);
		void reset();

	private:
		static const int batch_size = 65536;
		struct Cache
		{
			std::fstream file;
			vector<char> cache;
			int bit, now;
		} _in, _out;
	};
	class HuffmanTree
	{
	private:
		int _sn_size, _hash_size;
		int64_t _unit_size;
		uint64_t _size;
		struct node
		{
			node(int64_t = 0, int64_t = 0);
			int64_t data, weight;
			vector<node> sn;
		};
		node root, *now_node;
		void build_dfs(node &, string &, IO &, string::iterator  &);
		void hash_dfs(const node &, vector<char> &, vector<vector<char>>&, vector<int64_t>&, int);
		void sequence_dfs(const node &, string &, vector<int64_t> &);

	public:
		HuffmanTree(vector<int64_t> &, int64_t, int64_t, int);
		HuffmanTree(string &, IO &, int64_t = 1, int = 1);
		void hash(vector<vector<char>>&, vector<int64_t>&, int = 8);
		void sequence(string &, vector<int64_t> &);
		uint64_t size();
		void trans(unsigned char);
		int status();
		void flush(vector<char> &);
	};
} // namespace compress

#endif
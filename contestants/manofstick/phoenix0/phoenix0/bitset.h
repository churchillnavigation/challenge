#include <vector>

using namespace std;

struct BitSet
{
	struct iterator
	{
		const vector<uint64_t>& _data;
		int _index;
		uint64_t _bit;

		iterator(const vector<uint64_t>& data)
			: _data(data)
		{
			_index = 0;
			_bit = 1;
		}

		iterator(const vector<uint64_t>& data, int index, uint64_t bit)
			: _data(data)
			, _index(index)
			, _bit(bit)
		{}

		bool operator*() const
		{
			return (_data[_index] & _bit) == _bit;
		}

		bool operator==(const iterator& rhs) const
		{
			return _index == rhs._index
				&& _bit == rhs._bit;
		}

		bool operator!=(const iterator& rhs) const
		{
			return !(*this == rhs);
		}

		iterator& operator++()
		{
			_bit <<= 1;
			if (!_bit)
			{
				_bit = 1;
				++_index;
			}
			return *this;
		}
	};

	struct values_iterator {
		const vector<uint64_t>& _data;
		int _index;
		uint64_t _bit;
		int _bit_value;

		const int _end_index;
		const uint64_t _end_bit;

		values_iterator(const vector<uint64_t>& data, int end_index, uint64_t end_bit)
			: _data(data)
			, _end_index(end_index)
			, _end_bit(end_bit)
		{
			_index = 0;
			_bit = 0;
			_bit_value = 0;
			++(*this);
		}

		int operator*() const
		{
			return _index * 64 + _bit_value;
		}

		bool at_end()
		{
			return _index == _end_index && _bit == _end_bit;
		}

		values_iterator& operator++()
		{
			while (!(_index == _end_index && _bit == _end_bit))
			{
				if (_index < _end_index && _data[_index] == 0)
				{
					++_index;
					_bit = 0;
					_bit_value = 0;
					continue;
				}

				if (_bit == 0)
				{
					_bit = 1;
					_bit_value = 0;
				}
				else
				{
					_bit <<= 1;
					_bit_value++;
				}

				while (_bit_value < 64)
				{
					if ((_index == _end_index && _bit == _end_bit) || _data[_index] & _bit)
						goto found;
					_bit <<= 1;
					_bit_value++;
				}

				++_index;
				_bit = 0;
				_bit_value = 0;
			}
		found:
			return *this;
		}
	};

	uint64_t _count;
	vector<uint64_t> _data;
	const iterator _end;

	BitSet(uint32_t count, bool set=false)
		: _count(count)
		, _data(count / 64 + (count % 64 == 0 ? 0 : 1), set ? numeric_limits<uint64_t>::max() : uint64_t(0))
		, _end(_data, count / 64, uint64_t(1) << ((count % uint64_t(64)) % uint64_t(64)))
	{}

	void set(uint32_t bit)
	{
		uint32_t idx = bit / 64;
		uint64_t offset = uint64_t(1) << (bit % uint64_t(64));
		_data[idx] |= offset;
	}

	void reset(uint32_t bit)
	{
		uint32_t idx = bit / 64;
		uint64_t offset = uint64_t(1) << (bit % uint64_t(64));
		_data[idx] &= ~offset;
	}

	iterator begin()
	{
		return iterator(_data);
	}

	const iterator& end()
	{
		return _end;
	}

	values_iterator get_values_iterator()
	{
		return values_iterator(_data, _end._index, _end._bit);
	}

	bool operator[](int idx) const
	{
		uint64_t bit = uint64_t(1) << (idx % 64);
		return (_data[idx / 64] & bit) == bit;
	}

	void clear()
	{
		fill(_data.begin(), _data.end(), uint64_t(0));
	}
};

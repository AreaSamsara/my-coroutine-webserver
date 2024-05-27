#include "bytearray.h"
#include <iomanip>
#include <cstring>
#include <fstream>
#include <sstream>

#include "log.h"
#include "singleton.h"
#include "utility.h"

namespace ByteArraySpace
{
	using namespace LogSpace;
	using namespace SingletonSpace;
	using namespace UtilitySpace;
	using std::out_of_range;
	using std::ofstream;
	using std::ifstream;

	//class Node:public
	ByteArray::Node::Node(const size_t block_size) :m_ptr(new char[block_size]), m_next(nullptr), m_block_size(block_size) {}
	ByteArray::Node::Node() :m_ptr(nullptr), m_next(nullptr), m_block_size(0) {}
	ByteArray::Node::~Node()
	{
		if (m_ptr)
		{
			delete[] m_ptr;
		}
	}

	//class ByteArray:public
	ByteArray::ByteArray(const size_t block_size) :m_block_size(block_size), m_capacity(block_size),
		m_data_size(0), m_position(0), m_root(new Node(block_size)), m_current(m_root) {}
	ByteArray::~ByteArray()
	{
		//从根节点开始依次释放所有节点的内存
		Node* temp = m_root;
		while (temp)
		{
			m_current = temp;
			temp = temp->m_next;
			delete m_current;
		}
	}

	//write
	void ByteArray::writeFint8(const int8_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeFuint8(const uint8_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeFint16(const int16_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeFuint16(const uint16_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeFint32(const int32_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeFuint32(const uint32_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeFint64(const int64_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeFuint64(const uint64_t value)
	{
		write(&value, sizeof(value));
	}


	static uint32_t EncodeZigzag32(const int32_t value)
	{
		if (value < 0)
		{
			return (uint32_t(-value)) * 2 - 1;
		}
		else
		{
			return value * 2;
		}
	}

	static uint64_t EncodeZigzag64(const int64_t value)
	{
		if (value < 0)
		{
			return (uint64_t(-value)) * 2 - 1;
		}
		else
		{
			return value * 2;
		}
	}

	static int32_t DecodeZigzag32(const uint32_t value)
	{
		return (value >> 1) ^ -(value & 1);
	}

	static int64_t DecodeZigzag64(const uint64_t value)
	{
		return (value >> 1) ^ -(value & 1);
	}

	void ByteArray::writeInt32(const int32_t value)
	{
		writeUint32(EncodeZigzag32(value));
	}
	void ByteArray::writeUint32(uint32_t value)
	{
		uint8_t temp[5];
		uint8_t i = 0;
		while (value >= 0x80)
		{
			temp[i++] = (value & 0x7F) | 0x80;
			value >>= 7;
		}
		temp[i++] = value;
		write(temp, i);
	}
	void ByteArray::writeInt64(const int64_t value)
	{
		writeUint64(EncodeZigzag64(value));
	}
	void ByteArray::writeUint64(uint64_t value)
	{
		uint8_t temp[5];
		uint8_t i = 0;
		while (value >= 0x80)
		{
			temp[i++] = (value & 0x7F) | 0x80;
			value >>= 7;
		}
		temp[i++] = value;
		write(temp, i);
	}

	void ByteArray::writeFloat(const float value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeDouble(const double value)
	{
		write(&value, sizeof(value));
	}

	//length:int16,data
	void ByteArray::writeStringF16(const string& value)
	{
		writeFuint16(value.size());
		write(value.c_str(), value.size());
	}
	//length:int32,data
	void ByteArray::writeStringF32(const string& value)
	{
		writeFuint32(value.size());
		write(value.c_str(), value.size());
	}
	//length:int64,data
	void ByteArray::writeStringF64(const string& value)
	{
		writeFuint64(value.size());
		write(value.c_str(), value.size());
	}
	//length:varint,data
	void ByteArray::writeStringVint(const string& value)
	{
		writeFuint64(value.size());
		write(value.c_str(), value.size());
	}
	//data
	void ByteArray::writeStringWithoutLength(const string& value)
	{
		write(value.c_str(), value.size());
	}


	//read 
	int8_t ByteArray::readFint8()
	{
		int8_t value;
		read(&value, sizeof(value));
		return value;
	}
	uint8_t ByteArray::readFuint8()
	{
		uint8_t value;
		read(&value, sizeof(value));
		return value;
	}
	int16_t ByteArray::readFint16()
	{
		int16_t value;
		read(&value, sizeof(value));
		return value;
	}
	uint16_t ByteArray::readFuint16()
	{
		uint16_t value;
		read(&value, sizeof(value));
		return value;
	}
	int32_t ByteArray::readFint32()
	{
		int32_t value;
		read(&value, sizeof(value));
		return value;
	}
	uint32_t ByteArray::readFuint32()
	{
		uint32_t value;
		read(&value, sizeof(value));
		return value;
	}
	int64_t ByteArray::readFint64()
	{
		int64_t value;
		read(&value, sizeof(value));
		return value;
	}
	uint64_t ByteArray::readFuint64()
	{
		uint64_t value;
		read(&value, sizeof(value));
		return value;
	}

	int32_t ByteArray::readInt32()
	{
		return DecodeZigzag32(readUint32());
	}
	uint32_t ByteArray::readUint32()
	{
		uint32_t result = 0;
		for (int i = 0; i < 32; i += 7)
		{
			uint8_t b = readFuint8();
			if (b < 0x80)
			{
				result |= ((uint32_t)b) << i;
				break;
			}
			else
			{
				result |= ((uint32_t)(b & 0x7F)) << i;
			}
		}
		return result;
	}
	int64_t ByteArray::readInt64()
	{
		return DecodeZigzag64(readUint64());
	}
	uint64_t ByteArray::readUint64()
	{
		uint64_t result = 0;
		for (int i = 0; i < 32; i += 7)
		{
			uint8_t b = readFuint8();
			if (b < 0x80)
			{
				result |= ((uint64_t)b) << i;
				break;
			}
			else
			{
				result |= ((uint64_t)(b & 0x7F)) << i;
			}
		}
		return result;
	}

	float ByteArray::readFloat()
	{
		uint32_t value = readFuint32();
		float float_value;
		memcpy(&float_value, &value, sizeof(value));
		return float_value;
	}
	double ByteArray::readDouble()
	{
		uint64_t value = readFuint64();
		float float_value;
		memcpy(&float_value, &value, sizeof(value));
		return float_value;
	}

	//length:int16,data
	string ByteArray::readStringF16()
	{
		uint16_t length = readFuint16();
		string buffer;
		buffer.resize(length);
		read(&buffer[0], length);
		return buffer;
	}
	//length:int32,data
	string ByteArray::readStringF32()
	{
		uint32_t length = readFuint32();
		string buffer;
		buffer.resize(length);
		read(&buffer[0], length);
		return buffer;
	}
	//length:int64,data
	string ByteArray::readStringF64()
	{
		uint64_t length = readFuint64();
		string buffer;
		buffer.resize(length);
		read(&buffer[0], length);
		return buffer;
	}
	//length:varint,data
	string ByteArray::readStringVint()
	{
		uint64_t length = readFuint64();
		string buffer;
		buffer.resize(length);

		read(&buffer[0], length);
		return buffer;
	}

	//内部操作
	void ByteArray::clear()
	{
		m_position = 0;;
		m_data_size = 0;
		m_capacity = m_block_size;
		Node* temp = m_root->m_next;
		while (temp)
		{
			m_current = temp;
			temp = temp->m_next;
			delete m_current;
		}
		m_current = m_root;
		m_root->m_next = NULL;
	}

	void ByteArray::write(const void* buffer, size_t size)
	{
		if (size == 0)
		{
			return;
		}
		expendCapacity(size);

		size_t now_position = m_position % m_block_size;
		size_t now_capacity = m_current->m_block_size - now_position;
		size_t bpos = 0;

		while (size > 0)
		{
			if (now_capacity >= size)
			{
				memcpy(m_current->m_ptr + now_position, (const char*)buffer + bpos, size);
				if (m_current->m_block_size == (now_position + size))
				{
					m_current = m_current->m_next;
				}
				m_position += size;
				bpos += size;
				size = 0;
			}
			else
			{
				memcpy(m_current->m_ptr + now_position, (const char*)buffer + bpos, now_capacity);
				m_position += now_capacity;
				bpos += now_capacity;
				size -= now_capacity;
				m_current = m_current->m_next;
				now_capacity = m_current->m_block_size;
				now_position = 0;
			}
		}

		if (m_position > m_data_size)
		{
			m_data_size = m_position;
		}
	}

	void ByteArray::read(void* buffer, size_t size)
	{
		if (size > getRead_size())
		{
			throw out_of_range("not enough length");
		}

		size_t npos = m_position % m_block_size;
		size_t ncap = m_current->m_block_size - npos;
		size_t bpos = 0;
		while (size > 0)
		{
			if (ncap >= size)
			{
				memcpy((char*)buffer + bpos, m_current->m_ptr + npos, size);
				if (m_current->m_block_size == npos + size)
				{
					m_current = m_current->m_next;
				}
				m_position += size;
				bpos += size;
				size = 0;
			}
			else
			{
				memcpy((char*)buffer + bpos, m_current->m_ptr + npos, ncap);
				m_position += ncap;
				bpos += ncap;
				size -= ncap;	
				m_current = m_current->m_next;
				ncap = m_current->m_block_size;
				npos = 0;
			}
		}
	}

	void ByteArray::read(void* buffer, size_t size, size_t position)const
	{
		if (size > (m_data_size - position))
		{
			throw out_of_range("not enough length");
		}

		size_t npos = position % m_block_size;
		size_t ncap = m_current->m_block_size - npos;
		size_t bpos = 0;

		Node* current = m_current;

		while (size > 0)
		{
			if (ncap >= size)
			{
				memcpy((char*)buffer + bpos, current->m_ptr + npos, size);
				if (current->m_block_size == (npos + size))
				{
					current = current->m_next;
				}
				position += size;
				bpos += size;
				size = 0;
			}
			else
			{
				memcpy((char*)buffer + bpos, current->m_ptr + npos, ncap);
				position += ncap;
				bpos += ncap;
				size -= ncap;
				current = current->m_next;
				ncap = current->m_block_size;
				npos = 0;
			}
		}
	}

	void ByteArray::setPosition(size_t position)
	{
		if (position > m_capacity)
		{
			throw out_of_range("set_position out of range");
		}

		m_position = position;
		if (m_position > m_data_size)
		{
			m_data_size = m_position;
		}

		m_current = m_root;
		while (position > m_current->m_block_size)
		{
			position -= m_current->m_block_size;
			m_current = m_current->m_next;
		}
		if (position == m_current->m_block_size)
		{
			m_current = m_current->m_next;
		}
	}


	bool ByteArray::writeToFile(const string& name)const
	{
		ofstream fout;
		fout.open(name, std::ios::trunc | std::ios::binary);
		if (!fout.is_open())
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "writeToFile name=" << name << "error,errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		int64_t read_size = getRead_size();
		int64_t position = m_position;
		Node* current = m_current;

		while (read_size > 0)
		{
			int differ = position % m_block_size;
			int64_t length = (read_size > (int64_t)m_block_size ? m_block_size : read_size) - differ;
			fout.write(current->m_ptr + differ, length);
			current = current->m_next;
			position += length;
			read_size -= length;
		}

		return true;
	}

	bool ByteArray::readFromFile(const string& name)
	{
		ifstream fin;
		fin.open(name, std::ios::binary);
		if (!fin.is_open())
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "readFromFile name=" << name << "error,errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		shared_ptr<char> buffer(new char[m_block_size], [](char* ptr) { delete[]ptr; });
		while (!fin.eof())
		{
			fin.read(buffer.get(), m_block_size);
			write(buffer.get(), fin.gcount());
		}
		return true;
	}


	string ByteArray::toString() const
	{
		string str;
		str.resize(getRead_size());
		if (str.empty())
		{
			return str;
		}
		read(&str[0], str.size(), m_position);
		return str;
	}
	string ByteArray::toHexString() const
	{
		string str = toString();
		stringstream ss;

		for (size_t i = 0; i < str.size(); ++i)
		{
			if (i > 0 && i % 32 == 0)
			{
				ss << endl;
			}
			ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i] << " ";
		}
		return ss.str();
	}


	int64_t ByteArray::getReadBuffers(vector<iovec>& buffers, uint64_t length)
	{
		length = length > getRead_size() ? getRead_size() : length;
		if (length == 0)
		{
			return 0;
		}

		uint64_t size = length;

		size_t npos = m_position % m_block_size;
		size_t ncap = m_current->m_block_size - npos;
		iovec iov;
		Node* current = m_current;

		while (length > 0)
		{
			if (ncap >= length)
			{
				iov.iov_base = current->m_ptr + npos;
				iov.iov_len = length;
				length = 0;
			}
			else
			{
				iov.iov_base = current->m_ptr + npos;
				iov.iov_len = ncap;
				length -= ncap;
				current = current->m_next;
				ncap = current->m_block_size;
				npos = 0;
			}
			buffers.push_back(iov);
		}
		return size;
	}
	int64_t ByteArray::getReadBuffers(vector<iovec>& buffers, uint64_t length, uint64_t position)const
	{
		length = length > getRead_size() ? getRead_size() : length;
		if (length == 0)
		{
			return 0;
		}

		uint64_t size = length;

		size_t npos = position % m_block_size;

		size_t count = position / m_block_size;
		Node* current = m_root;
		while (count > 0)
		{
			current = current->m_next;
			--count;
		}

		size_t ncap = current->m_block_size - npos;
		iovec iov;

		while (length > 0)
		{
			if (ncap >= length)
			{
				iov.iov_base = current->m_ptr + npos;
				iov.iov_len = length;
				length = 0;
			}
			else
			{
				iov.iov_base = current->m_ptr + npos;
				iov.iov_len = ncap;
				length -= ncap;
				current = current->m_next;
				ncap = current->m_block_size;
				npos = 0;
			}
			buffers.push_back(iov);
		}

		return size;
	}
	int64_t ByteArray::getWriteBuffers(vector<iovec>& buffers, uint64_t length)
	{
		if (length == 0)
		{
			return 0;
		}
		expendCapacity(length);
		uint64_t size = length;

		size_t npos = m_position % m_block_size;
		size_t ncap = m_current->m_block_size - npos;
		iovec iov;
		Node* current = m_current;
		while (length > 0)
		{
			if (ncap >= length)
			{
				iov.iov_base = current->m_ptr + npos;
				iov.iov_len = length;
				length = 0;
			}
			else
			{
				iov.iov_base = current->m_ptr + npos;
				iov.iov_len = ncap;

				length -= ncap;
				current = current->m_next;
				ncap = current->m_block_size;
				npos = 0;
			}
			buffers.push_back(iov);
		}
		return size;
	}


	void ByteArray::expendCapacity(const size_t capacity_required)
	{
		/*if (capacity_required == 0)
		{
			return;
		}*/

		//如果当前可写入容量充足，则无需扩充，直接返回
		size_t available_capacity = getAvailable_capacity();
		if (available_capacity >= capacity_required)
		{
			return;
		}
		
		//否则获取所需容量与当前可写入容量的差值
		size_t differ_capacity = capacity_required - available_capacity;

		//需要增加的内存块数量（容量差值除以每个内存块的大小，向上取整）
		size_t block_count = (differ_capacity / m_block_size) + ((differ_capacity % m_block_size == 0) ? 0 : 1);

		//从根节点开始，直到查找到最后一个节点为止
		Node* tempnode = m_root;
		while (tempnode->m_next)
		{
			tempnode = tempnode->m_next;
		}


		//新增的第一个存储节点
		Node* first_newnode = NULL;

		//新建对应数量的存储节点
		for (size_t i = 0; i < block_count; ++i)
		{
			tempnode->m_next = new Node(m_block_size);

			//设置新增的第一个存储节点
			if (first_newnode == NULL)
			{
				first_newnode = tempnode->m_next;
			}

			tempnode = tempnode->m_next;
			//每新建一个存储节点，总容量增加一个内存块的大小
			m_capacity += m_block_size;
		}

		//如果原先可用容量为0，则将新增的第一个节点设作当前操作节点（原本应该为NULL）
		if (available_capacity == 0)
		{
			m_current = first_newnode;
		}
	}
}

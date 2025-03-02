#include "io/bytearray.h"
#include <iomanip>
#include <cstring>
#include <fstream>
#include <sstream>

#include "common/log.h"
#include "common/singleton.h"
#include "common/utility.h"

namespace ByteArraySpace
{
	using namespace LogSpace;
	using namespace SingletonSpace;
	using namespace UtilitySpace;
	using std::ifstream;
	using std::ofstream;
	using std::out_of_range;

	// class Node:public
	ByteArray::Node::Node(const size_t block_size) : m_ptr(new char[block_size]), m_next(nullptr), m_block_size(block_size) {}
	ByteArray::Node::Node() : m_ptr(nullptr), m_next(nullptr), m_block_size(0) {}
	ByteArray::Node::~Node()
	{
		if (m_ptr)
		{
			delete[] m_ptr;
		}
	}

	// class ByteArray:public
	ByteArray::ByteArray(const size_t block_size) : m_block_size(block_size), m_capacity(block_size),
													m_data_size(0), m_position(0), m_root(new Node(block_size)), m_current(m_root) {}
	ByteArray::~ByteArray()
	{
		// 从根节点开始依次释放所有节点的内存
		Node *temp = m_root;
		while (temp)
		{
			m_current = temp;
			temp = temp->m_next;
			delete m_current;
		}
	}

	// write
	// 写入固定长度的对应类型的数据
	void ByteArray::writeInt8(const int8_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeUint8(const uint8_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeInt16(const int16_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeUint16(const uint16_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeInt32(const int32_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeUint32(const uint32_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeInt64(const int64_t value)
	{
		write(&value, sizeof(value));
	}
	void ByteArray::writeUint64(const uint64_t value)
	{
		write(&value, sizeof(value));
	}

	// 写入可变长度（压缩）的对应类型的数据
	void ByteArray::writeInt32_compressed(const int32_t value)
	{
		writeUint32_compressed(EncodeZigzag32(value));
	}
	void ByteArray::writeUint32_compressed(uint32_t value)
	{
		uint8_t buffer[5]; // 由于继续位的存在，每次右移7位，故应该保证缓冲区大小乘以七大于等于数据位数（5*7>32）
		uint8_t i = 0;

		// 当value的值大于或等于0x80，即第一个字节的第八位（继续位）为1时，为压缩后的字节设置继续位
		while (value >= 0x80)
		{
			// 将value的最低7位提取出来，并与0x80进行或操作，设置最高位为1，表示这个字节之后还有更多字节
			buffer[i++] = (value & 0x7F) | 0x80;
			// 右移七位
			value >>= 7;
		}

		// 最后一个字节不需要设置继续位，直接写入缓冲区
		buffer[i++] = value;

		// 将缓冲区的内容写入字节序列
		write(buffer, i);
	}
	void ByteArray::writeInt64_compressed(const int64_t value)
	{
		writeUint64_compressed(EncodeZigzag64(value));
	}
	void ByteArray::writeUint64_compressed(uint64_t value)
	{
		uint8_t buffer[10]; // 由于继续位的存在，每次右移7位，故应该保证数组大小乘以七大于等于数据位数（10*7>64）
		uint8_t i = 0;
		// 当value的值大于或等于0x80，即第八位为1时
		while (value >= 0x80)
		{
			// 将value的最低7位提取出来，并与0x80进行或操作，设置最高位为1，表示这个字节之后还有更多字节
			buffer[i++] = (value & 0x7F) | 0x80;
			// 右移七位
			value >>= 7;
		}

		// 最后一个字节不需要设置继续位，直接写入缓冲区
		buffer[i++] = value;

		// 将缓冲区的内容写入字节序列
		write(buffer, i);
	}

	// 写入float类型的数据
	void ByteArray::writeFloat(const float value)
	{
		write(&value, sizeof(value));
	}
	// 写入double类型的数据
	void ByteArray::writeDouble(const double value)
	{
		write(&value, sizeof(value));
	}

	// 读取string类型的数据，用uint16_t作为长度类型
	void ByteArray::writeString16(const string &value)
	{
		writeUint16(value.size());
		write(value.c_str(), value.size());
	}
	// 读取string类型的数据，用uint32_t作为长度类型
	void ByteArray::writeString32(const string &value)
	{
		writeUint32(value.size());
		write(value.c_str(), value.size());
	}
	// 读取string类型的数据，用uint64_t作为长度类型
	void ByteArray::writeString64(const string &value)
	{
		writeUint64(value.size());
		write(value.c_str(), value.size());
	}
	// 读取string类型的数据，用可变长度的uint64_t作为长度类型
	void ByteArray::writeString64_compressed(const string &value)
	{
		writeUint64_compressed(value.size());
		write(value.c_str(), value.size());
	}
	// 写入string类型的数据，不附带长度
	void ByteArray::writeStringWithoutLength(const string &value)
	{
		write(value.c_str(), value.size());
	}

	// read
	// 读取固定长度的对应类型的数据
	int8_t ByteArray::readInt8()
	{
		int8_t value;
		read(&value, sizeof(value));
		return value;
	}
	uint8_t ByteArray::readUint8()
	{
		uint8_t value;
		read(&value, sizeof(value));
		return value;
	}
	int16_t ByteArray::readInt16()
	{
		int16_t value;
		read(&value, sizeof(value));
		return value;
	}
	uint16_t ByteArray::readUint16()
	{
		uint16_t value;
		read(&value, sizeof(value));
		return value;
	}
	int32_t ByteArray::readInt32()
	{
		int32_t value;
		read(&value, sizeof(value));
		return value;
	}
	uint32_t ByteArray::readUint32()
	{
		uint32_t value;
		read(&value, sizeof(value));
		return value;
	}
	int64_t ByteArray::readInt64()
	{
		int64_t value;
		read(&value, sizeof(value));
		return value;
	}
	uint64_t ByteArray::readUint64()
	{
		uint64_t value;
		read(&value, sizeof(value));
		return value;
	}

	// 读取可变长度（压缩）的对应类型的数据
	int32_t ByteArray::readInt32_compressed()
	{
		return DecodeZigzag32(readUint32_compressed());
	}
	uint32_t ByteArray::readUint32_compressed()
	{
		uint32_t result = 0;
		for (int i = 0; i < 32; i += 7)
		{
			// 从字节序列中读取一个字节
			uint8_t byte = readUint8();
			// 当byte的值小于0x80，即第八位（继续位）为0时，说明这是最后一个字节
			if (byte < 0x80)
			{
				// 将byte左移i位后和result取或
				result |= ((uint32_t)byte) << i;
				break;
			}
			else
			{
				// 将byte的最低七位左移i位后和result取或
				result |= ((uint32_t)(byte & 0x7F)) << i;
			}
		}
		return result;
	}
	int64_t ByteArray::readInt64_compressed()
	{
		return DecodeZigzag64(readUint64_compressed());
	}
	uint64_t ByteArray::readUint64_compressed()
	{
		uint64_t result = 0;
		for (int i = 0; i < 64; i += 7)
		{
			// 从字节序列中读取一个字节
			uint8_t b = readUint8();
			// 当byte的值小于0x80，即第八位（继续位）为0时，说明这是最后一个字节
			if (b < 0x80)
			{
				// 将byte左移i位后和result取或
				result |= ((uint64_t)b) << i;
				break;
			}
			else
			{
				// 将byte的最低七位左移i位后和result取或
				result |= ((uint64_t)(b & 0x7F)) << i;
			}
		}
		return result;
	}

	// 读取float类型的数据
	float ByteArray::readFloat()
	{
		// 先读取固定长度的32位数据，再将其内容复制到float变量中
		uint32_t value = readUint32();
		float float_value;
		memcpy(&float_value, &value, sizeof(value));
		return float_value;
	}
	// 读取double类型的数据
	double ByteArray::readDouble()
	{
		// 先读取固定长度的64位数据，再将其内容复制到double变量中
		uint64_t value = readUint64();
		float float_value;
		memcpy(&float_value, &value, sizeof(value));
		return float_value;
	}

	// 读取string类型的数据，用固定长度的uint16_t作为长度类型
	string ByteArray::readString16()
	{
		uint16_t length = readUint16();
		string buffer;
		buffer.resize(length);
		read(&buffer[0], length);
		return buffer;
	}
	// 读取string类型的数据，用固定长度的uint32_t作为长度类型
	string ByteArray::readString32()
	{
		uint32_t length = readUint32();
		string buffer;
		buffer.resize(length);
		read(&buffer[0], length);
		return buffer;
	}
	// 读取string类型的数据，用固定长度的uint64_t作为长度类型
	string ByteArray::readString64()
	{
		uint64_t length = readUint64();
		string buffer;
		buffer.resize(length);

		read(&buffer[0], length);
		return buffer;
	}
	// 写入string类型的数据，用可变长度的uint64_t作为长度类型
	string ByteArray::readString64_compressed()
	{
		uint64_t length = readUint64_compressed();
		string buffer;
		buffer.resize(length);

		read(&buffer[0], length);
		return buffer;
	}

	// 写入指定长度的数据（同时移动当前操作位置）
	void ByteArray::write(const void *buffer, size_t write_size)
	{
		// 扩容ByteArray使其可写入容量至少为指定值(如果原本就足以容纳,则不扩容)
		expendCapacity(write_size);

		// 相对于当前内存块的位置
		size_t position_in_block = m_position % m_block_size;
		// 当前内存块还剩余的容量（单位：字节）
		size_t capacity_in_block = m_block_size - position_in_block;
		// 缓冲区当前操作位置
		size_t buffer_position = 0;

		// 写入循环
		while (write_size > 0)
		{
			// 如果当前内存块剩余的容量足以写入（不包含恰好写满）
			if (capacity_in_block > write_size)
			{
				// 尽数写入所有的内容
				memcpy(m_current->m_ptr + position_in_block, (const char *)buffer + buffer_position, write_size);

				// 移动当前位置
				m_position += write_size;
				// 移动缓冲区位置
				buffer_position += write_size;
				// 已全部写入，将还需要写入的大小设为0
				write_size = 0;
			}
			// 如果不足以写入或恰能写满
			else
			{
				// 只将当前剩余容量填满
				memcpy(m_current->m_ptr + position_in_block, (const char *)buffer + buffer_position, capacity_in_block);

				// 移动当前位置
				m_position += capacity_in_block;
				// 移动缓冲区位置
				buffer_position += capacity_in_block;
				// 更新还需要写入的大小
				write_size -= capacity_in_block;

				// 当前内存块已被填满，移动当前节点
				m_current = m_current->m_next;
				// 重置当前内存块还剩余的容量
				capacity_in_block = m_block_size;
				// 重置相对于当前内存块的位置
				position_in_block = 0;
			}
		}

		// 将已保存数据大小与当前操作位置同步
		if (m_position > m_data_size)
		{
			m_data_size = m_position;
		}
	}

	// 读取指定长度的数据（同时移动当前操作位置）
	void ByteArray::read(void *buffer, size_t read_size)
	{
		// 如果要读取的数据大小超过可读取的值，抛出out_of_range异常
		if (read_size > getRead_size())
		{
			throw out_of_range("not enough length");
		}

		// 相对于当前内存块的位置
		size_t position_in_block = m_position % m_block_size;
		// 当前内存块还剩余的容量（单位：字节）
		size_t capacity_in_block = m_block_size - position_in_block;
		// 缓冲区当前操作位置
		size_t buffer_position = 0;

		while (read_size > 0)
		{
			// 如果无法读取完当前内存块剩余的内容（不包括恰好读完）
			if (capacity_in_block > read_size)
			{
				// 读取对应大小的数据
				memcpy((char *)buffer + buffer_position, m_current->m_ptr + position_in_block, read_size);

				// 移动当前位置
				m_position += read_size;
				// 移动缓冲区位置
				buffer_position += read_size;
				// 已全部读取，将还需要读取的大小设为0
				read_size = 0;
			}
			// 如果能够读取完（包括恰好读完）
			else
			{
				// 尽数读取所有的内容
				memcpy((char *)buffer + buffer_position, m_current->m_ptr + position_in_block, capacity_in_block);

				// 移动当前位置
				m_position += capacity_in_block;
				// 移动缓冲区位置
				buffer_position += capacity_in_block;
				// 设置还需要读取的大小
				read_size -= capacity_in_block;

				// 当前内存块已被读完，移动当前节点
				m_current = m_current->m_next;
				// 重置当前内存块还剩余的容量
				capacity_in_block = m_block_size;
				// 重置相对于当前内存块的位置
				position_in_block = 0;
			}
		}
	}

	// 从指定位置开始读取指定长度的数据（不移动当前操作位置）
	void ByteArray::read_without_moving(void *buffer, size_t read_size, size_t position) const
	{
		// 如果要读取的数据大小超过可读取的值，抛出out_of_range异常
		if (read_size > (m_data_size - position))
		{
			throw out_of_range("not enough length");
		}

		// 相对于当前内存块的位置
		size_t position_in_block = position % m_block_size;
		// 当前内存块还剩余的容量（单位：字节）
		// size_t capacity_in_block = m_current->m_block_size - position_in_block;
		size_t capacity_in_block = m_block_size - position_in_block;
		// 缓冲区当前操作位置
		size_t buffer_position = 0;

		Node *current = m_current;

		// 读取循环
		while (read_size > 0)
		{
			// 如果无法读取完当前内存块剩余的内容（不包括恰好读完）
			if (capacity_in_block >= read_size)
			{
				// 读取对应大小的数据
				memcpy((char *)buffer + buffer_position, current->m_ptr + position_in_block, read_size);

				// 移动当前位置
				position += read_size;
				// 移动缓冲区位置
				buffer_position += read_size;
				// 已全部读取，将还需要读取的大小设为0
				read_size = 0;
			}
			// 如果能够读取完（包括恰好读完）
			else
			{
				// 尽数读取所有的内容
				memcpy((char *)buffer + buffer_position, current->m_ptr + position_in_block, capacity_in_block);

				// 移动当前位置
				position += capacity_in_block;
				// 移动缓冲区位置
				buffer_position += capacity_in_block;
				// 设置还需要读取的大小
				read_size -= capacity_in_block;

				// 当前内存块已被读完，移动当前节点
				current = current->m_next;
				// 重置当前内存块还剩余的容量
				capacity_in_block = m_block_size;
				// 重置相对于当前内存块的位置
				position_in_block = 0;
			}
		}
	}

	// 把ByteArray的数据写入到文件中
	bool ByteArray::writeToFile(const string &filename) const
	{
		// 以截断和二进制模式打开文件
		ofstream fout(filename, std::ios::trunc | std::ios::binary);

		// 如果文件打开失败，报错并返回false
		if (!fout.is_open())
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "writeToFile name=" << filename << "error,errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}

		// 剩余可读取的数据大小（将当前可读的所有数据尽数写入文件）
		int64_t read_size = getRead_size();
		int64_t position = m_position;
		Node *current = m_current;

		while (read_size > 0)
		{
			// 相对于当前内存块的位置
			int position_in_block = position % m_block_size;

			// 如果还需要读取的数据大小小于当前内存块还剩余的容量，尽数读取；否则读取当前内存块剩余的内容（理论上从读取第二个内存块开始就会从内存块的开头开始读取）
			int64_t write_length = (read_size < (m_block_size - position_in_block) ? read_size : (m_block_size - position_in_block)); // 疑似有误

			// 将数据写入文件
			fout.write(current->m_ptr + position_in_block, write_length);

			// 更新当前操作节点
			current = current->m_next;
			// 移动当前操作位置
			position += write_length;
			// 更新剩余可读取的数据大小
			read_size -= write_length;
		}

		return true;
	}

	// 从文件中读取数据
	bool ByteArray::readFromFile(const string &filename)
	{
		// 以二进制模式打开文件
		ifstream fin(filename, std::ios::binary);

		// 如果文件打开失败，报错并返回false
		if (!fin.is_open())
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "readFromFile name=" << filename << "error,errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}

		shared_ptr<char> buffer(new char[m_block_size], [](char *ptr)
								{ delete[] ptr; });
		// 在读取到文件末尾前，不断按内存块大小依次读取文件内容并写入字节序列
		while (!fin.eof())
		{
			fin.read(buffer.get(), m_block_size);
			write(buffer.get(), fin.gcount());
		}
		return true;
	}

	// 获取可读取的缓存,保存成iovec数组,从position位置开始（不修改position）
	uint64_t ByteArray::getReadBuffers(vector<iovec> &buffers, const uint64_t read_size, uint64_t position) const
	{
		// 如果要读取的数据大小超过可读取的值，不进行读取，返回0
		if (read_size > getRead_size())
		{
			return 0;
		}

		// 还需要读取的数据大小
		uint64_t size_to_read = read_size;

		Node *current = m_root;
		// 如果起始位置为当前操作位置，直接获取当前操作节点
		if (position == m_position)
		{
			current = m_current;
		}
		// 否则根据起始位置设置节点
		else
		{
			size_t count = position / m_block_size;
			while (count > 0)
			{
				current = current->m_next;
				--count;
			}
		}

		// 相对于当前内存块的位置
		size_t position_in_block = position % m_block_size;
		// 当前内存块还剩余的容量（单位：字节）
		size_t capacity_in_block = m_block_size - position_in_block;
		// 临时的io向量
		iovec iov;

		// 读取循环
		while (size_to_read > 0)
		{
			// 如果无法读取完当前内存块剩余的内容
			if (capacity_in_block >= size_to_read)
			{
				// 读取对应大小的数据
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = size_to_read;

				// 已全部读取，清空还需要读取的数据大小
				size_to_read = 0;
			}
			// 如果能够读取完
			else
			{
				// 尽数读取所有的内容
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = capacity_in_block;

				// 设置还需要读取的数据大小
				size_to_read -= capacity_in_block;

				// 当前内存块已被读完，移动当前节点
				current = current->m_next;
				// 重置当前内存块还剩余的容量
				capacity_in_block = current->m_block_size;
				// 重置相对于当前内存块的位置
				position_in_block = 0;
			}
			buffers.push_back(iov);
		}

		// 返回读取到的数据大小
		return read_size;
	}

	// 获取可写入的缓存,保存成iovec数组（不修改position，可能扩容）
	uint64_t ByteArray::getWriteBuffers(vector<iovec> &buffers, const uint64_t write_size)
	{
		// 扩容ByteArray使其可写入容量至少为指定值(如果原本就足以容纳,则不扩容)
		expendCapacity(write_size);

		// 还需要写入的数据大小
		uint64_t size_to_write = write_size;

		// 相对于当前内存块的位置
		size_t position_in_block = m_position % m_block_size;
		// 当前内存块还剩余的容量（单位：字节）
		size_t capacity_in_block = m_current->m_block_size - position_in_block;

		// 临时的io向量
		iovec iov;
		Node *current = m_current;

		// 写入循环
		while (size_to_write > 0)
		{
			// 如果当前内存块剩余的容量足以写入
			if (capacity_in_block >= size_to_write)
			{
				// 尽数写入所有的内容
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = size_to_write;

				// 已全部写入，清空还需要写入的数据大小
				size_to_write = 0;
			}
			// 如果不足以写入
			else
			{
				// 只将当前剩余容量填满
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = capacity_in_block;

				// 设置还需要写入的数据大小
				size_to_write -= capacity_in_block;

				// 当前内存块已被填满，移动当前节点
				current = current->m_next;
				// 重置当前内存块还剩余的容量
				capacity_in_block = current->m_block_size;
				// 重置相对于当前内存块的位置
				position_in_block = 0;
			}
			buffers.push_back(iov);
		}

		// 返回成功写入的数据大小
		return write_size;
	}

	// 设置ByteArray当前位置（同时改变当前操作节点位置）
	void ByteArray::setPosition(size_t position)
	{
		// 如果要设置的位置超出总容量，抛出out_of_range异常
		if (position > m_capacity)
		{
			throw out_of_range("set_position out of range");
		}

		// 设置当前操作位置
		m_position = position;

		// 将已保存数据大小与当前操作位置同步
		if (m_position > m_data_size)
		{
			m_data_size = m_position;
		}

		// 先将当前节点设回根节点
		m_current = m_root;
		// 根据当前位置设置当前节点
		while (position >= m_current->m_block_size)
		{
			position -= m_current->m_block_size;
			m_current = m_current->m_next;
		}
	}

	// 将ByteArray里面的数据[m_position, m_size)转成string对象
	string ByteArray::toString() const
	{
		string str;
		// 将string大小设置为可读取数据大小
		str.resize(getRead_size());

		// 如果可读取的数据大小不为0，则从当前位置开始读取（否则直接返回）
		if (!str.empty())
		{
			read_without_moving(&str[0], str.size(), m_position);
		}
		return str;
	}

	// 将ByteArray里面的数据[m_position, m_size)转成16进制的string对象
	string ByteArray::toHexString() const
	{
		string str = toString();
		stringstream ss;

		for (size_t i = 0; i < str.size(); ++i)
		{
			// 每输出32个字节换一次行
			if (i > 0 && i % 32 == 0)
			{
				ss << endl;
			}
			ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i] << " ";
		}
		return ss.str();
	}

	// 清空ByteArray
	void ByteArray::clear()
	{
		m_position = 0;
		m_data_size = 0;
		m_capacity = m_block_size;

		// 释放除根节点以外的所有节点
		Node *temp = m_root->m_next;
		while (temp)
		{
			m_current = temp;
			temp = temp->m_next;
			delete m_current;
		}
		m_current = m_root;
		m_root->m_next = NULL;
	}

	// class ByteArray:private
	// 扩容ByteArray使其可写入容量至少为指定值(如果原本就足以容纳,则不扩容)
	void ByteArray::expendCapacity(const size_t capacity_required)
	{
		// 如果当前可写入容量充足，则无需扩充，直接返回
		size_t available_capacity = getAvailable_capacity();
		if (available_capacity >= capacity_required)
		{
			return;
		}

		// 否则获取所需容量与当前可写入容量的差值
		size_t differ_capacity = capacity_required - available_capacity;

		// 需要增加的内存块数量（容量差值除以每个内存块的大小，向上取整）
		size_t block_count = (differ_capacity / m_block_size) + ((differ_capacity % m_block_size == 0) ? 0 : 1);

		// 从根节点开始，直到查找到最后一个节点为止
		Node *tempnode = m_root;
		while (tempnode->m_next)
		{
			tempnode = tempnode->m_next;
		}

		////新增的第一个存储节点
		// Node* first_newnode = NULL;

		////新建对应数量的存储节点
		// for (size_t i = 0; i < block_count; ++i)
		//{
		//	tempnode->m_next = new Node(m_block_size);

		//	//设置新增的第一个存储节点
		//	if (first_newnode == NULL)
		//	{
		//		first_newnode = tempnode->m_next;
		//	}

		//	tempnode = tempnode->m_next;
		//	//每新建一个存储节点，总容量增加一个内存块的大小
		//	m_capacity += m_block_size;
		//}

		////如果原先可用容量为0，则将新增的第一个节点设作当前操作节点（原本应该为NULL）
		// if (available_capacity == 0)
		//{
		//	m_current = first_newnode;
		// }

		// 新建对应数量的存储节点
		for (size_t i = 0; i < block_count; ++i)
		{
			tempnode->m_next = new Node(m_block_size);
			tempnode = tempnode->m_next;

			// 如果原先可用容量为0，则将新增的第一个节点设作当前操作节点
			if (i == 0 && available_capacity == 0)
			{
				m_current = tempnode;
			}

			// 每新建一个存储节点，总容量增加一个内存块的大小
			m_capacity += m_block_size;
		}
	}

	// ZigZag编码在许多情况下可以减小数据的大小，特别是在处理大量绝对值小的负数或正数时，但在特定情况下（负数较少或数据的绝对值较大时），它也可能导致数据大小的增加
	// 32位zigzag编码
	uint32_t ByteArray::EncodeZigzag32(const int32_t value)
	{
		// 将负数转化为奇数，非负数转化为偶数
		if (value < 0)
		{
			return (uint32_t(-value)) * 2 - 1;
		}
		else
		{
			return (uint32_t)value * 2;
		}
	}
	// 64位zigzag编码
	uint64_t ByteArray::EncodeZigzag64(const int64_t value)
	{
		// 将负数转化为奇数，非负数转化为偶数
		if (value < 0)
		{
			return (uint64_t(-value)) * 2 - 1;
		}
		else
		{
			return (uint64_t)value * 2;
		}
	}

	// 32位zigzag解码
	int32_t ByteArray::DecodeZigzag32(const uint32_t value)
	{
		// 将奇数转化为负数，偶数转化为非负数
		return (value >> 1) ^ -(value & 1);
	}
	// 64位zigzag解码
	int64_t ByteArray::DecodeZigzag64(const uint64_t value)
	{
		// 将奇数转化为负数，偶数转化为非负数
		return (value >> 1) ^ -(value & 1);
	}
}
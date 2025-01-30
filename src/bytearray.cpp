#include "bytearray.h"
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
		// �Ӹ��ڵ㿪ʼ�����ͷ����нڵ���ڴ�
		Node *temp = m_root;
		while (temp)
		{
			m_current = temp;
			temp = temp->m_next;
			delete m_current;
		}
	}

	// write
	// д��̶����ȵĶ�Ӧ���͵�����
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

	// д��ɱ䳤�ȣ�ѹ�����Ķ�Ӧ���͵�����
	void ByteArray::writeInt32_compressed(const int32_t value)
	{
		writeUint32_compressed(EncodeZigzag32(value));
	}
	void ByteArray::writeUint32_compressed(uint32_t value)
	{
		uint8_t buffer[5]; // ���ڼ���λ�Ĵ��ڣ�ÿ������7λ����Ӧ�ñ�֤��������С�����ߴ��ڵ�������λ����5*7>32��
		uint8_t i = 0;

		// ��value��ֵ���ڻ����0x80������һ���ֽڵĵڰ�λ������λ��Ϊ1ʱ��Ϊѹ������ֽ����ü���λ
		while (value >= 0x80)
		{
			// ��value�����7λ��ȡ����������0x80���л�������������λΪ1����ʾ����ֽ�֮���и����ֽ�
			buffer[i++] = (value & 0x7F) | 0x80;
			// ������λ
			value >>= 7;
		}

		// ���һ���ֽڲ���Ҫ���ü���λ��ֱ��д�뻺����
		buffer[i++] = value;

		// ��������������д���ֽ�����
		write(buffer, i);
	}
	void ByteArray::writeInt64_compressed(const int64_t value)
	{
		writeUint64_compressed(EncodeZigzag64(value));
	}
	void ByteArray::writeUint64_compressed(uint64_t value)
	{
		uint8_t buffer[10]; // ���ڼ���λ�Ĵ��ڣ�ÿ������7λ����Ӧ�ñ�֤�����С�����ߴ��ڵ�������λ����10*7>64��
		uint8_t i = 0;
		// ��value��ֵ���ڻ����0x80�����ڰ�λΪ1ʱ
		while (value >= 0x80)
		{
			// ��value�����7λ��ȡ����������0x80���л�������������λΪ1����ʾ����ֽ�֮���и����ֽ�
			buffer[i++] = (value & 0x7F) | 0x80;
			// ������λ
			value >>= 7;
		}

		// ���һ���ֽڲ���Ҫ���ü���λ��ֱ��д�뻺����
		buffer[i++] = value;

		// ��������������д���ֽ�����
		write(buffer, i);
	}

	// д��float���͵�����
	void ByteArray::writeFloat(const float value)
	{
		write(&value, sizeof(value));
	}
	// д��double���͵�����
	void ByteArray::writeDouble(const double value)
	{
		write(&value, sizeof(value));
	}

	// ��ȡstring���͵����ݣ���uint16_t��Ϊ��������
	void ByteArray::writeString16(const string &value)
	{
		writeUint16(value.size());
		write(value.c_str(), value.size());
	}
	// ��ȡstring���͵����ݣ���uint32_t��Ϊ��������
	void ByteArray::writeString32(const string &value)
	{
		writeUint32(value.size());
		write(value.c_str(), value.size());
	}
	// ��ȡstring���͵����ݣ���uint64_t��Ϊ��������
	void ByteArray::writeString64(const string &value)
	{
		writeUint64(value.size());
		write(value.c_str(), value.size());
	}
	// ��ȡstring���͵����ݣ��ÿɱ䳤�ȵ�uint64_t��Ϊ��������
	void ByteArray::writeString64_compressed(const string &value)
	{
		writeUint64_compressed(value.size());
		write(value.c_str(), value.size());
	}
	// д��string���͵����ݣ�����������
	void ByteArray::writeStringWithoutLength(const string &value)
	{
		write(value.c_str(), value.size());
	}

	// read
	// ��ȡ�̶����ȵĶ�Ӧ���͵�����
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

	// ��ȡ�ɱ䳤�ȣ�ѹ�����Ķ�Ӧ���͵�����
	int32_t ByteArray::readInt32_compressed()
	{
		return DecodeZigzag32(readUint32_compressed());
	}
	uint32_t ByteArray::readUint32_compressed()
	{
		uint32_t result = 0;
		for (int i = 0; i < 32; i += 7)
		{
			// ���ֽ������ж�ȡһ���ֽ�
			uint8_t byte = readUint8();
			// ��byte��ֵС��0x80�����ڰ�λ������λ��Ϊ0ʱ��˵���������һ���ֽ�
			if (byte < 0x80)
			{
				// ��byte����iλ���resultȡ��
				result |= ((uint32_t)byte) << i;
				break;
			}
			else
			{
				// ��byte�������λ����iλ���resultȡ��
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
			// ���ֽ������ж�ȡһ���ֽ�
			uint8_t b = readUint8();
			// ��byte��ֵС��0x80�����ڰ�λ������λ��Ϊ0ʱ��˵���������һ���ֽ�
			if (b < 0x80)
			{
				// ��byte����iλ���resultȡ��
				result |= ((uint64_t)b) << i;
				break;
			}
			else
			{
				// ��byte�������λ����iλ���resultȡ��
				result |= ((uint64_t)(b & 0x7F)) << i;
			}
		}
		return result;
	}

	// ��ȡfloat���͵�����
	float ByteArray::readFloat()
	{
		// �ȶ�ȡ�̶����ȵ�32λ���ݣ��ٽ������ݸ��Ƶ�float������
		uint32_t value = readUint32();
		float float_value;
		memcpy(&float_value, &value, sizeof(value));
		return float_value;
	}
	// ��ȡdouble���͵�����
	double ByteArray::readDouble()
	{
		// �ȶ�ȡ�̶����ȵ�64λ���ݣ��ٽ������ݸ��Ƶ�double������
		uint64_t value = readUint64();
		float float_value;
		memcpy(&float_value, &value, sizeof(value));
		return float_value;
	}

	// ��ȡstring���͵����ݣ��ù̶����ȵ�uint16_t��Ϊ��������
	string ByteArray::readString16()
	{
		uint16_t length = readUint16();
		string buffer;
		buffer.resize(length);
		read(&buffer[0], length);
		return buffer;
	}
	// ��ȡstring���͵����ݣ��ù̶����ȵ�uint32_t��Ϊ��������
	string ByteArray::readString32()
	{
		uint32_t length = readUint32();
		string buffer;
		buffer.resize(length);
		read(&buffer[0], length);
		return buffer;
	}
	// ��ȡstring���͵����ݣ��ù̶����ȵ�uint64_t��Ϊ��������
	string ByteArray::readString64()
	{
		uint64_t length = readUint64();
		string buffer;
		buffer.resize(length);

		read(&buffer[0], length);
		return buffer;
	}
	// д��string���͵����ݣ��ÿɱ䳤�ȵ�uint64_t��Ϊ��������
	string ByteArray::readString64_compressed()
	{
		uint64_t length = readUint64_compressed();
		string buffer;
		buffer.resize(length);

		read(&buffer[0], length);
		return buffer;
	}

	// д��ָ�����ȵ����ݣ�ͬʱ�ƶ���ǰ����λ�ã�
	void ByteArray::write(const void *buffer, size_t write_size)
	{
		// ����ByteArrayʹ���д����������Ϊָ��ֵ(���ԭ������������,������)
		expendCapacity(write_size);

		// ����ڵ�ǰ�ڴ���λ��
		size_t position_in_block = m_position % m_block_size;
		// ��ǰ�ڴ�黹ʣ�����������λ���ֽڣ�
		size_t capacity_in_block = m_block_size - position_in_block;
		// ��������ǰ����λ��
		size_t buffer_position = 0;

		// д��ѭ��
		while (write_size > 0)
		{
			// �����ǰ�ڴ��ʣ�����������д�루������ǡ��д����
			if (capacity_in_block > write_size)
			{
				// ����д�����е�����
				memcpy(m_current->m_ptr + position_in_block, (const char *)buffer + buffer_position, write_size);

				// �ƶ���ǰλ��
				m_position += write_size;
				// �ƶ�������λ��
				buffer_position += write_size;
				// ��ȫ��д�룬������Ҫд��Ĵ�С��Ϊ0
				write_size = 0;
			}
			// ���������д���ǡ��д��
			else
			{
				// ֻ����ǰʣ����������
				memcpy(m_current->m_ptr + position_in_block, (const char *)buffer + buffer_position, capacity_in_block);

				// �ƶ���ǰλ��
				m_position += capacity_in_block;
				// �ƶ�������λ��
				buffer_position += capacity_in_block;
				// ���»���Ҫд��Ĵ�С
				write_size -= capacity_in_block;

				// ��ǰ�ڴ���ѱ��������ƶ���ǰ�ڵ�
				m_current = m_current->m_next;
				// ���õ�ǰ�ڴ�黹ʣ�������
				capacity_in_block = m_block_size;
				// ��������ڵ�ǰ�ڴ���λ��
				position_in_block = 0;
			}
		}

		// ���ѱ������ݴ�С�뵱ǰ����λ��ͬ��
		if (m_position > m_data_size)
		{
			m_data_size = m_position;
		}
	}

	// ��ȡָ�����ȵ����ݣ�ͬʱ�ƶ���ǰ����λ�ã�
	void ByteArray::read(void *buffer, size_t read_size)
	{
		// ���Ҫ��ȡ�����ݴ�С�����ɶ�ȡ��ֵ���׳�out_of_range�쳣
		if (read_size > getRead_size())
		{
			throw out_of_range("not enough length");
		}

		// ����ڵ�ǰ�ڴ���λ��
		size_t position_in_block = m_position % m_block_size;
		// ��ǰ�ڴ�黹ʣ�����������λ���ֽڣ�
		size_t capacity_in_block = m_block_size - position_in_block;
		// ��������ǰ����λ��
		size_t buffer_position = 0;

		while (read_size > 0)
		{
			// ����޷���ȡ�굱ǰ�ڴ��ʣ������ݣ�������ǡ�ö��꣩
			if (capacity_in_block > read_size)
			{
				// ��ȡ��Ӧ��С������
				memcpy((char *)buffer + buffer_position, m_current->m_ptr + position_in_block, read_size);

				// �ƶ���ǰλ��
				m_position += read_size;
				// �ƶ�������λ��
				buffer_position += read_size;
				// ��ȫ����ȡ��������Ҫ��ȡ�Ĵ�С��Ϊ0
				read_size = 0;
			}
			// ����ܹ���ȡ�꣨����ǡ�ö��꣩
			else
			{
				// ������ȡ���е�����
				memcpy((char *)buffer + buffer_position, m_current->m_ptr + position_in_block, capacity_in_block);

				// �ƶ���ǰλ��
				m_position += capacity_in_block;
				// �ƶ�������λ��
				buffer_position += capacity_in_block;
				// ���û���Ҫ��ȡ�Ĵ�С
				read_size -= capacity_in_block;

				// ��ǰ�ڴ���ѱ����꣬�ƶ���ǰ�ڵ�
				m_current = m_current->m_next;
				// ���õ�ǰ�ڴ�黹ʣ�������
				capacity_in_block = m_block_size;
				// ��������ڵ�ǰ�ڴ���λ��
				position_in_block = 0;
			}
		}
	}

	// ��ָ��λ�ÿ�ʼ��ȡָ�����ȵ����ݣ����ƶ���ǰ����λ�ã�
	void ByteArray::read_without_moving(void *buffer, size_t read_size, size_t position) const
	{
		// ���Ҫ��ȡ�����ݴ�С�����ɶ�ȡ��ֵ���׳�out_of_range�쳣
		if (read_size > (m_data_size - position))
		{
			throw out_of_range("not enough length");
		}

		// ����ڵ�ǰ�ڴ���λ��
		size_t position_in_block = position % m_block_size;
		// ��ǰ�ڴ�黹ʣ�����������λ���ֽڣ�
		// size_t capacity_in_block = m_current->m_block_size - position_in_block;
		size_t capacity_in_block = m_block_size - position_in_block;
		// ��������ǰ����λ��
		size_t buffer_position = 0;

		Node *current = m_current;

		// ��ȡѭ��
		while (read_size > 0)
		{
			// ����޷���ȡ�굱ǰ�ڴ��ʣ������ݣ�������ǡ�ö��꣩
			if (capacity_in_block >= read_size)
			{
				// ��ȡ��Ӧ��С������
				memcpy((char *)buffer + buffer_position, current->m_ptr + position_in_block, read_size);

				// �ƶ���ǰλ��
				position += read_size;
				// �ƶ�������λ��
				buffer_position += read_size;
				// ��ȫ����ȡ��������Ҫ��ȡ�Ĵ�С��Ϊ0
				read_size = 0;
			}
			// ����ܹ���ȡ�꣨����ǡ�ö��꣩
			else
			{
				// ������ȡ���е�����
				memcpy((char *)buffer + buffer_position, current->m_ptr + position_in_block, capacity_in_block);

				// �ƶ���ǰλ��
				position += capacity_in_block;
				// �ƶ�������λ��
				buffer_position += capacity_in_block;
				// ���û���Ҫ��ȡ�Ĵ�С
				read_size -= capacity_in_block;

				// ��ǰ�ڴ���ѱ����꣬�ƶ���ǰ�ڵ�
				current = current->m_next;
				// ���õ�ǰ�ڴ�黹ʣ�������
				capacity_in_block = m_block_size;
				// ��������ڵ�ǰ�ڴ���λ��
				position_in_block = 0;
			}
		}
	}

	// ��ByteArray������д�뵽�ļ���
	bool ByteArray::writeToFile(const string &filename) const
	{
		// �ԽضϺͶ�����ģʽ���ļ�
		ofstream fout(filename, std::ios::trunc | std::ios::binary);

		// ����ļ���ʧ�ܣ�����������false
		if (!fout.is_open())
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "writeToFile name=" << filename << "error,errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}

		// ʣ��ɶ�ȡ�����ݴ�С������ǰ�ɶ����������ݾ���д���ļ���
		int64_t read_size = getRead_size();
		int64_t position = m_position;
		Node *current = m_current;

		while (read_size > 0)
		{
			// ����ڵ�ǰ�ڴ���λ��
			int position_in_block = position % m_block_size;

			// �������Ҫ��ȡ�����ݴ�СС�ڵ�ǰ�ڴ�黹ʣ���������������ȡ�������ȡ��ǰ�ڴ��ʣ������ݣ������ϴӶ�ȡ�ڶ����ڴ�鿪ʼ�ͻ���ڴ��Ŀ�ͷ��ʼ��ȡ��
			int64_t write_length = (read_size < (m_block_size - position_in_block) ? read_size : (m_block_size - position_in_block)); // ��������

			// ������д���ļ�
			fout.write(current->m_ptr + position_in_block, write_length);

			// ���µ�ǰ�����ڵ�
			current = current->m_next;
			// �ƶ���ǰ����λ��
			position += write_length;
			// ����ʣ��ɶ�ȡ�����ݴ�С
			read_size -= write_length;
		}

		return true;
	}

	// ���ļ��ж�ȡ����
	bool ByteArray::readFromFile(const string &filename)
	{
		// �Զ�����ģʽ���ļ�
		ifstream fin(filename, std::ios::binary);

		// ����ļ���ʧ�ܣ�����������false
		if (!fin.is_open())
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__));
			log_event->getSstream() << "readFromFile name=" << filename << "error,errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::LOG_ERROR, log_event);
			return false;
		}

		shared_ptr<char> buffer(new char[m_block_size], [](char *ptr)
								{ delete[] ptr; });
		// �ڶ�ȡ���ļ�ĩβǰ�����ϰ��ڴ���С���ζ�ȡ�ļ����ݲ�д���ֽ�����
		while (!fin.eof())
		{
			fin.read(buffer.get(), m_block_size);
			write(buffer.get(), fin.gcount());
		}
		return true;
	}

	// ��ȡ�ɶ�ȡ�Ļ���,�����iovec����,��positionλ�ÿ�ʼ�����޸�position��
	uint64_t ByteArray::getReadBuffers(vector<iovec> &buffers, const uint64_t read_size, uint64_t position) const
	{
		// ���Ҫ��ȡ�����ݴ�С�����ɶ�ȡ��ֵ�������ж�ȡ������0
		if (read_size > getRead_size())
		{
			return 0;
		}

		// ����Ҫ��ȡ�����ݴ�С
		uint64_t size_to_read = read_size;

		Node *current = m_root;
		// �����ʼλ��Ϊ��ǰ����λ�ã�ֱ�ӻ�ȡ��ǰ�����ڵ�
		if (position == m_position)
		{
			current = m_current;
		}
		// ���������ʼλ�����ýڵ�
		else
		{
			size_t count = position / m_block_size;
			while (count > 0)
			{
				current = current->m_next;
				--count;
			}
		}

		// ����ڵ�ǰ�ڴ���λ��
		size_t position_in_block = position % m_block_size;
		// ��ǰ�ڴ�黹ʣ�����������λ���ֽڣ�
		size_t capacity_in_block = m_block_size - position_in_block;
		// ��ʱ��io����
		iovec iov;

		// ��ȡѭ��
		while (size_to_read > 0)
		{
			// ����޷���ȡ�굱ǰ�ڴ��ʣ�������
			if (capacity_in_block >= size_to_read)
			{
				// ��ȡ��Ӧ��С������
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = size_to_read;

				// ��ȫ����ȡ����ջ���Ҫ��ȡ�����ݴ�С
				size_to_read = 0;
			}
			// ����ܹ���ȡ��
			else
			{
				// ������ȡ���е�����
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = capacity_in_block;

				// ���û���Ҫ��ȡ�����ݴ�С
				size_to_read -= capacity_in_block;

				// ��ǰ�ڴ���ѱ����꣬�ƶ���ǰ�ڵ�
				current = current->m_next;
				// ���õ�ǰ�ڴ�黹ʣ�������
				capacity_in_block = current->m_block_size;
				// ��������ڵ�ǰ�ڴ���λ��
				position_in_block = 0;
			}
			buffers.push_back(iov);
		}

		// ���ض�ȡ�������ݴ�С
		return read_size;
	}

	// ��ȡ��д��Ļ���,�����iovec���飨���޸�position���������ݣ�
	uint64_t ByteArray::getWriteBuffers(vector<iovec> &buffers, const uint64_t write_size)
	{
		// ����ByteArrayʹ���д����������Ϊָ��ֵ(���ԭ������������,������)
		expendCapacity(write_size);

		// ����Ҫд������ݴ�С
		uint64_t size_to_write = write_size;

		// ����ڵ�ǰ�ڴ���λ��
		size_t position_in_block = m_position % m_block_size;
		// ��ǰ�ڴ�黹ʣ�����������λ���ֽڣ�
		size_t capacity_in_block = m_current->m_block_size - position_in_block;

		// ��ʱ��io����
		iovec iov;
		Node *current = m_current;

		// д��ѭ��
		while (size_to_write > 0)
		{
			// �����ǰ�ڴ��ʣ�����������д��
			if (capacity_in_block >= size_to_write)
			{
				// ����д�����е�����
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = size_to_write;

				// ��ȫ��д�룬��ջ���Ҫд������ݴ�С
				size_to_write = 0;
			}
			// ���������д��
			else
			{
				// ֻ����ǰʣ����������
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = capacity_in_block;

				// ���û���Ҫд������ݴ�С
				size_to_write -= capacity_in_block;

				// ��ǰ�ڴ���ѱ��������ƶ���ǰ�ڵ�
				current = current->m_next;
				// ���õ�ǰ�ڴ�黹ʣ�������
				capacity_in_block = current->m_block_size;
				// ��������ڵ�ǰ�ڴ���λ��
				position_in_block = 0;
			}
			buffers.push_back(iov);
		}

		// ���سɹ�д������ݴ�С
		return write_size;
	}

	// ����ByteArray��ǰλ�ã�ͬʱ�ı䵱ǰ�����ڵ�λ�ã�
	void ByteArray::setPosition(size_t position)
	{
		// ���Ҫ���õ�λ�ó������������׳�out_of_range�쳣
		if (position > m_capacity)
		{
			throw out_of_range("set_position out of range");
		}

		// ���õ�ǰ����λ��
		m_position = position;

		// ���ѱ������ݴ�С�뵱ǰ����λ��ͬ��
		if (m_position > m_data_size)
		{
			m_data_size = m_position;
		}

		// �Ƚ���ǰ�ڵ���ظ��ڵ�
		m_current = m_root;
		// ���ݵ�ǰλ�����õ�ǰ�ڵ�
		while (position >= m_current->m_block_size)
		{
			position -= m_current->m_block_size;
			m_current = m_current->m_next;
		}
	}

	// ��ByteArray���������[m_position, m_size)ת��string����
	string ByteArray::toString() const
	{
		string str;
		// ��string��С����Ϊ�ɶ�ȡ���ݴ�С
		str.resize(getRead_size());

		// ����ɶ�ȡ�����ݴ�С��Ϊ0����ӵ�ǰλ�ÿ�ʼ��ȡ������ֱ�ӷ��أ�
		if (!str.empty())
		{
			read_without_moving(&str[0], str.size(), m_position);
		}
		return str;
	}

	// ��ByteArray���������[m_position, m_size)ת��16���Ƶ�string����
	string ByteArray::toHexString() const
	{
		string str = toString();
		stringstream ss;

		for (size_t i = 0; i < str.size(); ++i)
		{
			// ÿ���32���ֽڻ�һ����
			if (i > 0 && i % 32 == 0)
			{
				ss << endl;
			}
			ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i] << " ";
		}
		return ss.str();
	}

	// ���ByteArray
	void ByteArray::clear()
	{
		m_position = 0;
		m_data_size = 0;
		m_capacity = m_block_size;

		// �ͷų����ڵ���������нڵ�
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
	// ����ByteArrayʹ���д����������Ϊָ��ֵ(���ԭ������������,������)
	void ByteArray::expendCapacity(const size_t capacity_required)
	{
		// �����ǰ��д���������㣬���������䣬ֱ�ӷ���
		size_t available_capacity = getAvailable_capacity();
		if (available_capacity >= capacity_required)
		{
			return;
		}

		// �����ȡ���������뵱ǰ��д�������Ĳ�ֵ
		size_t differ_capacity = capacity_required - available_capacity;

		// ��Ҫ���ӵ��ڴ��������������ֵ����ÿ���ڴ��Ĵ�С������ȡ����
		size_t block_count = (differ_capacity / m_block_size) + ((differ_capacity % m_block_size == 0) ? 0 : 1);

		// �Ӹ��ڵ㿪ʼ��ֱ�����ҵ����һ���ڵ�Ϊֹ
		Node *tempnode = m_root;
		while (tempnode->m_next)
		{
			tempnode = tempnode->m_next;
		}

		////�����ĵ�һ���洢�ڵ�
		// Node* first_newnode = NULL;

		////�½���Ӧ�����Ĵ洢�ڵ�
		// for (size_t i = 0; i < block_count; ++i)
		//{
		//	tempnode->m_next = new Node(m_block_size);

		//	//���������ĵ�һ���洢�ڵ�
		//	if (first_newnode == NULL)
		//	{
		//		first_newnode = tempnode->m_next;
		//	}

		//	tempnode = tempnode->m_next;
		//	//ÿ�½�һ���洢�ڵ㣬����������һ���ڴ��Ĵ�С
		//	m_capacity += m_block_size;
		//}

		////���ԭ�ȿ�������Ϊ0���������ĵ�һ���ڵ�������ǰ�����ڵ㣨ԭ��Ӧ��ΪNULL��
		// if (available_capacity == 0)
		//{
		//	m_current = first_newnode;
		// }

		// �½���Ӧ�����Ĵ洢�ڵ�
		for (size_t i = 0; i < block_count; ++i)
		{
			tempnode->m_next = new Node(m_block_size);
			tempnode = tempnode->m_next;

			// ���ԭ�ȿ�������Ϊ0���������ĵ�һ���ڵ�������ǰ�����ڵ�
			if (i == 0 && available_capacity == 0)
			{
				m_current = tempnode;
			}

			// ÿ�½�һ���洢�ڵ㣬����������һ���ڴ��Ĵ�С
			m_capacity += m_block_size;
		}
	}

	// ZigZag��������������¿��Լ�С���ݵĴ�С���ر����ڴ�����������ֵС�ĸ���������ʱ�������ض�����£��������ٻ����ݵľ���ֵ�ϴ�ʱ������Ҳ���ܵ������ݴ�С������
	// 32λzigzag����
	uint32_t ByteArray::EncodeZigzag32(const int32_t value)
	{
		// ������ת��Ϊ�������Ǹ���ת��Ϊż��
		if (value < 0)
		{
			return (uint32_t(-value)) * 2 - 1;
		}
		else
		{
			return (uint32_t)value * 2;
		}
	}
	// 64λzigzag����
	uint64_t ByteArray::EncodeZigzag64(const int64_t value)
	{
		// ������ת��Ϊ�������Ǹ���ת��Ϊż��
		if (value < 0)
		{
			return (uint64_t(-value)) * 2 - 1;
		}
		else
		{
			return (uint64_t)value * 2;
		}
	}

	// 32λzigzag����
	int32_t ByteArray::DecodeZigzag32(const uint32_t value)
	{
		// ������ת��Ϊ������ż��ת��Ϊ�Ǹ���
		return (value >> 1) ^ -(value & 1);
	}
	// 64λzigzag����
	int64_t ByteArray::DecodeZigzag64(const uint64_t value)
	{
		// ������ת��Ϊ������ż��ת��Ϊ�Ǹ���
		return (value >> 1) ^ -(value & 1);
	}
}

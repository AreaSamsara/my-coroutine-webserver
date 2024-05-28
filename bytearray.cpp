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
		//�Ӹ��ڵ㿪ʼ�����ͷ����нڵ���ڴ�
		Node* temp = m_root;
		while (temp)
		{
			m_current = temp;
			temp = temp->m_next;
			delete m_current;
		}
	}

	//write
	//д��̶����ȵĶ�Ӧ���͵�����
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

	//д��ɱ䳤�ȣ�ѹ�����Ķ�Ӧ���͵�����
	void ByteArray::writeInt32(const int32_t value)
	{
		writeUint32(EncodeZigzag32(value));
	}
	void ByteArray::writeUint32(uint32_t value)
	{
		uint8_t buffer[5];	//���ڼ���λ�Ĵ��ڣ�ÿ������7λ����Ӧ�ñ�֤��������С�����ߴ��ڵ�������λ����5*7>32��
		uint8_t i = 0;

		//��value��ֵ���ڻ����0x80������һ���ֽڵĵڰ�λ������λ��Ϊ1ʱ��Ϊѹ������ֽ����ü���λ
		while (value >= 0x80)
		{
			//��value�����7λ��ȡ����������0x80���л�������������λΪ1����ʾ����ֽ�֮���и����ֽ�
			buffer[i++] = (value & 0x7F) | 0x80;
			//������λ
			value >>= 7;
		}

		//���һ���ֽڲ���Ҫ���ü���λ��ֱ��д�뻺����
		buffer[i++] = value;

		//��������������д���ֽ�����
		write(buffer, i);
	}
	void ByteArray::writeInt64(const int64_t value)
	{
		writeUint64(EncodeZigzag64(value));
	}
	void ByteArray::writeUint64(uint64_t value)
	{
		uint8_t buffer[10];	//���ڼ���λ�Ĵ��ڣ�ÿ������7λ����Ӧ�ñ�֤�����С�����ߴ��ڵ�������λ����10*7>64��
		uint8_t i = 0;
		//��value��ֵ���ڻ����0x80�����ڰ�λΪ1ʱ
		while (value >= 0x80)
		{
			//��value�����7λ��ȡ����������0x80���л�������������λΪ1����ʾ����ֽ�֮���и����ֽ�
			buffer[i++] = (value & 0x7F) | 0x80;
			//������λ
			value >>= 7;
		}

		//���һ���ֽڲ���Ҫ���ü���λ��ֱ��д�뻺����
		buffer[i++] = value;

		//��������������д���ֽ�����
		write(buffer, i);
	}

	//д��float���͵�����
	void ByteArray::writeFloat(const float value)
	{
		write(&value, sizeof(value));
	}
	//д��double���͵�����
	void ByteArray::writeDouble(const double value)
	{
		write(&value, sizeof(value));
	}

	//��ȡstring���͵����ݣ���uint16_t��Ϊ��������
	void ByteArray::writeStringF16(const string& value)
	{
		writeFuint16(value.size());
		write(value.c_str(), value.size());
	}
	//��ȡstring���͵����ݣ���uint32_t��Ϊ��������
	void ByteArray::writeStringF32(const string& value)
	{
		writeFuint32(value.size());
		write(value.c_str(), value.size());
	}
	//��ȡstring���͵����ݣ���uint64_t��Ϊ��������
	void ByteArray::writeStringF64(const string& value)
	{
		writeFuint64(value.size());
		write(value.c_str(), value.size());
	}
	//��ȡstring���͵����ݣ��ÿɱ䳤�ȵ�uint64_t��Ϊ��������
	void ByteArray::writeStringVint(const string& value)
	{
		writeUint64(value.size());
		write(value.c_str(), value.size());
	}
	//д��string���͵����ݣ�����������
	void ByteArray::writeStringWithoutLength(const string& value)
	{
		write(value.c_str(), value.size());
	}


	//read 
	//��ȡ�̶����ȵĶ�Ӧ���͵�����
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

	//��ȡ�ɱ䳤�ȣ�ѹ�����Ķ�Ӧ���͵�����
	int32_t ByteArray::readInt32()
	{
		return DecodeZigzag32(readUint32());
	}
	uint32_t ByteArray::readUint32()
	{
		uint32_t result = 0;
		for (int i = 0; i < 32; i += 7)
		{
			//���ֽ������ж�ȡһ���ֽ�
			uint8_t byte = readFuint8();
			//��byte��ֵС��0x80�����ڰ�λ������λ��Ϊ0ʱ��˵���������һ���ֽ�
			if (byte < 0x80)
			{
				//��byte����iλ���resultȡ��
				result |= ((uint32_t)byte) << i;
				break;
			}
			else
			{
				//��byte�������λ����iλ���resultȡ��
				result |= ((uint32_t)(byte & 0x7F)) << i;
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
		for (int i = 0; i < 64; i += 7)
		{
			//���ֽ������ж�ȡһ���ֽ�
			uint8_t b = readFuint8();
			//��byte��ֵС��0x80�����ڰ�λ������λ��Ϊ0ʱ��˵���������һ���ֽ�
			if (b < 0x80)
			{
				//��byte����iλ���resultȡ��
				result |= ((uint64_t)b) << i;
				break;
			}
			else
			{
				//��byte�������λ����iλ���resultȡ��
				result |= ((uint64_t)(b & 0x7F)) << i;
			}
		}
		return result;
	}

	//��ȡfloat���͵�����
	float ByteArray::readFloat()
	{
		//�ȶ�ȡ�̶����ȵ�32λ���ݣ��ٽ������ݸ��Ƶ�float������
		uint32_t value = readFuint32();
		float float_value;
		memcpy(&float_value, &value, sizeof(value));
		return float_value;
	}
	//��ȡdouble���͵�����
	double ByteArray::readDouble()
	{
		//�ȶ�ȡ�̶����ȵ�64λ���ݣ��ٽ������ݸ��Ƶ�double������
		uint64_t value = readFuint64();
		float float_value;
		memcpy(&float_value, &value, sizeof(value));
		return float_value;
	}

	//��ȡstring���͵����ݣ��ù̶����ȵ�uint16_t��Ϊ��������
	string ByteArray::readStringF16()
	{
		uint16_t length = readFuint16();
		string buffer;
		buffer.resize(length);
		read(&buffer[0], length);
		return buffer;
	}
	//��ȡstring���͵����ݣ��ù̶����ȵ�uint32_t��Ϊ��������
	string ByteArray::readStringF32()
	{
		uint32_t length = readFuint32();
		string buffer;
		buffer.resize(length);
		read(&buffer[0], length);
		return buffer;
	}
	//��ȡstring���͵����ݣ��ù̶����ȵ�uint64_t��Ϊ��������
	string ByteArray::readStringF64()
	{
		uint64_t length = readFuint64();
		string buffer;
		buffer.resize(length);

		read(&buffer[0], length);
		return buffer;
	}
	//д��string���͵����ݣ��ÿɱ䳤�ȵ�uint64_t��Ϊ��������
	string ByteArray::readStringVint()
	{
		uint64_t length = readUint64();
		string buffer;
		buffer.resize(length);

		read(&buffer[0], length);
		return buffer;
	}



	//���ByteArray
	void ByteArray::clear()
	{
		m_position = 0;;
		m_data_size = 0;
		m_capacity = m_block_size;

		//�ͷų����ڵ���������нڵ�
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

	//д��size���ȵ����ݣ�ͬʱ�ƶ���ǰ����λ�ã�
	void ByteArray::write(const void* buffer, size_t size)
	{
		if (size == 0)
		{
			return;
		}

		//����ByteArrayʹ���д����������Ϊָ��ֵ(���ԭ������������,������)
		expendCapacity(size);

		//����ڵ�ǰ�ڴ���λ��
		size_t position_in_block = m_position % m_block_size;
		//��ǰ�ڴ�黹ʣ�����������λ���ֽڣ�
		size_t capacity_in_block = m_current->m_block_size - position_in_block;
		//��������ǰ����λ��
		size_t buffer_position = 0;

		//д��ѭ��
		while (size > 0)
		{
			//�����ǰ�ڴ��ʣ�����������д��
			if (capacity_in_block >= size)
			{
				//����д�����е�����
				memcpy(m_current->m_ptr + position_in_block, (const char*)buffer + buffer_position, size);

				//���ǡ��������ǰ�ڴ�飬�ƶ���ǰ�ڵ�
				if (m_current->m_block_size == (position_in_block + size))
				{
					m_current = m_current->m_next;
				}

				//�ƶ���ǰλ��
				m_position += size;
				//�ƶ�������λ��
				buffer_position += size;
				//��ȫ��д�룬���
				size = 0;
			}
			//���������д��
			else
			{
				//ֻ����ǰʣ����������
				memcpy(m_current->m_ptr + position_in_block, (const char*)buffer + buffer_position, capacity_in_block);

				//�ƶ���ǰλ��
				m_position += capacity_in_block;
				//�ƶ�������λ��
				buffer_position += capacity_in_block;

				size -= capacity_in_block;

				//��ǰ�ڴ���ѱ��������ƶ���ǰ�ڵ�
				m_current = m_current->m_next;

				//���õ�ǰ�ڴ�黹ʣ�������
				capacity_in_block = m_current->m_block_size;
				//��������ڵ�ǰ�ڴ���λ��
				position_in_block = 0;
			}
		}

		//���ѱ������ݴ�С�뵱ǰ����λ��ͬ��
		if (m_position > m_data_size)
		{
			m_data_size = m_position;
		}
	}

	//��ȡsize���ȵ����ݣ�ͬʱ�ƶ���ǰ����λ�ã�
	void ByteArray::read(void* buffer, size_t size)
	{
		//���Ҫ��ȡ�����ݴ�С�����ɶ�ȡ��ֵ���׳�out_of_range�쳣
		if (size > getRead_size())
		{
			throw out_of_range("not enough length");
		}

		//����ڵ�ǰ�ڴ���λ��
		size_t position_in_block = m_position % m_block_size;
		//��ǰ�ڴ�黹ʣ�����������λ���ֽڣ�
		size_t capacity_in_block = m_current->m_block_size - position_in_block;
		//��������ǰ����λ��
		size_t buffer_position = 0;

		//��ȡѭ��
		while (size > 0)
		{
			//����޷���ȡ�굱ǰ�ڴ��ʣ�������
			if (capacity_in_block >= size)
			{
				//��ȡ��Ӧ��С������
				memcpy((char*)buffer + buffer_position, m_current->m_ptr + position_in_block, size);

				//���ǡ�ö�ȡ�굱ǰ�ڴ�飬�ƶ���ǰ�ڵ�
				if (m_current->m_block_size == position_in_block + size)
				{
					m_current = m_current->m_next;
				}

				//�ƶ���ǰλ��
				m_position += size;
				//�ƶ�������λ��
				buffer_position += size;
				//��ȫ����ȡ�����
				size = 0;
			}
			//����ܹ���ȡ��
			else
			{
				//������ȡ���е�����
				memcpy((char*)buffer + buffer_position, m_current->m_ptr + position_in_block, capacity_in_block);

				//�ƶ���ǰλ��
				m_position += capacity_in_block;
				//�ƶ�������λ��
				buffer_position += capacity_in_block;

				size -= capacity_in_block;

				//��ǰ�ڴ���ѱ����꣬�ƶ���ǰ�ڵ�
				m_current = m_current->m_next;

				//���õ�ǰ�ڴ�黹ʣ�������
				capacity_in_block = m_current->m_block_size;
				//��������ڵ�ǰ�ڴ���λ��
				position_in_block = 0;
			}
		}
	}

	//��ȡsize���ȵ����ݣ����ƶ���ǰ����λ�ã�
	void ByteArray::read(void* buffer, size_t size, size_t position)const
	{
		//���Ҫ��ȡ�����ݴ�С�����ɶ�ȡ��ֵ���׳�out_of_range�쳣
		if (size > (m_data_size - position))
		{
			throw out_of_range("not enough length");
		}

		//����ڵ�ǰ�ڴ���λ��
		size_t position_in_block = position % m_block_size;
		//��ǰ�ڴ�黹ʣ�����������λ���ֽڣ�
		size_t capacity_in_block = m_current->m_block_size - position_in_block;
		//��������ǰ����λ��
		size_t buffer_position = 0;

		Node* current = m_current;

		while (size > 0)
		{
			//����޷���ȡ�굱ǰ�ڴ��ʣ�������
			if (capacity_in_block >= size)
			{
				//��ȡ��Ӧ��С������
				memcpy((char*)buffer + buffer_position, current->m_ptr + position_in_block, size);

				//���ǡ�ö�ȡ�굱ǰ�ڴ�飬�ƶ���ǰ�ڵ�
				if (current->m_block_size == (position_in_block + size))
				{
					current = current->m_next;
				}

				//�ƶ���ǰλ��
				position += size;
				//�ƶ�������λ��
				buffer_position += size;
				//��ȫ����ȡ�����
				size = 0;
			}
			//����ܹ���ȡ��
			else
			{
				//������ȡ���е�����
				memcpy((char*)buffer + buffer_position, current->m_ptr + position_in_block, capacity_in_block);
				//�ƶ���ǰλ��
				position += capacity_in_block;
				//�ƶ�������λ��
				buffer_position += capacity_in_block;

				size -= capacity_in_block;

				//��ǰ�ڴ���ѱ����꣬�ƶ���ǰ�ڵ�
				current = current->m_next;

				//���õ�ǰ�ڴ�黹ʣ�������
				capacity_in_block = current->m_block_size;
				//��������ڵ�ǰ�ڴ���λ��
				position_in_block = 0;
			}
		}
	}

	//����ByteArray��ǰλ��
	void ByteArray::setPosition(size_t position)
	{
		//���Ҫ���õ�λ�ó������������׳�out_of_range�쳣
		if (position > m_capacity)
		{
			throw out_of_range("set_position out of range");
		}

		//���õ�ǰ����λ��
		m_position = position;

		//���ѱ������ݴ�С�뵱ǰ����λ��ͬ��
		if (m_position > m_data_size)
		{
			m_data_size = m_position;
		}

		//�Ƚ���ǰ�ڵ���ظ��ڵ�
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


	//��ByteArray������д�뵽�ļ���
	bool ByteArray::writeToFile(const string& filename)const
	{
		ofstream fout;
		//�ԽضϺͶ�����ģʽ���ļ�
		fout.open(filename, std::ios::trunc | std::ios::binary);
		//����ļ���ʧ�ܣ���������false
		if (!fout.is_open())
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "writeToFile name=" << filename << "error,errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		//ʣ��ɶ�ȡ�����ݴ�С������ǰ�ɶ����������ݾ���д���ļ���
		int64_t read_size = getRead_size();
		int64_t position = m_position;
		Node* current = m_current;

		while (read_size > 0)
		{
			//����ڵ�ǰ�ڴ���λ��
			int position_in_block = position % m_block_size;

			int64_t write_length = (read_size > (int64_t)m_block_size ? m_block_size : read_size) - position_in_block;	//��������
			//������д���ļ�
			fout.write(current->m_ptr + position_in_block, write_length);

			current = current->m_next;	//��������
			//�ƶ���ǰ����λ��
			position += write_length;
			//����ʣ��ɶ�ȡ�����ݴ�С
			read_size -= write_length;
		}

		return true;
	}

	//���ļ��ж�ȡ����
	bool ByteArray::readFromFile(const string& filename)
	{
		ifstream fin;
		//�Զ�����ģʽ���ļ�
		fin.open(filename, std::ios::binary);
		//����ļ���ʧ�ܣ���������false
		if (!fin.is_open())
		{
			shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			log_event->getSstream() << "readFromFile name=" << filename << "error,errno=" << errno << " strerror=" << strerror(errno);
			Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::ERROR, log_event);
			return false;
		}

		shared_ptr<char> buffer(new char[m_block_size], [](char* ptr) { delete[]ptr; });
		//�ڶ�ȡ���ļ�ĩβǰ�����ϰ��ڴ���С���ζ�ȡ�ļ����ݲ�д���ֽ�����
		while (!fin.eof())
		{
			fin.read(buffer.get(), m_block_size);
			write(buffer.get(), fin.gcount());
		}
		return true;
	}


	//��ByteArray���������[m_position, m_size)ת��string����
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

	//��ByteArray���������[m_position, m_size)ת��16���Ƶ�string����
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

	//ֻ��ȡ���ݣ����޸�position
	//��ȡ�ɶ�ȡ�Ļ���,�����iovec����
	uint64_t ByteArray::getReadBuffers(vector<iovec>& buffers, uint64_t length)
	{
		length = length > getRead_size() ? getRead_size() : length;
		if (length == 0)
		{
			return 0;
		}

		//��ȡ�������ݴ�С
		uint64_t size = length;

		//����ڵ�ǰ�ڴ���λ��
		size_t position_in_block = m_position % m_block_size;
		//��ǰ�ڴ�黹ʣ�����������λ���ֽڣ�
		size_t capacity_in_block = m_current->m_block_size - position_in_block;

		iovec iov;
		Node* current = m_current;

		while (length > 0)
		{
			//����޷���ȡ�굱ǰ�ڴ��ʣ�������
			if (capacity_in_block >= length)
			{
				//��ȡ��Ӧ��С������
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = length;

				//��ȫ����ȡ�����
				length = 0;
			}
			//����ܹ���ȡ��
			else
			{
				//������ȡ���е�����
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = capacity_in_block;

				length -= capacity_in_block;

				//��ǰ�ڴ���ѱ����꣬�ƶ���ǰ�ڵ�
				current = current->m_next;

				//���õ�ǰ�ڴ�黹ʣ�������
				capacity_in_block = current->m_block_size;
				//��������ڵ�ǰ�ڴ���λ��
				position_in_block = 0;
			}
			buffers.push_back(iov);
		}

		//���ض�ȡ�������ݴ�С
		return size;
	}
	//��ȡ�ɶ�ȡ�Ļ���,�����iovec����,��positionλ�ÿ�ʼ
	uint64_t ByteArray::getReadBuffers(vector<iovec>& buffers, uint64_t length, uint64_t position)const
	{
		length = length > getRead_size() ? getRead_size() : length;
		if (length == 0)
		{
			return 0;
		}

		//��ȡ�������ݴ�С
		uint64_t size = length;

		//����ڵ�ǰ�ڴ���λ��
		size_t position_in_block = position % m_block_size;


		size_t count = position / m_block_size;
		Node* current = m_root;
		while (count > 0)
		{
			current = current->m_next;
			--count;
		}

		//��ǰ�ڴ�黹ʣ�����������λ���ֽڣ�
		size_t capacity_in_block = current->m_block_size - position_in_block;
		iovec iov;

		while (length > 0)
		{
			//����޷���ȡ�굱ǰ�ڴ��ʣ�������
			if (capacity_in_block >= length)
			{
				//��ȡ��Ӧ��С������
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = length;

				//��ȫ����ȡ�����
				length = 0;
			}
			//����ܹ���ȡ��
			else
			{
				//������ȡ���е�����
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = capacity_in_block;

				length -= capacity_in_block;

				//��ǰ�ڴ���ѱ����꣬�ƶ���ǰ�ڵ�
				current = current->m_next;

				//���õ�ǰ�ڴ�黹ʣ�������
				capacity_in_block = current->m_block_size;
				//��������ڵ�ǰ�ڴ���λ��
				position_in_block = 0;
			}
			buffers.push_back(iov);
		}

		//���ض�ȡ�������ݴ�С
		return size;
	}

	//�������������޸�position
	//��ȡ��д��Ļ���,�����iovec����
	uint64_t ByteArray::getWriteBuffers(vector<iovec>& buffers, uint64_t length)
	{
		if (length == 0)
		{
			return 0;
		}

		//����ByteArrayʹ���д����������Ϊָ��ֵ(���ԭ������������,������)
		expendCapacity(length);

		//�ɹ�д������ݴ�С
		uint64_t size = length;

		//����ڵ�ǰ�ڴ���λ��
		size_t position_in_block = m_position % m_block_size;
		//��ǰ�ڴ�黹ʣ�����������λ���ֽڣ�
		size_t capacity_in_block = m_current->m_block_size - position_in_block;

		iovec iov;
		Node* current = m_current;

		//д��ѭ��
		while (length > 0)
		{
			//�����ǰ�ڴ��ʣ�����������д��
			if (capacity_in_block >= length)
			{
				//����д�����е�����
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = length;

				//��ȫ��д�룬���
				length = 0;
			}
			//���������д��
			else
			{
				//ֻ����ǰʣ����������
				iov.iov_base = current->m_ptr + position_in_block;
				iov.iov_len = capacity_in_block;

				length -= capacity_in_block;

				//��ǰ�ڴ���ѱ��������ƶ���ǰ�ڵ�
				current = current->m_next;

				//���õ�ǰ�ڴ�黹ʣ�������
				capacity_in_block = current->m_block_size;
				//��������ڵ�ǰ�ڴ���λ��
				position_in_block = 0;
			}
			buffers.push_back(iov);
		}

		//���سɹ�д������ݴ�С
		return size;
	}






	//class ByteArray:private
	//����ByteArrayʹ���д����������Ϊָ��ֵ(���ԭ������������,������)
	void ByteArray::expendCapacity(const size_t capacity_required)
	{
		//�����ǰ��д���������㣬���������䣬ֱ�ӷ���
		size_t available_capacity = getAvailable_capacity();
		if (available_capacity >= capacity_required)
		{
			return;
		}
		
		//�����ȡ���������뵱ǰ��д�������Ĳ�ֵ
		size_t differ_capacity = capacity_required - available_capacity;

		//��Ҫ���ӵ��ڴ��������������ֵ����ÿ���ڴ��Ĵ�С������ȡ����
		size_t block_count = (differ_capacity / m_block_size) + ((differ_capacity % m_block_size == 0) ? 0 : 1);

		//�Ӹ��ڵ㿪ʼ��ֱ�����ҵ����һ���ڵ�Ϊֹ
		Node* tempnode = m_root;
		while (tempnode->m_next)
		{
			tempnode = tempnode->m_next;
		}


		//�����ĵ�һ���洢�ڵ�
		Node* first_newnode = NULL;

		//�½���Ӧ�����Ĵ洢�ڵ�
		for (size_t i = 0; i < block_count; ++i)
		{
			tempnode->m_next = new Node(m_block_size);

			//���������ĵ�һ���洢�ڵ�
			if (first_newnode == NULL)
			{
				first_newnode = tempnode->m_next;
			}

			tempnode = tempnode->m_next;
			//ÿ�½�һ���洢�ڵ㣬����������һ���ڴ��Ĵ�С
			m_capacity += m_block_size;
		}

		//���ԭ�ȿ�������Ϊ0���������ĵ�һ���ڵ�������ǰ�����ڵ㣨ԭ��Ӧ��ΪNULL��
		if (available_capacity == 0)
		{
			m_current = first_newnode;
		}
	}

	//ZigZag�������������¿��Լ�С���ݵĴ�С���ر����ڴ����������ֵС�ĸ���������ʱ�������ض�����£��������ٻ����ݵľ���ֵ�ϴ�ʱ������Ҳ���ܵ������ݴ�С������
	//32λzigzag����
	uint32_t ByteArray::EncodeZigzag32(const int32_t value)
	{
		//������ת��Ϊ�������Ǹ���ת��Ϊż��
		if (value < 0)
		{
			return (uint32_t(-value)) * 2 - 1;
		}
		else
		{
			return (uint32_t)value * 2;
		}
	}
	//64λzigzag����
	uint64_t ByteArray::EncodeZigzag64(const int64_t value)
	{
		//������ת��Ϊ�������Ǹ���ת��Ϊż��
		if (value < 0)
		{
			return (uint64_t(-value)) * 2 - 1;
		}
		else
		{
			return (uint64_t)value * 2;
		}
	}

	//32λzigzag����
	int32_t ByteArray::DecodeZigzag32(const uint32_t value)
	{
		//������ת��Ϊ������ż��ת��Ϊ�Ǹ���
		return (value >> 1) ^ -(value & 1);
	}
	//64λzigzag����
	int64_t ByteArray::DecodeZigzag64(const uint64_t value)
	{
		//������ת��Ϊ������ż��ת��Ϊ�Ǹ���
		return (value >> 1) ^ -(value & 1);
	}
}

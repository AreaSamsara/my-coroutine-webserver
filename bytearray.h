#pragma once
#include <memory>
#include <vector>
#include <sys/uio.h>

namespace ByteArraySpace
{
	using std::string;
	using std::vector;

	//�ֽ�������
	class ByteArray
	{
	public:
		//�洢�ڵ���
		class Node
		{
		public:
			Node(const size_t block_size);
			Node();
			~Node();

			//�ڴ���ַָ��
			char* m_ptr;
			//��һ���洢�ڵ�
			Node* m_next;
			//�洢���ڴ���С����λ���ֽڣ�
			size_t m_block_size;
		};
	public:
		ByteArray(const size_t block_size = 4096);
		~ByteArray();

		//write
		//д��̶����ȵĶ�Ӧ���͵�����
		void writeFint8(const int8_t value);
		void writeFuint8(const uint8_t value);
		void writeFint16(const int16_t value);
		void writeFuint16(const uint16_t value);
		void writeFint32(const int32_t value);
		void writeFuint32(const uint32_t value);
		void writeFint64(const int64_t value);
		void writeFuint64(const uint64_t value);

		//д��ɱ䳤�ȣ�ѹ�����Ķ�Ӧ���͵�����
		void writeInt32(const int32_t value);
		void writeUint32(uint32_t value);
		void writeInt64(const int64_t value);
		void writeUint64(uint64_t value);

		//д��float���͵�����
		void writeFloat(const float value);
		//д��double���͵�����
		void writeDouble(const double value);

		//д��string���͵����ݣ��ù̶����ȵ�uint16_t��Ϊ��������
		void writeStringF16(const string& value);
		//д��string���͵����ݣ��ù̶����ȵ�uint32_t��Ϊ��������
		void writeStringF32(const string& value);
		//д��string���͵����ݣ��ù̶����ȵ�uint64_t��Ϊ��������
		void writeStringF64(const string& value);
		//д��string���͵����ݣ��ÿɱ䳤�ȵ�uint64_t��Ϊ��������
		void writeStringVint(const string& value);
		//д��string���͵����ݣ�����������
		void writeStringWithoutLength(const string& value);


		//read 
		//��ȡ�̶����ȵĶ�Ӧ���͵�����
		int8_t readFint8();
		uint8_t readFuint8();
		int16_t readFint16();
		uint16_t readFuint16();
		int32_t readFint32();
		uint32_t readFuint32();
		int64_t readFint64();
		uint64_t readFuint64();

		//��ȡ�ɱ䳤�ȣ�ѹ�����Ķ�Ӧ���͵�����
		int32_t readInt32();
		uint32_t readUint32();
		int64_t readInt64();
		uint64_t readUint64();

		//��ȡfloat���͵�����
		float readFloat();
		//��ȡdouble���͵�����
		double readDouble();

		//��ȡstring���͵����ݣ���uint16_t��Ϊ��������
		string readStringF16();
		//��ȡstring���͵����ݣ���uint32_t��Ϊ��������
		string readStringF32();
		//��ȡstring���͵����ݣ���uint64_t��Ϊ��������
		string readStringF64();
		//��ȡstring���͵����ݣ����޷���Varint64��Ϊ��������
		string readStringVint();


		//д��size���ȵ����ݣ�ͬʱ�ƶ���ǰ����λ�ã�
		void write(const void* buffer, size_t size);
		//��ȡsize���ȵ����ݣ�ͬʱ�ƶ���ǰ����λ�ã�
		void read(void* buffer, size_t size);
		//��ȡsize���ȵ����ݣ����ƶ���ǰ����λ�ã�
		void read(void* buffer, size_t size, size_t position)const;
		//��ByteArray������д�뵽�ļ���
		bool writeToFile(const string& filename)const;
		//���ļ��ж�ȡ����
		bool readFromFile(const string& filename);


		//����ÿ�����洢�ڵ�ģ��ڴ��Ĵ�С
		size_t getBlock_size()const { return m_block_size; }
		//���ؿɶ�ȡ���ݴ�С
		size_t getRead_size()const { return m_data_size - m_position; }
		//�������ݵĳ���
		size_t getData_size()const { return m_data_size; }
		//����ByteArray��ǰλ��
		size_t getPosition()const { return m_position; }
		//����ByteArray��ǰλ��
		void setPosition(size_t position);

		

		//��ByteArray���������[m_position, m_size)ת��string����
		string toString() const;
		//��ByteArray���������[m_position, m_size)ת��16���Ƶ�string����
		string toHexString() const;


		//���ByteArray
		void clear();

		//ֻ��ȡ���ݣ����޸�position
		//��ȡ�ɶ�ȡ�Ļ���,�����iovec����
		uint64_t getReadBuffers(vector<iovec>& buffers, uint64_t length = ~0ull);
		//��ȡ�ɶ�ȡ�Ļ���,�����iovec����,��positionλ�ÿ�ʼ
		uint64_t getReadBuffers(vector<iovec>& buffers, uint64_t length, uint64_t position)const;

		//�������������޸�position
		//��ȡ��д��Ļ���,�����iovec����
		uint64_t getWriteBuffers(vector<iovec>& buffers, uint64_t length);		
	private:
		//����ByteArrayʹ���д����������Ϊָ��ֵ(���ԭ������������,������)
		void expendCapacity(const size_t capacity_required);
		//��ȡ��ǰ�Ŀ�д������
		size_t getAvailable_capacity()const { return m_capacity - m_position; }

		//ZigZag�������������¿��Լ�С���ݵĴ�С���ر����ڴ����������ֵС�ĸ���������ʱ�������ض�����£��������ٻ����ݵľ���ֵ�ϴ�ʱ������Ҳ���ܵ������ݴ�С������
		//32λzigzag����
		uint32_t EncodeZigzag32(const int32_t value);
		//64λzigzag����
		uint64_t EncodeZigzag64(const int64_t value);

		//32λzigzag����
		int32_t DecodeZigzag32(const uint32_t value);
		//64λzigzag����
		int64_t DecodeZigzag64(const uint64_t value);
	private:
		//ÿ�����洢�ڵ�ģ��ڴ��Ĵ�С(��λ���ֽ�)
		size_t m_block_size;
		//��ǰ������(��λ���ֽ�)(�洢�ڵ��С*�洢�ڵ����)
		size_t m_capacity;
		//��ǰ�ѱ������ݵĴ�С(��λ���ֽ�)
		size_t m_data_size;

		//��ǰ����λ��
		size_t m_position;
		//��һ���洢�ڵ�ָ��
		Node* m_root;
		//��ǰ�����Ĵ洢�ڵ�ָ��
		Node* m_current;
	};
}

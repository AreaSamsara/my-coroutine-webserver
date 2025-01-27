#pragma once
#include <memory>
#include <vector>
#include <sys/uio.h>

namespace ByteArraySpace
{
	using std::string;
	using std::vector;

	/*
	* �ֽ�����ϵͳ���÷�����
	* 1.ByteArray����һ�������࣬���԰��ڴ�飨��СΪһ�������ֽڣ��洢���ݣ��������Ľṹ�洢�ڴ���Լ����ڴ���Ƭ
	* 2.����ͨ���ƶ���ǰλ���������ֽ�����
	* 3.���԰�������ԭ�����͵Ĺ̶����Ƚ��ж�д��Ҳ���Զ�int��uint���ͽ���ѹ�����ٶ�д
	*/

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
			const size_t m_block_size;
		};
	public:
		ByteArray(const size_t block_size = 4096);
		~ByteArray();

		//write
		//д��̶����ȵĶ�Ӧ���͵�����
		void writeInt8(const int8_t value);
		void writeUint8(const uint8_t value);
		void writeInt16(const int16_t value);
		void writeUint16(const uint16_t value);
		void writeInt32(const int32_t value);
		void writeUint32(const uint32_t value);
		void writeInt64(const int64_t value);
		void writeUint64(const uint64_t value);

		//д��ɱ䳤�ȣ�ѹ�����Ķ�Ӧ���͵�����
		void writeInt32_compressed(const int32_t value);
		void writeUint32_compressed(uint32_t value);
		void writeInt64_compressed(const int64_t value);
		void writeUint64_compressed(uint64_t value);

		//д��float���͵�����
		void writeFloat(const float value);
		//д��double���͵�����
		void writeDouble(const double value);

		//д��string���͵����ݣ��ù̶����ȵ�uint16_t��Ϊ��������
		void writeString16(const string& value);
		//д��string���͵����ݣ��ù̶����ȵ�uint32_t��Ϊ��������
		void writeString32(const string& value);
		//д��string���͵����ݣ��ù̶����ȵ�uint64_t��Ϊ��������
		void writeString64(const string& value);
		//д��string���͵����ݣ��ÿɱ䳤�ȵ�uint64_t��Ϊ��������
		void writeString64_compressed(const string& value);
		//д��string���͵����ݣ�����������
		void writeStringWithoutLength(const string& value);


		//read 
		//��ȡ�̶����ȵĶ�Ӧ���͵�����
		int8_t readInt8();
		uint8_t readUint8();
		int16_t readInt16();
		uint16_t readUint16();
		int32_t readInt32();
		uint32_t readUint32();
		int64_t readInt64();
		uint64_t readUint64();

		//��ȡ�ɱ䳤�ȣ�ѹ�����Ķ�Ӧ���͵�����
		int32_t readInt32_compressed();
		uint32_t readUint32_compressed();
		int64_t readInt64_compressed();
		uint64_t readUint64_compressed();

		//��ȡfloat���͵�����
		float readFloat();
		//��ȡdouble���͵�����
		double readDouble();

		//��ȡstring���͵����ݣ���uint16_t��Ϊ��������
		string readString16();
		//��ȡstring���͵����ݣ���uint32_t��Ϊ��������
		string readString32();
		//��ȡstring���͵����ݣ���uint64_t��Ϊ��������
		string readString64();
		//��ȡstring���͵����ݣ��ÿɱ䳤�ȣ�ѹ������uint64_t��Ϊ��������
		string readString64_compressed();


		//д��ָ�����ȵ����ݣ�ͬʱ�ƶ���ǰ����λ�ã�
		void write(const void* buffer, size_t write_size);
		//��ȡָ�����ȵ����ݣ�ͬʱ�ƶ���ǰ����λ�ã�
		void read(void* buffer, size_t read_size);
		//��ָ��λ�ÿ�ʼ��ȡָ�����ȵ����ݣ����ƶ���ǰ����λ�ã�
		void read_without_moving(void* buffer, size_t read_size, size_t position)const;
		//��ByteArray������д�뵽�ļ���
		bool writeToFile(const string& filename)const;
		//���ļ��ж�ȡ����
		bool readFromFile(const string& filename);
		//�ӵ�ǰλ�ÿ�ʼ��ȡ��д��Ļ���,�����iovec���飨���޸�position���������ݣ�
		uint64_t getWriteBuffers(vector<iovec>& buffers, const uint64_t size_to_write);
		//��positionλ�ÿ�ʼ��ȡ�ɶ�ȡ�Ļ���,�����iovec���飨���޸�position��
		uint64_t getReadBuffers(vector<iovec>& buffers, const uint64_t read_size, uint64_t position)const;


		//����ÿ�����洢�ڵ�ģ��ڴ��Ĵ�С
		size_t getBlock_size()const { return m_block_size; }
		//���ؿɶ�ȡ���ݴ�С���ѱ������ݵ��ܴ�С��ȥ��ǰλ�ã�
		size_t getRead_size()const { return m_data_size - m_position; }
		//�������ݵĳ���
		size_t getData_size()const { return m_data_size; }
		//����ByteArray��ǰλ��
		size_t getPosition()const { return m_position; }
		//����ByteArray��ǰλ�ã�ͬʱ�ı䵱ǰ�����ڵ�λ�ã�
		void setPosition(size_t position);

		//��ByteArray���������[m_position, m_size)ת��string����
		string toString() const;
		//��ByteArray���������[m_position, m_size)ת��16���Ƶ�string����
		string toHexString() const;

		//���ByteArray
		void clear();
	private:
		//����ByteArrayʹ���д����������Ϊָ��ֵ(���ԭ������������,������)
		void expendCapacity(const size_t capacity_required);
		//��ȡ��ǰ�Ŀ�д������
		size_t getAvailable_capacity()const { return m_capacity - m_position; }

		//ZigZag��������������¿��Լ�С���ݵĴ�С���ر����ڴ�����������ֵС�ĸ���������ʱ�������ض�����£��������ٻ����ݵľ���ֵ�ϴ�ʱ������Ҳ���ܵ������ݴ�С������
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
		const size_t m_block_size;
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

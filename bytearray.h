#pragma once
#include <memory>
#include <vector>
#include <sys/uio.h>

namespace ByteArraySpace
{
	using std::string;
	using std::vector;

	//字节序列类
	class ByteArray
	{
	public:
		//存储节点类
		class Node
		{
		public:
			Node(const size_t block_size);
			Node();
			~Node();

			//内存块地址指针
			char* m_ptr;
			//下一个存储节点
			Node* m_next;
			//存储的内存块大小（单位：字节）
			size_t m_block_size;
		};
	public:
		ByteArray(const size_t block_size = 4096);
		~ByteArray();

		//write
		//写入固定长度的对应类型的数据
		void writeFint8(const int8_t value);
		void writeFuint8(const uint8_t value);
		void writeFint16(const int16_t value);
		void writeFuint16(const uint16_t value);
		void writeFint32(const int32_t value);
		void writeFuint32(const uint32_t value);
		void writeFint64(const int64_t value);
		void writeFuint64(const uint64_t value);

		//写入可变长度（压缩）的对应类型的数据
		void writeInt32(const int32_t value);
		void writeUint32(uint32_t value);
		void writeInt64(const int64_t value);
		void writeUint64(uint64_t value);

		//写入float类型的数据
		void writeFloat(const float value);
		//写入double类型的数据
		void writeDouble(const double value);

		//写入string类型的数据，用固定长度的uint16_t作为长度类型
		void writeStringF16(const string& value);
		//写入string类型的数据，用固定长度的uint32_t作为长度类型
		void writeStringF32(const string& value);
		//写入string类型的数据，用固定长度的uint64_t作为长度类型
		void writeStringF64(const string& value);
		//写入string类型的数据，用可变长度的uint64_t作为长度类型
		void writeStringVint(const string& value);
		//写入string类型的数据，不附带长度
		void writeStringWithoutLength(const string& value);


		//read 
		//读取固定长度的对应类型的数据
		int8_t readFint8();
		uint8_t readFuint8();
		int16_t readFint16();
		uint16_t readFuint16();
		int32_t readFint32();
		uint32_t readFuint32();
		int64_t readFint64();
		uint64_t readFuint64();

		//读取可变长度（压缩）的对应类型的数据
		int32_t readInt32();
		uint32_t readUint32();
		int64_t readInt64();
		uint64_t readUint64();

		//读取float类型的数据
		float readFloat();
		//读取double类型的数据
		double readDouble();

		//读取string类型的数据，用uint16_t作为长度类型
		string readStringF16();
		//读取string类型的数据，用uint32_t作为长度类型
		string readStringF32();
		//读取string类型的数据，用uint64_t作为长度类型
		string readStringF64();
		//读取string类型的数据，用无符号Varint64作为长度类型
		string readStringVint();


		//写入指定长度的数据（同时移动当前操作位置）
		void write(const void* buffer, size_t write_size);
		//读取指定长度的数据（同时移动当前操作位置）
		void read(void* buffer, size_t read_size);
		//从指定位置开始读取指定长度的数据（不移动当前操作位置）
		void read_without_moving(void* buffer, size_t read_size, size_t position)const;
		//把ByteArray的数据写入到文件中
		bool writeToFile(const string& filename)const;
		//从文件中读取数据
		bool readFromFile(const string& filename);


		//返回每个（存储节点的）内存块的大小
		size_t getBlock_size()const { return m_block_size; }
		//返回可读取数据大小（已保存数据的总大小减去当前位置）
		size_t getRead_size()const { return m_data_size - m_position; }
		//返回数据的长度
		size_t getData_size()const { return m_data_size; }
		//返回ByteArray当前位置
		size_t getPosition()const { return m_position; }
		//设置ByteArray当前位置（同时改变当前操作节点位置）
		void setPosition(size_t position);

		

		//将ByteArray里面的数据[m_position, m_size)转成string对象
		string toString() const;
		//将ByteArray里面的数据[m_position, m_size)转成16进制的string对象
		string toHexString() const;


		//清空ByteArray
		void clear();

		//获取可读取的缓存,保存成iovec数组（不修改position）
		//uint64_t getReadBuffers(vector<iovec>& buffers, uint64_t read_size = ~0ull)const;
		//获取可读取的缓存,保存成iovec数组,从position位置开始
		uint64_t getReadBuffers(vector<iovec>& buffers, const uint64_t read_size, uint64_t position)const;

		//获取可写入的缓存,保存成iovec数组（不修改position，可能扩容）
		uint64_t getWriteBuffers(vector<iovec>& buffers, const uint64_t size_to_write);
	private:
		//扩容ByteArray使其可写入容量至少为指定值(如果原本就足以容纳,则不扩容)
		void expendCapacity(const size_t capacity_required);
		//获取当前的可写入容量
		size_t getAvailable_capacity()const { return m_capacity - m_position; }

		//ZigZag编码在许多情况下可以减小数据的大小，特别是在处理大量绝对值小的负数或正数时，但在特定情况下（负数较少或数据的绝对值较大时），它也可能导致数据大小的增加
		//32位zigzag编码
		uint32_t EncodeZigzag32(const int32_t value);
		//64位zigzag编码
		uint64_t EncodeZigzag64(const int64_t value);

		//32位zigzag解码
		int32_t DecodeZigzag32(const uint32_t value);
		//64位zigzag解码
		int64_t DecodeZigzag64(const uint64_t value);
	private:
		//每个（存储节点的）内存块的大小(单位：字节)
		size_t m_block_size;
		//当前总容量(单位：字节)(存储节点大小*存储节点个数)
		size_t m_capacity;
		//当前已保存数据的大小(单位：字节)
		size_t m_data_size;

		//当前操作位置
		size_t m_position;
		//第一个存储节点指针
		Node* m_root;
		//当前操作的存储节点指针
		Node* m_current;
	};
}

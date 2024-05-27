#pragma once
#include <memory>
#include <vector>
#include <sys/uio.h>

namespace ByteArraySpace
{
	using std::string;
	using std::vector;

	class ByteArray
	{
	public:
		class Node
		{
		public:
			Node(const size_t size);
			Node();
			~Node();

			char* m_ptr;
			Node* m_next;
			size_t m_size;
		};
	public:
		ByteArray(const size_t base_size = 4096);
		~ByteArray();

		//write
		void writeFint8(const int8_t value);
		void writeFuint8(const uint8_t value);
		void writeFint16(const int16_t value);
		void writeFuint16(const uint16_t value);
		void writeFint32(const int32_t value);
		void writeFuint32(const uint32_t value);
		void writeFint64(const int64_t value);
		void writeFuint64(const uint64_t value);

		void writeInt32(const int32_t value);
		void writeUint32(uint32_t value);
		void writeInt64(const int64_t value);
		void writeUint64(uint64_t value);

		void writeFloat(const float value);
		void writeDouble(const double value);

		//length:int16,data
		void writeStringF16(const string& value);
		//length:int32,data
		void writeStringF32(const string& value);
		//length:int64,data
		void writeStringF64(const string& value);
		//length:varint,data
		void writeStringVint(const string& value);
		//data
		void writeStringWithoutLength(const string& value);


		//read 
		int8_t readFint8();
		uint8_t readFuint8();
		int16_t readFint16();
		uint16_t readFuint16();
		int32_t readFint32();
		uint32_t readFuint32();
		int64_t readFint64();
		uint64_t readFuint64();

		int32_t readInt32();
		uint32_t readUint32();
		int64_t readInt64();
		uint64_t readUint64();

		float readFloat();
		double readDouble();

		//length:int16,data
		string readStringF16();
		//length:int32,data
		string readStringF32();
		//length:int64,data
		string readStringF64();
		//length:varint,data
		string readStringVint();

		//内部操作
		void clear();

		void write(const void* buffer, size_t size);
		void read(void* buffer, size_t size);
		void read(void* buffer, size_t size, size_t position)const;

		size_t getBase_size()const { return m_base_size; }

		size_t getPosition()const { return m_position; }
		void setPosition(size_t position);

		size_t getRead_size()const { return m_size - m_position; }

		bool writeToFile(const string& name)const;
		bool readFromFile(const string& name);

		string toString() const;
		string toHexString() const;

		//只获取内容，不修改position
		int64_t getReadBuffers(vector<iovec>& buffers, uint64_t length = ~0ull);
		int64_t getReadBuffers(vector<iovec>& buffers, uint64_t length, uint64_t position)const;
		//增加容量，不修改position
		int64_t getWriteBuffers(vector<iovec>& buffers, uint64_t length);

		size_t getSize()const { return m_size; }
	private:
		void addCapacity(size_t size);
		size_t getCapacity()const { return m_capacity - m_position; }
	private:
		size_t m_base_size;
		size_t m_position;
		size_t m_capacity;
		size_t m_size;
		int8_t m_endian;
		Node* m_root;
		Node* m_current;
	};
}

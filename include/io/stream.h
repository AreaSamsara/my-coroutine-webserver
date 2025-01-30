#pragma once
#include <memory>
#include "io/bytearray.h"

namespace StreamSpace
{
	using namespace ByteArraySpace;
	using std::shared_ptr;

	class Stream
	{
	public:
		virtual ~Stream() {}
		virtual int read(void *buffer, const size_t read_length) = 0;
		virtual int read(shared_ptr<ByteArray> bytearray, const size_t read_length) = 0;
		virtual int read_fixed_size(void *buffer, const size_t read_length);
		virtual int read_fixed_size(shared_ptr<ByteArray> bytearray, const size_t read_length);

		virtual int write(const void *buffer, const size_t write_length) = 0;
		virtual int write(shared_ptr<ByteArray> bytearray, const size_t write_length) = 0;
		virtual int write_fixed_size(const void *buffer, const size_t write_length);
		virtual int write_fixed_size(shared_ptr<ByteArray> bytearray, const size_t write_length);

		virtual void close() = 0;
	};
}
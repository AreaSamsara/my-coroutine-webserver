#include "stream.h"

namespace StreamSpace
{
	int Stream::read_fixed_size(void* buffer, const size_t read_length)
	{
		size_t offset = 0;
		size_t length_to_read = read_length;
		while (length_to_read > 0)
		{
			size_t return_value = read((char*)buffer + offset, length_to_read);
			//如果read()函数出错，直接返回该值
			if (return_value <= 0)
			{
				return return_value;
			}
			offset += return_value;
			length_to_read -= return_value;
		}
		return read_length;
	}
	int Stream::read_fixed_size(shared_ptr<ByteArray> bytearray,const size_t read_length)
	{
		size_t length_to_read = read_length;
		while (length_to_read > 0)
		{
			size_t return_value = read(bytearray, length_to_read);
			//如果read()函数出错，直接返回该值
			if (return_value <= 0)
			{
				return return_value;
			}
			length_to_read -= return_value;
		}
		return read_length;
	}

	int Stream::write_fixed_size(const void* buffer, const size_t write_length)
	{
		size_t offset = 0;
		size_t length_to_write = write_length;
		while (length_to_write > 0)
		{
			size_t return_value = write((char*)buffer + offset, length_to_write);
			//如果write()函数出错，直接返回该值
			if (return_value <= 0)
			{
				return return_value;
			}
			offset += return_value;
			length_to_write -= return_value;
		}
		return write_length;
	}
	int Stream::write_fixed_size(shared_ptr<ByteArray> bytearray, const size_t write_length)
	{
		size_t length_to_write = write_length;
		while (length_to_write > 0)
		{
			size_t return_value = write(bytearray, length_to_write);
			//如果write()函数出错，直接返回该值
			if (return_value <= 0)
			{
				return return_value;
			}
			length_to_write -= return_value;
		}
		return write_length;
	}
}
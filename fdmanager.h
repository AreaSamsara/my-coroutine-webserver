#pragma once
#include <memory>
#include "thread.h"
#include "iomanager.h"

namespace FdManagerSpace
{
	using namespace IOManagerSpace;
	using std::enable_shared_from_this;

	//文件描述符实体类
	class FileDescriptorEntity :public enable_shared_from_this<FileDescriptorEntity>
	{
	public:
		FileDescriptorEntity(const int file_descriptor);
		~FileDescriptorEntity();

		//获取私有成员
		bool isInitialized()const { return m_is_initialized; }
		bool isSocket()const { return m_is_socket; }
		bool isSystemNonblock()const { return m_is_systemNonblock; }
		bool isUserNonblock()const { return m_is_userNonblock; }
		bool isClose()const { return m_is_close; }
		uint64_t getTimeout(const int type)const;

		//修改私有成员
		void setSystemNonblock(const bool value) { m_is_systemNonblock = value; }
		void setUserNonblock(const bool value) { m_is_userNonblock = value; }
		void setTimeout(const int type, const uint64_t timeout);

		bool initialize();
		//bool close();
	private:
		//将bool值按只占1位的位字段打包，节省内存（位字段不可在定义的同时初始化）
		//是否已初始化
		bool m_is_initialized : 1;
		//是否socket
		bool m_is_socket : 1;
		//是否hook非阻塞
		bool m_is_systemNonblock : 1;
		//是否用户主动设置为非阻塞
		bool m_is_userNonblock : 1;
		//是否处于关闭状态
		bool m_is_close : 1;
		//文件描述符
		int m_file_descriptor;
		//读取的超时时间
		uint64_t m_recv_timeout = -1;
		//写入的超时时间
		uint64_t m_send_timeout = -1;
	};

	//文件描述符管理者
	class FileDescriptorManager
	{
	public:
		FileDescriptorManager();

		shared_ptr<FileDescriptorEntity> getFile_descriptor(const int file_descriptor, const bool auto_create = false);
		void deleteFile_descriptor(const int file_descriptor);
	private:
		//互斥锁（读/写锁）
		Mutex_Read_Write m_mutex;
		vector<shared_ptr<FileDescriptorEntity>> m_file_descriptor_entities;
	};
}
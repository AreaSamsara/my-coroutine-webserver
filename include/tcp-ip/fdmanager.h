#pragma once
#include <memory>
#include "concurrent/thread.h"
#include "concurrent/iomanager.h"

namespace FdManagerSpace
{
	using namespace IOManagerSpace;

	/*
	 * 类关系：
	 * FileDescriptorEntity包含文件描述符及其相关信息以及设置超时时间的方法
	 * FileDescriptorManager内含装有多个FileDescriptorEntity的容器，对FileDescriptorEntity进行管理
	 */
	/*
	 * 文件描述符管理系统调用方法：
	 * 本系统用于封装文件描述符，以辅助其他系统（如hook系统、socket系统）对文件描述符进行管理
	 */

	// 文件描述符实体类
	class FileDescriptorEntity
	{
	public:
		FileDescriptorEntity(const int file_descriptor);
		~FileDescriptorEntity() {}

		// 获取私有成员
		bool isSocket() const { return m_is_socket; }
		bool isSystemNonblock() const { return m_is_systemNonblock; }
		bool isUserNonblock() const { return m_is_userNonblock; }
		bool isClose() const { return m_is_close; }

		// 修改私有成员
		void setSystemNonblock(const bool value) { m_is_systemNonblock = value; }
		void setUserNonblock(const bool value) { m_is_userNonblock = value; }

		// 根据类型获取对应的超时时间
		uint64_t getTimeout(const int type) const;
		// 根据类型设置对应的超时时间
		void setTimeout(const int type, const uint64_t timeout);

	private:
		// 将bool值按只占1位的位字段打包，节省内存（位字段不可在定义的同时初始化，故在初始化列表进行赋值）
		// 是否socket
		bool m_is_socket : 1;
		// 是否hook非阻塞
		bool m_is_systemNonblock : 1;
		// 是否用户主动设置为非阻塞
		bool m_is_userNonblock : 1;
		// 是否处于关闭状态
		bool m_is_close : 1;
		// 文件描述符
		int m_file_descriptor;
		// 读取的超时时间
		uint64_t m_recv_timeout = -1;
		// 写入的超时时间
		uint64_t m_send_timeout = -1;
	};

	// 文件描述符管理者
	class FileDescriptorManager
	{
	public:
		FileDescriptorManager();

		// 获取文件描述符对应的实体，并在该实体不存在且auto_create为true时创建之
		shared_ptr<FileDescriptorEntity> getFile_descriptor(const int file_descriptor, const bool auto_create = false);
		// 删除文件描述符对应的实体
		void deleteFile_descriptor(const int file_descriptor);

	private:
		// 互斥锁（读/写锁）
		Mutex_Read_Write m_mutex;
		// 文件描述符实体容器
		vector<shared_ptr<FileDescriptorEntity>> m_file_descriptor_entities;
	};
}
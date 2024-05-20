#include "fdmanager.h"
#include "sys/stat.h"
#include "hook.h"

namespace FdManagerSpace
{
	using namespace HookSpace;

	//class FileDescriptorEntity:public
	FileDescriptorEntity::FileDescriptorEntity(const int file_descriptor)
		:m_file_descriptor(file_descriptor),m_is_initialized(false),m_is_socket(false)
		,m_is_systemNonblock(false),m_is_userNonblock(false),m_is_close(false)
	{
		initialize();
	}

	//根据类型获取对应的超时时间
	uint64_t FileDescriptorEntity::getTimeout(const int type)const
	{
		if (type == SO_RCVTIMEO)
		{
			return m_recv_timeout;
		}
		else
		{
			return m_send_timeout;
		}
	}

	//根据类型设置对应的超时时间
	void FileDescriptorEntity::setTimeout(const int type, const uint64_t timeout)
	{
		if (type == SO_RCVTIMEO)
		{
			m_recv_timeout = timeout;
		}
		else
		{
			m_send_timeout = timeout;
		}
	}

	//class FileDescriptorEntity:private
	//初始化文件描述符实体
	bool FileDescriptorEntity::initialize()
	{
		//如果已经初始化了，直接返回true
		if (m_is_initialized)
		{
			return true;
		}
		//重置超时时间
		m_recv_timeout = -1;
		m_send_timeout = -1;

		//尝试获取文件状态
		struct stat file_descriptor_state;
		//如果获取失败（fstat()返回-1），设置文件描述符为未初始化且非socket
		if (fstat(m_file_descriptor, &file_descriptor_state) == -1)
		{
			m_is_initialized = false;
			m_is_socket = false;
		}
		//否则设置文件描述符为已初始化，并根据S_ISSOCK设置其是否为socket
		else
		{
			m_is_initialized = true;
			m_is_socket = S_ISSOCK(file_descriptor_state.st_mode);
		}

		//如果该文件描述符是socket，设置文件描述符为hook非阻塞
		if (m_is_socket)
		{
			//尝试获取文件标志
			int flags = fcntl_origin(m_file_descriptor, F_GETFL, 0);
			//如果文件标志未设置非阻塞，设置之
			if (!(flags & O_NONBLOCK))
			{
				fcntl_origin(m_file_descriptor, F_SETFL, flags | O_NONBLOCK);
			}
			//设置文件描述符为hook非阻塞
			m_is_systemNonblock = true;
		}
		//否则设置文件描述符为hook阻塞
		else
		{
			m_is_systemNonblock = false;
		}

		//重置文件描述符为用户主动设置非阻塞、关闭状态
		m_is_userNonblock = false;
		m_is_close = false;

		return m_is_initialized;
	}





	//class FileDescriptorManager:public
	FileDescriptorManager::FileDescriptorManager()
	{
		//容器初始大小设置为64
		m_file_descriptor_entities.resize(64);
	}

	//获取文件描述符对应的实体，并在该实体不存在且auto_create为true时创建之
	shared_ptr<FileDescriptorEntity> FileDescriptorManager::getFile_descriptor(const int file_descriptor, const bool auto_create)
	{
		//先加互斥锁，保护m_file_descriptor_entities
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
		//如果容器大小不足
		if (m_file_descriptor_entities.size() <= file_descriptor)
		{
			//如果不需要自动创建新的实体，则直接返回空指针
			if (!auto_create)
			{
				return nullptr;
			}
			//否则扩充容器大小到需求值的1.5倍并创建新实体，返回之
			else
			{
				//解锁读取锁
				readlock.unlock();
				//先加互斥锁，保护m_file_descriptor_entities
				WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

				//扩充容器大小到需求值的1.5倍
				m_file_descriptor_entities.resize(1.5 * file_descriptor);

				//根据传入的文件描述符创建对应实体并返回
				shared_ptr<FileDescriptorEntity> file_descriptor_entity(new FileDescriptorEntity(file_descriptor));
				m_file_descriptor_entities[file_descriptor] = file_descriptor_entity;
				return file_descriptor_entity;
			}
		}
		//如果容器大小充足
		else
		{
			//如果文件描述符对应的实体不为空或不需要自动创建，直接返回对应的实体
			if (m_file_descriptor_entities[file_descriptor] || !auto_create)
			{
				return m_file_descriptor_entities[file_descriptor];
			}
			//否则创建对应实体并返回
			else
			{
				//解锁读取锁
				readlock.unlock();

				//先加互斥锁，保护m_file_descriptor_entities
				WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

				//根据传入的文件描述符创建对应实体并返回
				shared_ptr<FileDescriptorEntity> file_descriptor_entity(new FileDescriptorEntity(file_descriptor));
				m_file_descriptor_entities[file_descriptor] = file_descriptor_entity;
				return file_descriptor_entity;
			}
		}
	}

	//删除文件描述符对应的实体
	void FileDescriptorManager::deleteFile_descriptor(const int file_descriptor)
	{
		//先加互斥锁，保护m_file_descriptor_entities
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
		//仅在传入的文件描述符有效时，将对应的实体清空
		if (m_file_descriptor_entities.size() > file_descriptor)
		{
			m_file_descriptor_entities[file_descriptor].reset();
		}
	}
}
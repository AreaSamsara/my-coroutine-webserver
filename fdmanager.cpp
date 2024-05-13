#include "fdmanager.h"
#include "sys/stat.h"
#include "hook.h"

namespace FdManagerSpace
{
	using namespace HookSpace;

	FileDescriptorEntity::FileDescriptorEntity(const int file_descriptor)
		:m_file_descriptor(file_descriptor),m_is_initialized(false),m_is_socket(false)
		,m_is_systemNonblock(false),m_is_userNonblock(false),m_is_close(false)
	{
		initialize();
	}
	FileDescriptorEntity::~FileDescriptorEntity()
	{

	}

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

	bool FileDescriptorEntity::initialize()
	{
		if (m_is_initialized)
		{
			return true;
		}
		m_recv_timeout = -1;
		m_send_timeout = -1;

		struct stat file_descriptor_state;
		if (fstat(m_file_descriptor, &file_descriptor_state) == -1)
		{
			m_is_initialized = false;
			m_is_socket = false;
		}
		else
		{
			m_is_initialized = true;
			m_is_socket = S_ISSOCK(file_descriptor_state.st_mode);
		}

		if (m_is_socket)
		{
			int flags = fcntl_f(m_file_descriptor, F_GETFL, 0);
			if (!(flags & O_NONBLOCK))
			{
				fcntl_f(m_file_descriptor, F_SETFL, flags | O_NONBLOCK);
			}
			m_is_systemNonblock = true;
		}
		else
		{
			m_is_systemNonblock = false;
		}

		m_is_userNonblock = false;
		m_is_close = false;
		return m_is_initialized;
	}
	//bool close();






	FileDescriptorManager::FileDescriptorManager()
	{
		m_file_descriptor_entities.resize(64);
	}

	shared_ptr<FileDescriptorEntity> FileDescriptorManager::getFile_descriptor(const int file_descriptor, const bool auto_create)
	{
		//先加互斥锁，保护
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
		if (m_file_descriptor_entities.size() <= file_descriptor)
		{
			if (!auto_create)
			{
				return nullptr;
			}
		}
		else
		{
			if (m_file_descriptor_entities[file_descriptor] || !auto_create)
			{
				return m_file_descriptor_entities[file_descriptor];
			}
			readlock.unlock();

			//先加互斥锁，保护
			WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

			shared_ptr<FileDescriptorEntity> file_descriptor_entity(new FileDescriptorEntity(file_descriptor));
			m_file_descriptor_entities[file_descriptor] = file_descriptor_entity;
			return file_descriptor_entity;
		}
	}


	void FileDescriptorManager::deleteFile_descriptor(const int file_descriptor)
	{
		//先加互斥锁，保护
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
		if (m_file_descriptor_entities.size() <= file_descriptor)
		{
			return;
		}
		m_file_descriptor_entities[file_descriptor].reset();
	}
}
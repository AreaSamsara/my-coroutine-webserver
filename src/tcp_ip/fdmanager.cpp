#include "tcp_ip/fdmanager.h"
#include "sys/stat.h"
#include "concurrent/hook.h"

namespace FdManagerSpace
{
	using namespace HookSpace;

	// class FileDescriptorEntity:public
	FileDescriptorEntity::FileDescriptorEntity(const int file_descriptor)
		: m_file_descriptor(file_descriptor), m_is_socket(false), m_is_systemNonblock(false), m_is_userNonblock(false), m_is_close(false)
	{
		// ���Ի�ȡ�ļ�״̬
		struct stat file_descriptor_state;
		// �����ȡ�ɹ���fstat()������-1��������S_ISSOCK�������Ƿ�Ϊsocket������Ĭ������Ϊ��socket��
		if (fstat(m_file_descriptor, &file_descriptor_state) != -1)
		{
			m_is_socket = S_ISSOCK(file_descriptor_state.st_mode);
		}

		// ������ļ���������socket�������ļ�������Ϊhook������������Ĭ������Ϊhook������
		if (m_is_socket)
		{
			// ���Ի�ȡ�ļ���־��ʹ��ԭʼ�⺯����
			int flags = fcntl_origin(m_file_descriptor, F_GETFL, 0);
			// ����ļ���־δ���÷�����������֮��ʹ��ԭʼ�⺯����
			if (!(flags & O_NONBLOCK))
			{
				fcntl_origin(m_file_descriptor, F_SETFL, flags | O_NONBLOCK);
			}
			// �����ļ�������Ϊhook������
			m_is_systemNonblock = true;
		}
	}

	// �������ͻ�ȡ��Ӧ�ĳ�ʱʱ��
	uint64_t FileDescriptorEntity::getTimeout(const int type) const
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

	// �����������ö�Ӧ�ĳ�ʱʱ��
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

	// class FileDescriptorManager:public
	FileDescriptorManager::FileDescriptorManager()
	{
		// ������ʼ��С����Ϊ64
		m_file_descriptor_entities.resize(64);
	}

	// ��ȡ�ļ���������Ӧ��ʵ�壬���ڸ�ʵ�岻������auto_createΪtrueʱ����֮
	shared_ptr<FileDescriptorEntity> FileDescriptorManager::getFile_descriptor(const int file_descriptor, const bool auto_create)
	{
		// ����ļ�������Ϊ-1����Ч����ֱ�ӷ���nullptr
		if (file_descriptor == -1)
		{
			return nullptr;
		}

		// �ȼӻ�����������m_file_descriptor_entities
		ReadScopedLock<Mutex_Read_Write> readlock(m_mutex);
		// ���������С����
		if (m_file_descriptor_entities.size() <= file_descriptor)
		{
			// �������Ҫ�Զ������µ�ʵ�壬��ֱ�ӷ��ؿ�ָ��
			if (!auto_create)
			{
				return nullptr;
			}
			// ��������������С������ֵ��1.5����������ʵ�壬����֮
			else
			{
				// ������ȡ��
				readlock.unlock();
				// �ȼӻ�����������m_file_descriptor_entities
				WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

				// ����������С������ֵ��1.5��
				m_file_descriptor_entities.resize(1.5 * file_descriptor);

				// ���ݴ�����ļ�������������Ӧʵ�岢����
				shared_ptr<FileDescriptorEntity> file_descriptor_entity(new FileDescriptorEntity(file_descriptor));
				m_file_descriptor_entities[file_descriptor] = file_descriptor_entity;
				return file_descriptor_entity;
			}
		}
		// ���������С����
		else
		{
			// ����ļ���������Ӧ��ʵ�岻Ϊ�ջ���Ҫ�Զ�������ֱ�ӷ��ض�Ӧ��ʵ��
			if (m_file_descriptor_entities[file_descriptor] || !auto_create)
			{
				return m_file_descriptor_entities[file_descriptor];
			}
			// ���򴴽���Ӧʵ�岢����
			else
			{
				// ������ȡ��
				readlock.unlock();

				// �ȼӻ�����������m_file_descriptor_entities
				WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);

				// ���ݴ�����ļ�������������Ӧʵ�岢����
				shared_ptr<FileDescriptorEntity> file_descriptor_entity(new FileDescriptorEntity(file_descriptor));
				m_file_descriptor_entities[file_descriptor] = file_descriptor_entity;
				return file_descriptor_entity;
			}
		}
	}

	// ɾ���ļ���������Ӧ��ʵ��
	void FileDescriptorManager::deleteFile_descriptor(const int file_descriptor)
	{
		// �ȼӻ�����������m_file_descriptor_entities
		WriteScopedLock<Mutex_Read_Write> writelock(m_mutex);
		// ���ڴ�����ļ���������Чʱ������Ӧ��ʵ�����
		if (m_file_descriptor_entities.size() > file_descriptor)
		{
			m_file_descriptor_entities[file_descriptor].reset();
		}
	}
}
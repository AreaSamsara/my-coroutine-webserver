#pragma once
#include <memory>
#include "thread.h"
#include "iomanager.h"

namespace FdManagerSpace
{
	using namespace IOManagerSpace;

	/*
	* ���ϵ��
	* FileDescriptorEntity�����ļ����������������Ϣ�Լ����ó�ʱʱ��ķ���
	* FileDescriptorManager�ں�װ�ж��FileDescriptorEntity����������FileDescriptorEntity���й���
	*/
	/*
	* �ļ�����������ϵͳ���÷�����
	* ��ϵͳ���ڷ�װ�ļ����������Ը�������ϵͳ����hookϵͳ��socketϵͳ�����ļ����������й���
	*/

	//�ļ�������ʵ����
	class FileDescriptorEntity
	{
	public:
		FileDescriptorEntity(const int file_descriptor);
		~FileDescriptorEntity() {}

		//��ȡ˽�г�Ա
		bool isSocket()const { return m_is_socket; }
		bool isSystemNonblock()const { return m_is_systemNonblock; }
		bool isUserNonblock()const { return m_is_userNonblock; }
		bool isClose()const { return m_is_close; }

		//�޸�˽�г�Ա
		void setSystemNonblock(const bool value) { m_is_systemNonblock = value; }
		void setUserNonblock(const bool value) { m_is_userNonblock = value; }

		//�������ͻ�ȡ��Ӧ�ĳ�ʱʱ��
		uint64_t getTimeout(const int type)const;
		//�����������ö�Ӧ�ĳ�ʱʱ��
		void setTimeout(const int type, const uint64_t timeout);
	private:
		//��boolֵ��ֻռ1λ��λ�ֶδ������ʡ�ڴ棨λ�ֶβ����ڶ����ͬʱ��ʼ�������ڳ�ʼ���б����и�ֵ��
		//�Ƿ�socket
		bool m_is_socket : 1;
		//�Ƿ�hook������
		bool m_is_systemNonblock : 1;
		//�Ƿ��û���������Ϊ������
		bool m_is_userNonblock : 1;
		//�Ƿ��ڹر�״̬
		bool m_is_close : 1;
		//�ļ�������
		int m_file_descriptor;
		//��ȡ�ĳ�ʱʱ��
		uint64_t m_recv_timeout = -1;
		//д��ĳ�ʱʱ��
		uint64_t m_send_timeout = -1;
	};

	//�ļ�������������
	class FileDescriptorManager
	{
	public:
		FileDescriptorManager();

		//��ȡ�ļ���������Ӧ��ʵ�壬���ڸ�ʵ�岻������auto_createΪtrueʱ����֮
		shared_ptr<FileDescriptorEntity> getFile_descriptor(const int file_descriptor, const bool auto_create = false);
		//ɾ���ļ���������Ӧ��ʵ��
		void deleteFile_descriptor(const int file_descriptor);
	private:
		//����������/д����
		Mutex_Read_Write m_mutex;
		//�ļ�������ʵ������
		vector<shared_ptr<FileDescriptorEntity>> m_file_descriptor_entities;
	};
}
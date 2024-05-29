#include "bytearray.h"
#include "utility.h"
#include "log.h"
#include "utility.h"
#include "singleton.h"

using namespace ByteArraySpace; 
using namespace UtilitySpace;
using namespace SingletonSpace;
using namespace LogSpace;
using std::shared_ptr;


void test()
{
	int count = 100, base_size = 1;


	vector<uint32_t> vec;
	//��ȡ��������
	for (int i = 0; i < count; ++i)
	{
		vec.push_back(rand());
	}

	shared_ptr<ByteArray> bytearray(new ByteArray(base_size));
	//���������������д��
	for (auto& i : vec)
	{
		bytearray->writeFuint32(i);
	}

	bytearray->setPosition(0);

	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		log_event->getSstream() << "bytearray:" << bytearray->toString();
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
	}


	for (size_t i = 0; i < vec.size(); ++i)
	{
		//��ȡ���������ӡ
		int32_t value = bytearray->readFuint32();

		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		log_event->getSstream() << i << "-" << value << "-" << (int32_t)vec[i];
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);

		//valueӦ�õ���vec[i]�����򱨴�
		if (value != vec[i])
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}
	}

	//��ʱ�ɶ����ݵĴ�СӦ��Ϊ�㣨���ݱ����꣩�����򱨴�
	if (bytearray->getRead_size() != 0)
	{
		shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		Assert(event);
	}




	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
	log_event->getSstream() << "write/read count=" << count << " base_size=" << base_size << " size=" << bytearray->getData_size();
	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
}

void test_file()
{
	int count = 100, base_size = 1;


	vector<uint64_t> vec;
	//��ȡ��������
	for (int i = 0; i < count; ++i)
	{
		vec.push_back(rand());
	}
	shared_ptr<ByteArray> bytearray(new ByteArray(base_size));
	//���������������д��
	for (auto& i : vec)
	{
		bytearray->writeUint64(i);
	}

	bytearray->setPosition(0);


	for (size_t i = 0; i < vec.size(); ++i)
	{
		//��ȡ�����
		int32_t value = bytearray->readUint64();

		//valueӦ�õ���vec[i]�����򱨴�
		if (value != vec[i])
		{
			shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
			Assert(event);
		}
	}

	//��ʱ�ɶ����ݵĴ�СӦ��Ϊ�㣨���ݱ����꣩�����򱨴�
	if (bytearray->getRead_size() != 0)
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		Assert(log_event);
	}

	shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
	log_event->getSstream() << "write/read count=" << count << " base_size=" << base_size << " size=" << bytearray->getData_size();
	Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);




	bytearray->setPosition(0);

	if (!bytearray->writeToFile("./test_bytearray.txt"))
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		Assert(log_event);
	}

	shared_ptr<ByteArray> bytearray2(new ByteArray(base_size * 2));

	if (!bytearray2->readFromFile("./test_bytearray.txt"))
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		Assert(log_event);
	}

	bytearray2->setPosition(0);


	/*{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		log_event->getSstream() << "bytearray:" << bytearray->toString();
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
		log_event->setSstream("");
		log_event->getSstream() << "bytearray2:" << bytearray2->toString();
		Singleton<LoggerManager>::GetInstance_normal_ptr()->getDefault_logger()->log(LogLevel::INFO, log_event);
	}*/

	if (bytearray->toString() != bytearray2->toString())
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		Assert(log_event);
	}

	if (bytearray->getPosition() != 0)
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		Assert(log_event);
	}

	if (bytearray2->getPosition() != 0)
	{
		shared_ptr<LogEvent> log_event(new LogEvent(__FILE__, __LINE__, GetThread_id(), GetThread_name(), GetFiber_id(), 0, time(0)));
		Assert(log_event);
	}
}


int main(int argc, char** argv)
{
	test();
	test_file();
	return 0;
}
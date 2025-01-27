//#pragma once
//#include <exception>
//#include <boost/lexical_cast.hpp>
//
//#include "log.h"
//#include "singleton.h"
//
//namespace ConfigSpace
//{
//	using namespace LogSpace;
//	using namespace SingletonSpace;
//
//	using std::dynamic_pointer_cast;
//	using std::exception;
//	using std::invalid_argument;
//
//	class ConfigVarBase
//	{
//	public:
//		ConfigVarBase(const string& name, const string& description = "")
//			:m_name(name),m_description(description){}
//		virtual ~ConfigVarBase(){}		//�������������Ӧ������Ϊ�麯��
//
//		//��ȡ������Ա
//		const string getName()const { return m_name; }
//		const string getDescription()const { return m_description; }
//
//		virtual string toString() = 0;	//���麯�����������ʵ��
//		virtual bool fromString(const string& val) = 0;		//���麯�����������ʵ��
//	protected:
//		string m_name;
//		string m_description;
//	};
//
//	template<class T>
//	class ConfigVar :public ConfigVarBase
//	{
//	public:
//		ConfigVar(const string& name, const T& default_value, const string& description = "")
//			:ConfigVarBase(name,description),m_value(default_value){}
//
//		string toString()override
//		{
//			try
//			{
//				return boost::lexical_cast<string>(m_value);
//			}
//			catch (exception& e)
//			{
//				//������־�¼�
//				//__FILE__���ص�ǰ�ļ����ļ������Դ�·������__LINE__���ص�ǰ��������;elapseΪ����ֵ
//				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
//				event->getSstream() << "ConfigVar::toString exception" << e.what()
//					<< "convert:" << typeid(m_value).name() << " to string";
//				//ʹ��LoggerManager������Ĭ��logger�����־
//				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
//			}
//			return "";
//		}
//		bool fromString(const string& val) override
//		{
//			try
//			{
//				m_value = boost::lexical_cast<T>(val);
//			}
//			catch (exception& e)
//			{
//				//������־�¼�
//				//__FILE__���ص�ǰ�ļ����ļ������Դ�·������__LINE__���ص�ǰ��������;elapseΪ����ֵ
//				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
//				event->getSstream() << "ConfigVar::toString exception" << e.what()
//					<< "convert:" << typeid(m_value).name();
//				//ʹ��LoggerManager������Ĭ��logger�����־
//				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
//			}
//		}
//		
//		void setValue(const T& value) { m_value = value; }
//		const T getValue()const { return m_value; }
//	private:
//		T m_value;
//	};
//
//
//
//	class Config
//	{
//	public:
//		template<class T>
//		static shared_ptr<ConfigVar<T>> Lookup(const string& name,
//			const T& default_value, const string& description = "")
//		{
//			auto temp = Lookup<T>(name);
//			if (temp)
//			{
//				//������־�¼�
//				//__FILE__���ص�ǰ�ļ����ļ������Դ�·������__LINE__���ص�ǰ��������;elapseΪ����ֵ
//				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
//				event->getSstream() << "Lookup name=" << name << "exists";
//				//ʹ��LoggerManager������Ĭ��logger�����־
//				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);
//
//				return temp;
//			}
//			else if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._0123456789") != string::npos)
//			{
//				//������־�¼�
//				//__FILE__���ص�ǰ�ļ����ļ������Դ�·������__LINE__���ص�ǰ��������;elapseΪ����ֵ
//				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
//				event->getSstream() << "Lookup name invalid ";
//				//ʹ��LoggerManager������Ĭ��logger�����־
//				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
//
//				throw invalid_argument(name);
//			}
//
//			shared_ptr<ConfigVar<T>> v(new ConfigVar<T>(name, default_value, description));
//			s_datas[name] = v;
//			return v;
//		}
//
//		template<class T>
//		static shared_ptr<ConfigVar<T>> Lookup(const string& name)
//		{
//			auto iterator = s_datas.find(name);
//			if (iterator == s_datas.end())
//			{
//				return nullptr;
//			}
//			return dynamic_pointer_cast<ConfigVar<T>>(iterator->second);
//		}
//	private:
//		static map<string, shared_ptr<ConfigVarBase>> s_datas;
//	};
//}
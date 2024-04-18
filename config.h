#pragma once
#include <exception>
#include <boost/lexical_cast.hpp>

#include "log.h"
#include "singleton.h"

namespace ConfigSpace
{
	using namespace LogSpace;
	using namespace SingletonSpace;

	using std::dynamic_pointer_cast;
	using std::exception;
	using std::invalid_argument;

	class ConfigVarBase
	{
	public:
		ConfigVarBase(const string& name, const string& description = "")
			:m_name(name),m_description(description){}
		virtual ~ConfigVarBase(){}		//父类的析构函数应该设置为虚函数

		//获取保护成员
		const string getName()const { return m_name; }
		const string getDescription()const { return m_description; }

		virtual string toString() = 0;	//纯虚函数，子类必须实现
		virtual bool fromString(const string& val) = 0;		//纯虚函数，子类必须实现
	protected:
		string m_name;
		string m_description;
	};

	template<class T>
	class ConfigVar :public ConfigVarBase
	{
	public:
		ConfigVar(const string& name, const T& default_value, const string& description = "")
			:ConfigVarBase(name,description),m_value(default_value){}

		string toString()override
		{
			try
			{
				return boost::lexical_cast<string>(m_value);
			}
			catch (exception& e)
			{
				//设置日志事件
				//__FILE__返回当前文件的文件名（自带路径），__LINE__返回当前代码行数;elapse为测试值
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
				event->getSstream() << "ConfigVar::toString exception" << e.what()
					<< "convert:" << typeid(m_value).name() << " to string";
				//使用LoggerManager单例的默认logger输出日志
				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
			}
			return "";
		}
		bool fromString(const string& val) override
		{
			try
			{
				m_value = boost::lexical_cast<T>(val);
			}
			catch (exception& e)
			{
				//设置日志事件
				//__FILE__返回当前文件的文件名（自带路径），__LINE__返回当前代码行数;elapse为测试值
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
				event->getSstream() << "ConfigVar::toString exception" << e.what()
					<< "convert:" << typeid(m_value).name();
				//使用LoggerManager单例的默认logger输出日志
				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);
			}
		}
		
		void setValue(const T& value) { m_value = value; }
		const T getValue()const { return m_value; }
	private:
		T m_value;
	};



	class Config
	{
	public:
		template<class T>
		static shared_ptr<ConfigVar<T>> Lookup(const string& name,
			const T& default_value, const string& description = "")
		{
			auto temp = Lookup<T>(name);
			if (temp)
			{
				//设置日志事件
				//__FILE__返回当前文件的文件名（自带路径），__LINE__返回当前代码行数;elapse为测试值
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
				event->getSstream() << "Lookup name=" << name << "exists";
				//使用LoggerManager单例的默认logger输出日志
				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::INFO, event);

				return temp;
			}
			else if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._0123456789") != string::npos)
			{
				//设置日志事件
				//__FILE__返回当前文件的文件名（自带路径），__LINE__返回当前代码行数;elapse为测试值
				shared_ptr<LogEvent> event(new LogEvent(__FILE__, __LINE__, 0, time(0)));
				event->getSstream() << "Lookup name invalid ";
				//使用LoggerManager单例的默认logger输出日志
				Singleton<LoggerManager>::GetInstance_shared_ptr()->getDefault_logger()->log(LogLevel::ERROR, event);

				throw invalid_argument(name);
			}

			shared_ptr<ConfigVar<T>> v(new ConfigVar<T>(name, default_value, description));
			s_datas[name] = v;
			return v;
		}

		template<class T>
		static shared_ptr<ConfigVar<T>> Lookup(const string& name)
		{
			auto iterator = s_datas.find(name);
			if (iterator == s_datas.end())
			{
				return nullptr;
			}
			return dynamic_pointer_cast<ConfigVar<T>>(iterator->second);
		}
	private:
		static map<string, shared_ptr<ConfigVarBase>> s_datas;
	};
}
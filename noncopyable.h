#pragma once

namespace NoncopyableSpace
{
	class Noncopyable
	{
	public:
		//默认构造函数
		Noncopyable() = default;
		//默认析构函数
		~Noncopyable() = default;
		//复制构造函数（禁用）
		Noncopyable(const Noncopyable&) = delete;
		//赋值运算符（禁用）
		Noncopyable& operator=(const Noncopyable&) = delete;
	};
}
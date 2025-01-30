#pragma once

namespace NoncopyableSpace
{
	// 不可复制类，作为基类被继承
	class Noncopyable
	{
	public:
		// 显式声明默认构造函数
		Noncopyable() = default;
		// 显式声明默认析构函数
		~Noncopyable() = default;
		// 复制构造函数（禁用）
		Noncopyable(const Noncopyable &) = delete;
		// 赋值运算符（禁用）
		Noncopyable &operator=(const Noncopyable &) = delete;
	};
}
#pragma once

namespace NoncopyableSpace
{
	class Noncopyable
	{
	public:
		//Ĭ�Ϲ��캯��
		Noncopyable() = default;
		//Ĭ����������
		~Noncopyable() = default;
		//���ƹ��캯�������ã�
		Noncopyable(const Noncopyable&) = delete;
		//��ֵ����������ã�
		Noncopyable& operator=(const Noncopyable&) = delete;
	};
}
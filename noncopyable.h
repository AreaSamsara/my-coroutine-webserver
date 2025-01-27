#pragma once

namespace NoncopyableSpace
{
	//���ɸ����࣬��Ϊ���౻�̳�
	class Noncopyable
	{
	public:
		//��ʽ����Ĭ�Ϲ��캯��
		Noncopyable() = default;
		//��ʽ����Ĭ����������
		~Noncopyable() = default;
		//���ƹ��캯�������ã�
		Noncopyable(const Noncopyable&) = delete;
		//��ֵ����������ã�
		Noncopyable& operator=(const Noncopyable&) = delete;
	};
}
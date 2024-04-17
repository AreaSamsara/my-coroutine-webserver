#pragma once
#include <memory>

namespace SingletonSpace 
{
    using std::shared_ptr;

    /**
     * @brief ����ģʽ��װ��
     * @details T ����
     *          X Ϊ�˴�����ʵ����Ӧ��Tag
     *          N ͬһ��Tag������ʵ������
     */
    template<class T, class X = void, int N = 0>
    class Singleton {
    public:
        /**
         * @brief ���ص�����ָ��
         */
        static T* GetInstance_normal_ptr() 
        {
            static T v;
            return &v;
            //return &GetInstanceX<T, X, N>();
        }
        /**
         * @brief ���ص�������ָ��
         */
        static shared_ptr<T> GetInstance_shared_ptr()
        {
            static shared_ptr<T> v(new T);
            return v;
            //return GetInstancePtr<T, X, N>();
        }
    };
}
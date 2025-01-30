#pragma once
#include <memory>

namespace SingletonSpace
{
    using std::shared_ptr;

    /**
     * @brief 单例模式封装类
     * @details T 类型
     *          X 为了创造多个实例对应的Tag
     *          N 同一个Tag创造多个实例索引
     */
    template <class T, class X = void, int N = 0>
    class Singleton
    {
    public:
        // 返回单例裸指针
        static T *GetInstance_normal_ptr()
        {
            static T v;
            return &v;
            // return &GetInstanceX<T, X, N>();
        }
        // 由于hook系统内部的不可名状bug，暂时禁用单例智能指针，所有单例对象均改为使用单例裸指针
        ////返回单例智能指针
        // static shared_ptr<T> GetInstance_shared_ptr()
        //{
        //     static shared_ptr<T> v(new T);
        //     return v;
        //     //return GetInstancePtr<T, X, N>();
        // }
    };
}
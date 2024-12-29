#pragma once
#include "../scheduler/scheduler.h"

namespace co
{

enum {
    opt_scheduler,
    opt_stack_size,
    opt_dispatch,
    opt_affinity,
};

template <int OptType>
struct __go_option;

template <>
struct __go_option<opt_scheduler>
{
    /* 调度器 Scheduler */
    Scheduler* scheduler_;
    /* 构造函数 */
    explicit __go_option(Scheduler* scheduler) : scheduler_(scheduler) {}
    explicit __go_option(Scheduler& scheduler) : scheduler_(&scheduler) {}
};

template <>
struct __go_option<opt_stack_size>
{
    /* 栈大小 Stack Size */
    std::size_t stack_size_;
    /* 构造函数 */
    explicit __go_option(std::size_t v) : stack_size_(v) {}
};

template <>
struct __go_option<opt_affinity>
{
    /* CPU亲和性 Affinity */
    bool affinity_;
    /* 构造函数 */
    explicit __go_option(bool affinity) : affinity_(affinity) {}
};

/* 协程 */
struct __go
{
    /* 构造函数 */
    __go(const char* file, int lineno)
    {
        scheduler_ = nullptr;
        opt_.file_ = file;
        opt_.lineno_ = lineno;
    }

    /* 重载-运算符 */
    template <typename Function>
    ALWAYS_INLINE void operator-(Function const& f)
    {
        if (!scheduler_) scheduler_ = Processer::GetCurrentScheduler();
        if (!scheduler_) scheduler_ = &Scheduler::getInstance();
        scheduler_->CreateTask(f, opt_);
    }

    /* 重载-运算符 */
    ALWAYS_INLINE __go& operator-(__go_option<opt_scheduler> const& opt)
    {
        scheduler_ = opt.scheduler_;
        return *this;
    }

    /* 重载-运算符 */
    ALWAYS_INLINE __go& operator-(__go_option<opt_stack_size> const& opt)
    {
        opt_.stack_size_ = opt.stack_size_;
        return *this;
    }

    /* 重载-运算符 */
    ALWAYS_INLINE __go& operator-(__go_option<opt_affinity> const& opt)
    {
        opt_.affinity_ = opt.affinity_;
        return *this;
    }

    /* 任务选项 */
    TaskOpt opt_;
    /* 调度器 */
    Scheduler* scheduler_;
};

//template <typename R>
//struct __async_wait
//{
//    R result_;
//    Channel<R> ch_;
//
//    __async_wait() : ch_(1) {}
//
//    template <typename F>
//    ALWAYS_INLINE R&& operator-(F const& fn)
//    {
//        g_Scheduler.GetThreadPool().AsyncWait<R>(ch_, fn);
//        ch_ >> result_;
//        return std::move(result_);
//    }
//};
//
//template <>
//struct __async_wait<void>
//{
//    Channel<void> ch_;
//
//    template <typename F>
//    ALWAYS_INLINE void operator-(F const& fn)
//    {
//        g_Scheduler.GetThreadPool().AsyncWait<void>(ch_, fn);
//        ch_ >> nullptr;
//    }
//};
//
//typedef	::co::Scheduler::TaskListener co_listener;

} //namespace co


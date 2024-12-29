#pragma once
#include "../common/config.h"
#include "../common/deque.h"
#include "../common/spinlock.h"
#include "../routine_sync/timer.h"
#include "../task/task.h"
#include "../debug/listener.h"
#include "processer.h"
#include <mutex>

namespace co {

struct TaskOpt
{
    /* CPU亲和性 */
    bool affinity_ = false;
    /* */
    int lineno_ = 0;
    /* 协程栈大小 */
    std::size_t stack_size_ = 0;
    /* 文件 */
    const char* file_ = nullptr;
};

// 协程调度器
// 负责管理1到N个调度线程, 调度从属协程.
// 可以调用Create接口创建更多额外的调度器
class Scheduler
{
    friend class Processer;

public:
    /* 单例模式，获取调度器实例 */
    ALWAYS_INLINE static Scheduler& getInstance();

    /* 创建调度器 */
    static Scheduler* Create();

    /* 创建协程 */
    void CreateTask(TaskF const& fn, TaskOpt const& opt);

    /* 当前是否处于协程中 */
    bool IsCoroutine();

    /* 是否没有协程可执行 */
    bool IsEmpty();

    // 启动调度器
    // @minThreadNumber : 最小调度线程数, 为0时, 设置为cpu核心数.
    // @maxThreadNumber : 最大调度线程数, 为0时, 设置为minThreadNumber.
    //          如果maxThreadNumber大于minThreadNumber, 则当协程产生长时间阻塞时,
    //          可以自动扩展调度线程数.
    void Start(int minThreadNumber = 1, int maxThreadNumber = 0);
    void goStart(int minThreadNumber = 1, int maxThreadNumber = 0);

    /* 最大线程数限制 */
    static const int s_ulimitedMaxThreadNumber = 40960;

    // 停止调度 
    // 注意: 停止后无法恢复, 仅用于安全退出main函数, 不保证终止所有线程.
    //       如果某个调度线程被协程阻塞, 必须等待阻塞结束才能退出.
    void Stop();

    /**使用独立的定时器线程 */
    void UseAloneTimerThread();

    /* 当前调度器中的协程数量 */
    uint32_t TaskCount();

    /* 当前协程ID, ID从1开始（不在协程中则返回0） */
    uint64_t GetCurrentTaskID();

    /* 当前协程切换的次数 */
    uint64_t GetCurrentTaskYieldCount();

    /* 设置当前协程调试信息, 打印调试信息时将回显 */
    void SetCurrentTaskDebugInfo(std::string const& info);

    /* 同步定时器 */
    typedef ::libgo::RoutineSyncTimer TimerType;

public:
    inline TimerType & GetTimer() { return timer_ ? *timer_ : StaticGetTimer(); }

    /* 是否停止 */
    inline bool IsStop() { return stop_; }

    /* 是否正在退出 */
    static bool& IsExiting();

private:
    /* 协程ID */
    using idx_t     = std::size_t;
    /* 激活协程队列 */
    using ActiveMap = std::multimap<std::size_t, idx_t>;
    /* 阻塞协程队列 */
    using BlockMap  = std::map<idx_t, std::size_t>;
    /* 构造函数 */
    Scheduler();
    /* 析构函数 */
    ~Scheduler();

    /* 禁用拷贝构造函数、移动构造函数、拷贝赋值函数、移动赋值函数 */
    Scheduler(Scheduler const&) = delete;
    Scheduler(Scheduler &&) = delete;
    Scheduler& operator=(Scheduler const&) = delete;
    Scheduler& operator=(Scheduler &&) = delete;

    /* 删除任务 */
    static void DeleteTask(RefObject* tk, void* arg);

    // 将一个协程加入可执行队列中
    /* 添加任务 */
    void AddTask(Task* tk);

    // dispatcher调度线程函数
    // 1.根据待执行协程计算负载, 将高负载的P中的协程steal一些给空载的P
    // 2.侦测到阻塞的P(单个协程运行时间超过阀值), 将P中的其他协程steal给其他P
    void DispatcherThread();

    void NewProcessThread();

    void DispatchBlocks(BlockMap &blockings,ActiveMap &actives);

    /* 负载均衡 */
    void LoadBalance(ActiveMap &actives,std::size_t activeTasks);
    
    /* 静态定时器 */
    TimerType & StaticGetTimer();

    // deque of Processer, write by start or dispatch thread
    /* 逻辑处理器Processer队列 */
    Deque<Processer*> processers_;

    /* 是否已经启动 */
    LFLock started_;

    /* 任务数量 */
    atomic_t<uint32_t> taskCount_{0};

    volatile uint32_t lastActive_ = 0;

    /* 定时器 */
    TimerType *timer_ = nullptr;
    
    /* 最小线程数量 */
    int minThreadNumber_ = 1;
    /* 最大线程数量 */
    int maxThreadNumber_ = 1;

    /* 调度线程 */
    std::thread dispatchThread_;

    /* 互斥锁 */
    std::mutex stopMtx_;

    /* 停止标志 */
    bool stop_ = false;
};

} //namespace co

#define g_Scheduler ::co::Scheduler::getInstance()

namespace co
{
    /* 单例模式，获取调度器实例 */
    ALWAYS_INLINE Scheduler& Scheduler::getInstance()
    {
        /* 静态变量 Scheduler 调度器 */
        static Scheduler obj;
        return obj;
    }

} //namespace co

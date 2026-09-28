# pthread 接口文档（中文）

## 1. 概述

POSIX Threads（pthread）是 Unix/Linux 平台上的线程标准接口，定义了一组用于创建、同步和管理线程的函数。

在 Linux 下，pthread 通常通过 `pthread.h` 头文件使用，并通过 `-pthread` 选项链接。

示例：

```c
#include <pthread.h>

int main(void) {
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, worker, NULL);
    if (ret != 0) {
        return 1;
    }
    pthread_join(tid, NULL);
    return 0;
}
```

编译命令：

```bash
gcc test.c -pthread -o test
```

---

## 2. 线程模型基础

### 2.1 线程与进程的区别

- 线程属于同一个进程。
- 线程共享进程的地址空间、全局数据、堆内存等资源。
- 每个线程有自己的栈空间。
- 线程之间通常通过互斥锁、条件变量、信号量等机制同步。

### 2.2 线程 ID

`pthread_t` 是线程标识符，类型通常是一个 opaque 类型（不透明类型），其意义仅在当前进程内唯一。

常见接口：

- `pthread_self()`：获取当前线程 ID
- `pthread_equal()`：比较两个线程 ID 是否相等

---

## 3. 常用 pthread 接口

### 3.1 创建线程

#### `pthread_create`

```c
int pthread_create(pthread_t *thread,
                   const pthread_attr_t *attr,
                   void *(*start_routine)(void *),
                   void *arg);
```

参数说明：

- `thread`：输出参数，保存新线程 ID
- `attr`：线程属性，通常传 `NULL` 表示使用默认属性
- `start_routine`：线程入口函数，签名必须为：

```c
void *func(void *arg);
```

- `arg`：传递给线程入口函数的参数

返回值：

- 成功返回 `0`
- 失败返回错误码（不是 `errno`，而是直接返回错误码）

注意：

- 线程入口函数必须返回 `void *`，通常最后返回 `NULL`
- 如果线程需要传递复杂参数，建议使用结构体并传入地址

#### 示例：

```c
#include <pthread.h>
#include <stdio.h>

void *worker(void *arg) {
    int *n = (int *)arg;
    printf("thread: %d\n", *n);
    return NULL;
}

int main(void) {
    pthread_t tid;
    int value = 42;
    pthread_create(&tid, NULL, worker, &value);
    pthread_join(tid, NULL);
    return 0;
}
```

---

### 3.2 退出线程

#### `pthread_exit`

```c
void pthread_exit(void *value_ptr);
```

作用：

- 终止当前线程
- `value_ptr` 可由 `pthread_join` 接收

示例：

```c
void *worker(void *arg) {
    int x = 100;
    pthread_exit((void *)(intptr_t)x);
}
```

`pthread_join` 可以获取该退出值。

---

### 3.3 等待线程结束

#### `pthread_join`

```c
int pthread_join(pthread_t thread, void **value_ptr);
```

作用：

- 等待指定线程结束
- 当线程调用 `pthread_exit` 或从入口函数返回时，`join` 才返回

参数：

- `thread`：要等待的线程 ID
- `value_ptr`：接收线程退出值的指针；通常传 `NULL` 表示不关心退出值

示例：

```c
void *worker(void *arg) {
    return (void *)123;
}

int main(void) {
    pthread_t tid;
    void *ret = NULL;

    pthread_create(&tid, NULL, worker, NULL);
    pthread_join(tid, &ret);
    printf("ret = %ld\n", (long)ret);
    return 0;
}
```

---

### 3.4 分离线程

#### `pthread_detach`

```c
int pthread_detach(pthread_t thread);
```

作用：

- 将线程标记为“分离态”
- 线程结束后自动回收资源，不需要调用 `pthread_join`

适用场景：

- 后台任务线程
- 生命周期独立于主线程

注意：

- 一旦线程被分离，就不能再 `join`
- 不能对同一个线程重复 detach

---

### 3.5 获取当前线程 ID

#### `pthread_self`

```c
pthread_t pthread_self(void);
```

作用：

- 返回当前线程 ID

示例：

```c
printf("thread id = %lu\n", (unsigned long)pthread_self());
```

#### `pthread_equal`

```c
int pthread_equal(pthread_t t1, pthread_t t2);
```

作用：

- 比较两个线程 ID 是否相等

---

## 4. 互斥锁（Mutex）

互斥锁用于保护共享资源，防止多个线程同时访问同一段临界区。

### 4.1 初始化互斥锁

```c
int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr);
```

### 4.2 锁定

```c
int pthread_mutex_lock(pthread_mutex_t *mutex);
```

### 4.3 解锁

```c
int pthread_mutex_unlock(pthread_mutex_t *mutex);
```

### 4.4 销毁

```c
int pthread_mutex_destroy(pthread_mutex_t *mutex);
```

示例：

```c
#include <pthread.h>

static int counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg) {
    pthread_mutex_lock(&lock);
    counter++;
    pthread_mutex_unlock(&lock);
    return NULL;
}
```

### 4.5 互斥锁类型

常见类型：

- `PTHREAD_MUTEX_NORMAL`
- `PTHREAD_MUTEX_RECURSIVE`
- `PTHREAD_MUTEX_ERRORCHECK`
- `PTHREAD_MUTEX_DEFAULT`

可通过属性初始化指定。

---

## 5. 条件变量（Condition Variable）

条件变量用于线程之间的等待与唤醒，通常与互斥锁配合使用。

### 5.1 初始化

```c
int pthread_cond_init(pthread_cond_t *cond,
                      const pthread_condattr_t *attr);
```

### 5.2 等待

```c
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
```

### 5.3 唤醒一个线程

```c
int pthread_cond_signal(pthread_cond_t *cond);
```

### 5.4 唤醒全部线程

```c
int pthread_cond_broadcast(pthread_cond_t *cond);
```

### 5.5 销毁

```c
int pthread_cond_destroy(pthread_cond_t *cond);
```

典型用法：

```c
pthread_mutex_lock(&mutex);
while (!ready) {
    pthread_cond_wait(&cond, &mutex);
}
pthread_mutex_unlock(&mutex);
```

---

## 6. 线程同步的典型模式

### 6.1 生产者-消费者模型

- 生产者线程将数据放入缓冲区
- 消费者线程从缓冲区取出数据
- 使用互斥锁保护缓冲区
- 使用条件变量通知状态变化

### 6.2 共享资源保护

对临界区使用互斥锁，例如：

- 全局计数器
- 队列
- 日志缓存
- 共享状态对象

---

## 7. 返回值与错误处理

### 7.1 pthread 接口的返回值约定

大多数 pthread 函数遵循以下约定：

- 成功：返回 `0`
- 失败：返回错误码

注意：

- pthread 函数通常不会设置 `errno`
- 需要读取函数返回值本身

### 7.2 常见错误码

常见错误包括：

- `EAGAIN`：资源暂时不足
- `EINVAL`：参数非法
- `EPERM`：权限不足
- `EDEADLK`：死锁

---

## 8. 线程函数签名和参数传递

线程入口函数的标准签名：

```c
void *worker(void *arg);
```

如果需要传多个参数，常用方法：

```c
typedef struct {
    int id;
    int value;
} Task;

void *worker(void *arg) {
    Task *task = (Task *)arg;
    printf("id=%d value=%d\n", task->id, task->value);
    return NULL;
}
```

调用方式：

```c
Task task = {1, 42};
pthread_create(&tid, NULL, worker, &task);
```

---

## 9. 常见坑与注意事项

### 9.1 线程入口函数必须是 `void *(*)(void *)`

线程入口函数的签名必须匹配，否则会出现未定义行为。

### 9.2 传参不能随便传 `NULL`

如果线程函数内部会解引用参数，则不能传 `NULL`。例如：

```c
void *worker(void *arg) {
    int *p = (int *)arg;
    printf("%d\n", *p);  // 如果 arg==NULL 会崩溃
    return NULL;
}
```

### 9.3 不要在线程函数中返回局部变量地址

例如：

```c
void *worker(void *arg) {
    int x = 10;
    return &x;  // 错误：返回局部变量地址
}
```

正确做法：

- 返回指向堆分配对象的地址
- 或者通过 `pthread_join` 接收退出值（通常是指针）

### 9.4 线程函数不能直接调用非静态成员函数

如果你在 C++ 中写类似：

```cpp
class ThreadPool {
public:
    void worker() {
        // ...
    }
};
```

`pthread_create` 需要一个静态函数或普通全局函数作为线程入口，不能直接传入类成员函数指针。

正确方式：

```cpp
class ThreadPool {
public:
    static void *threadEntry(void *arg) {
        auto *self = static_cast<ThreadPool *>(arg);
        self->worker();
        return nullptr;
    }

    void worker() {
        // 线程工作逻辑
    }
};
```

并在创建线程时传入 `this`：

```cpp
pthread_create(&tid, nullptr, ThreadPool::threadEntry, this);
```

---

## 10. 与当前线程池代码的对应关系

在你的线程池实现中，最关键的接口是：

- `pthread_create`
- `pthread_join`
- `pthread_detach`
- `pthread_mutex_lock`
- `pthread_mutex_unlock`
- `pthread_cond_wait`
- `pthread_cond_signal`

对于线程池典型结构：

- 任务队列用互斥锁保护
- 线程等待任务用条件变量
- 每个 worker 线程循环从队列取任务
- 完成后继续等待或退出

示例思路：

```cpp
class ThreadPool {
public:
    static void *workerEntry(void *arg) {
        auto *self = static_cast<ThreadPool *>(arg);
        while (true) {
            pthread_mutex_lock(&self->mutex_);
            while (self->tasks_.empty() && !self->stop_) {
                pthread_cond_wait(&self->cond_, &self->mutex_);
            }
            if (self->stop_ && self->tasks_.empty()) {
                pthread_mutex_unlock(&self->mutex_);
                break;
            }

            auto task = self->tasks_.front();
            self->tasks_.pop();
            pthread_mutex_unlock(&self->mutex_);

            task();
        }
        return nullptr;
    }

private:
    pthread_mutex_t mutex_;
    pthread_cond_t cond_;
};
```

---

## 11. 常用 pthread 接口速查

| 接口 | 作用 |
| --- | --- |
| `pthread_create` | 创建线程 |
| `pthread_exit` | 退出当前线程 |
| `pthread_join` | 等待线程结束 |
| `pthread_detach` | 分离线程 |
| `pthread_self` | 获取当前线程 ID |
| `pthread_equal` | 比较线程 ID |
| `pthread_mutex_init` | 初始化互斥锁 |
| `pthread_mutex_lock` | 加锁 |
| `pthread_mutex_unlock` | 解锁 |
| `pthread_mutex_destroy` | 销毁互斥锁 |
| `pthread_cond_init` | 初始化条件变量 |
| `pthread_cond_wait` | 等待条件 |
| `pthread_cond_signal` | 唤醒一个等待线程 |
| `pthread_cond_broadcast` | 唤醒全部等待线程 |
| `pthread_cond_destroy` | 销毁条件变量 |

---

## 12. 推荐阅读

- Linux man page: `man pthreads`
- Linux man page: `man pthread_create`
- Linux man page: `man pthread_join`
- Linux man page: `man pthread_mutex_lock`
- Linux man page: `man pthread_cond_wait`

官方文档地址：

- https://man7.org/linux/man-pages/man7/pthreads.7.html

---

## 13. 总结

pthread 是 Linux/Unix 中最基础也最重要的线程接口之一。核心能力包括：

- 创建与管理线程
- 线程终止与回收
- 互斥锁同步
- 条件变量同步
- 共享资源保护

熟练掌握这些接口后，可以构建线程池、工作队列、异步任务框架以及并发服务器等程序。

如果你想继续深入，我可以进一步补充：

1. C++ 中 `std::thread` 与 pthread 的对比
2. 线程池实现的完整 C++ 版本
3. 生产者/消费者队列的 pthread 示例
4. 结合你当前的 `thread_pool.h` 文件做逐行代码讲解

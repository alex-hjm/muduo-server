# 第一章 线程安全的对象生命期管理

## 1.1 当析构函数遇到多线程

当一个对象能被多个线程同时看到时，那么对象的销毁时机就会变得模糊不清，可能出现多种竞态条件（race condition）。  

### 线程安全的定义

- 多个线程同时访问时，其表现出正确的行为。
- 无论操作系统如何调度这些线程，无论这些线程的执行顺序如何交织（interleaving）。
- 调用端代码无须额外的同步或其他协调动作。

C++标准库里的大多数class都不是线程安全的，包括std:: string、std::vector、std::map等，因为这些class通常需要在外部加锁才能供多个线程同时访问。

### MutexLock与MutexLockGuard

- MutexLock封装临界区（critical section），这是一个简单的资源类，用RAII手法封装互斥器的创建与销毁。

  MutexLock一般是别的class的数据成员。

- MutexLockGuard封装临界区的进入和退出，即加锁和解锁。MutexLockGuard一般是个栈上对象，它的作用域刚好等于临界区域。

这两个class都不允许拷贝构造和赋值

一个线程安全的Counter示例 [代码]()

## 1.2 对象的创建很简单

对象构造要做到线程安全，唯一的要求是在构造期间不要泄露this指针，即

- 不要在构造函数中注册任何回调；
- 也不要在构造函数中把this传给跨线程的对象；
- 即便在构造函数的最后一行也不行。

原因：因为在构造函数执行期间对象还没有完成初始化，如果this被泄露（escape）给了其他对象（其自身创建的子对象除外），那么别的线程有可能访问这个半成品对象，这会造成难以预料的后果。

```c++
//don't do this
class Foo : public Observer
{
  public:
    Foo(Observer* s)
    {
        s->register_(this);//错误，非线程安全
    }
  virtual void update();
};
```

对象构造的正确方法：

```c++
//do this
class Foo : public Observer
{
  public:
    Foo();
    virtual void update();
    
    //另外定义一个函数，在构造之后执行回调函数的注册工作
	void observe(Observer* s)
    {
        s->register_(this);
    }
};
Foo* pFoo =new Foo;
0bservable* s = getSubject();
pFoo->observe(s);//二段式构造，或者直接写s->register_（pFoo);
```

二段式构造——即构造函数+initialize()——有时会是好办法，这虽然不符合C++教条，但是多线程下别无选择。既然允许二段式构造，那么构造函数不必主动抛异常，调用方靠initialize()的返回值来判断对象是否构造成功，这能简化错误处理。即使构造函数的最后一行也不要泄露this。

## 1.3 销毁太难

在多线程程序中，对象析构存在竞争条件，对一般成员函数而言，做到线程安全的办法是让它们顺次执行，而不要并发执行（关键是不要同时读写共享状态）——即让每个成员函数的临界区不重叠。而析构函数会把mutex成员变量销毁掉，使保护临界区的互斥器失效。

### mutex不是办法

mutex只能保证函数一个接一个地执行，考虑下面的代码，它试图用互斥锁来保护析构函数：

![](images\1.jpg)

此时，有A、B两个线程都能看到Foo对象x，线程A即将销毁x，而线程B正准备调用x->update()。

![](images\2.jpg)

尽管线程A在销毁对象之后把指针置为了NULL，尽管线程B在调用x的成员函数之前检查了指针x的值，但还是无法避免一种race condition：
1．线程A执行到了析构函数的(1)处，已经持有了互斥锁，即将继续往下执行。
2．线程B通过了if (x)检测，阻塞在(2)处。

### 作为数据成员的mutex不能保护析构

作为class数据成员的MutexLock只能用于同步本class的其他数据成员的读和写，它不能保护安全地析构。因为MutexLock成员的生命期最多与对象一样长，而析构动作可说是发生在对象身故之后（或者身亡之时）。

如果要同时读写一个class的两个对象，有潜在的死锁可能。比方说有swap()这个函数：

```c++
void swap(Counter& a, Counter& b)
{
    MutexLockGuard aLock(a.mutex_); //潜在死锁
    MutexLockGuard bLock(b.mutex_);
    int64_t value = b.value_;
    a.value_ = b.value_;
    b.value_ = value;
}
```

如果线程A执行swap(a, b);而同时线程B执行swap(b, a);，就有可能死锁。operator=()也是类似的道理。

```c++
Counter& Counter::operator=(const Counter& rhs)
{
  if (this == &rhs)
    return *this;

  MutexLockGuard myLock(mutex_);  //潜在死锁
  MutexLockGuard itsLock(rhs.mutex_);
  value_ = rhs.value_;//改成value_ = rhs.value()会死锁
  return *this;
}
```

一个函数如果要锁住相同类型的多个对象，为了保证始终按相同的顺序加锁，我们可以比较mutex对象的地址，始终先加锁地址较小的mutex。（也可以使用层次锁）

## 1.4 线程安全的Observer有多难

一个动态创建的对象是否还活着，光看指针是看不出来的。在面向对象程序设计中，对象的关系主要有三种：composition、aggregation、association。

- composition（组合／复合）关系：对象x的生命期由其唯一的拥有者owner控制，owner析构的时候会把x也析构掉。（直接数据成员、scoped_ptr成员、容器元素)

- association（关联／联系）关系：表示一个对象a用到了另一个对象b，调用了后者的成员函数。

  a持有b的指针（或引用），但是b的生命期不由a单独控制。

- aggregation（聚合）关系：形式上与association相同，如果b是动态创建的并在整个程序结束前有可能被释放，那么就会出现竞态条件。

一个简单的解决办法是：只创建不销毁。

——> 程序使用一个对象池来暂存用过的对象，下次申请新对象时，如果对象池里有存货，就重复利用现有的对象，否则就新建一个。对象用完了，不是直接释放掉，而是放回池子里。

问题：1. 对象池的线程安全 2. 全局共享数据引发的锁争用 3. 共享对象的类型多种，如何设计 4. 会不会造成内存泄漏与分片

如果对象注册了任何非静态成员函数回调，那么必然在某处持有了指向该对象的指针，这就暴露在了race condition之下。

一个典型的场景是Observer模式 [代码]()

## 1.5 原始指针有何不妥

指向对象的原始指针（raw pointer）是坏的，尤其当暴露给别的线程时。

**空悬指针**

有两个指针p1和p2，指向堆上的同一个对象Object，p1和p2位于不同的线程中。假设线程A通过p1指针将对象销毁了（尽管把p1置为了NULL），那p2就成了空悬指针。这是一种典型的C/C++内存错误。要想安全地销毁对象，要在别人（线程）都看不到的情况下销毁。（垃圾回收）

![](images\3.jpg)

## 1.6 shared_ptr/weak_ptr

hared_ptr是引用计数型智能指针，引用计数是自动化资源管理的常用手法，当引用计数降为0时，对象（资源）即被销毁。weak_ptr也是一个引用计数型智能指针，但是它不增加对象的引用次数，即弱（weak）引用。

- shared_ptr控制对象的生命期。shared_ptr是强引用，只要有一个指向x对象的shared_ptr存在，该x对象就不会析构。当指向对象x的最后一个shared_ptr析构或reset()的时候，x保证会被销毁。
- weak_ptr不控制对象的生命期，但是它知道对象是否还活着，如果对象还活着，那么它可以提升为有效的shared_ptr；如果对象已经死了，提升会失败，返回一个空的shared_ptr。（线程安全）

## 1.7 系统地避免各种指针错误

C++里可能出现的内存问题大致有这么几个方面：

1．缓冲区溢出（buffer overrun）：用`std::vector<char> /std::string`或自己编写Buffer class来管理缓冲区，自动记住用缓冲区的长度，并通过成员函数而不是裸指针来修改缓冲区。

2．空悬指针／野指针：用shared_ptr/weak_ptr

3．重复释放（double delete）：用scoped_ptr，只在对象析构的时候释放一次。

4．内存泄漏（memory leak）：用scoped_ptr，对象析构的时候自动释放内存。

5．不配对的new[]/delete：不配对的new[]/delete：把new[]统统替换为std::vector/scoped_array。

6．内存碎片（memory fragmentation）

scoped_ptr/shared_ptr/weak_ptr都是值语意，要么是栈上对象，或是其他对象的直接数据成员，或是标准库容器里的元素。

## 1.8 应用到Observer上

既然通过weak_ptr能探查对象的生死，那么Observer模式的竞态条件就很容易解决，只要让Observable保存`weak_ptr<Observer>`即可：[代码]()

把Observer*替换为weak_ptr<Observer>部分解决了Observer模式的线程安全，但还有以下几个疑点: 1. 侵入性 2. 不是完全线程安全 3. 锁争用 4. 死锁

## 1.9　再论shared_ptr的线程安全

shared_ptr本身不是100％线程安全的。它的引用计数本身是安全且无锁的，但对象的读写则不是，因为shared_ptr有两个数据成员，读写操作不能原子化。

- 一个shared_ptr对象实体可被多个线程同时读取
- 两个shared_ptr对象实体可以被两个线程同时写入，“析构”算写操作；
- 如果要从多个线程读写同一个shared_ptr对象，那么需要加锁

要在多个线程中同时访问同一个shared_ptr，正确的做法是用mutex保护：

```c++
MutexLock mutex; //No need for ReaderWriterLock
shared_ptr<Foo> globalPtr
//把 globalPtr 安全地传给doit()  
void doit(const boost::shared_ptr<Foo> & pFoo);
```

globalPtr能被多个线程看到，那么它的读写需要加锁。不必用读写锁，因为临界区非常小，用互斥锁也不会阻塞并发读。

```c++
   void read()
   {
      boost::shared_ptr<Foo> localPtr;
      {
         muduo::MutexLockGuard lock(mutex);
         localPtr=globalPtr;//read globalPtr
      }
      //读写localPtr也无需加锁
      doit(localPtr);
   }

   void write()
   {
      boost::shared_ptr<Foo> newPtr(new Foo);//对象创建在临界区之外,缩短临界区长度
      globalPtr.reset();//临界区外销毁对象
      {
         muduo::MutexLockGuard lock(mutex);
         globalPtr=newPtr;//write to globalPtr
      }
      //读写localPtr也无需加锁
      doit(newPtr);  
   }
   void destory()
   {
        boost::shared_ptr<Foo> localPtr;//
      {
         muduo::MutexLockGuard lock(mutex);
         globalPtr.swap(localPtr);
      }
   }
```

## 1.10 shared_ptr技术与陷阱

**意外延长对象的生命期**

shared_ptr是强引用，只要有一个指向x对象的shared_ptr存在，该对象就不会析构，延长对象的生命期。另外一个出错的可能是boost::bind，因为boost::bind会把实参拷贝一份，如果参数是shared_ptr，那么对象的生命期就不会短于boost::function对象：

```c++
class Foo
{
    void doit();
}
shared_ptr<Foo> pFoo(new Foo);
boost::function<void()> func = boost::bind(&Foo::doit,pFoo); //long life foo
```

**函数参数**

因为要修改引用计数（而且拷贝的时候通常要加锁），shared_ptr的拷贝开销比拷贝原始指针要高，但是需要拷贝的时候并不多。多数情况下它可以以const reference方式传递.

**析构动作在创建时被捕获**

- 虚析构不再是必需的
- shared_ptr<void>可以持有任何对象，而且能安全地释放
- shared_ptr对象可以安全地跨越模块边界
- 二进制兼容性
- 析构动作可以定制

**析构所在的线程**

当最后一个指向x的shared_ptr离开其作用域的时候，x会同时在同一个线程析构：

- 如果对象的析构比较耗时，那么可能会拖慢关键线程的速度
- 可以用一个单独的线程来专门做析构，通过一个BlockingQueue<shared_ptr<void> >把对象的析构都转移到那个专用线程，从而解放关键线程。

------

shared_ptr是管理共享资源的利器，需要注意避免循环引用，通常的做法是owner持有指向child的shared_ptr，child持有指向owner的weak_ptr。

## 1.11 对象池

假设有Stock类，代表一只股票的价格。每一只股票有一个唯一的字符串标识，比如Google的key是"NASDAQ:GOOG"，Stock对象能不断获取新价格。为了节省系统资源，同一个程序里边每一只出现的股票只有一个Stock对象，如果多处用到同一只股票，那么Stock对象应该被共享。如果某一只股票没有再在任何地方用到，其对应的Stock对象应该析构，以释放资源，这隐含了“引用计数”。

可以设计一个对象池StockFactory ：[代码]()

**enable_shared_from_this**

这是一个以其派生类为模板类型实参的基类模板，继承它，this指针就能变身为shared_ptr。

**弱回调**

把shared_ptr绑（boost::bind）到boost:function里，那么回调的时候StockFactory对象始终存在，是安全的。这同时也延长了对象的生命期，使之不短于绑得的boost:function对象。

借助shared_ptr和weak_ptr完美地解决了两个对象相互引用的问题。本节的StockFactory只有针对单个Stock对象的操作，如果程序需要遍历整个stocks_，稍不注意就会造成死锁或数据损坏

# 第二章 线程同步精要

并发编程有两种基本模型，一种是message passing（消息传递），另一种是shared memory（共享内存）。

线程同步的四项原则，按重要性排列：

1．首要原则是尽量最低限度地共享对象，减少需要同步的场合。

2．其次是使用高级的并发编程构件，如TaskQueue、Producer-Consumer Queue、CountDownLatch等等。

3．最后不得已必须使用底层同步原语（primitives）时，只用非递归的互斥器和条件变量，慎用读写锁，不要用信号量。

4．除了使用atomic整数之外，不自己编写lock-free代码，也不要用“内核级”同步原语

## 2.1 互斥器（mutex）

互斥器（mutex）是使用得最多的同步原语，粗略地说，它保护了临界区，任何一个时刻最多只能有一个线程在此mutex划出的临界区内活动。

主要原则有：

- 用RAII手法封装mutex的创建、销毁、加锁、解锁这四个操作。

- 只用非递归的mutex（即不可重入的mutex）。
- 不手工调用lock()和unlock()函数，一切交给栈上的Guard对象的构造和析构函数负责。Guard对象的生命期正好等于临界区。
- 在每次构造Guard对象的时候，思考一路上（调用栈上）已经持有的锁，防止因加锁顺序不同而导致死锁（deadlock）。

次要原则有：

- 不使用跨进程的mutex，进程间通信只用TCP sockets。
- 加锁、解锁在同一个线程，线程a不能去unlock线程b已经锁住的mutex（RAII自动保证）。
- 别忘了解锁（RAII自动保证）。
- 不重复解锁（RAII自动保证）。
- 必要的时候可以考虑用PTHREAD_MUTEX_ERRORCHECK来排错。

### 只使用非递归的mutex

mutex分为递归（recursive）和非递归（non-recursive）两种，这是POSIX的叫法，另外的名字是可重入（reentrant）与非可重入。

它们的唯一区别在于：同一个线程可以重复对recursive mutex加锁，但是不能重复对non-recursive mutex加锁。

在同一个线程里多次对non-recursive mutex加锁会立刻导致死锁，能够及早（在编码阶段）发现问题。而recursive mutex可能会隐藏代码里的一些问题。典型情况是你以为拿到一个锁就能修改对象了，没想到外层代码已经拿到了锁，正在修改（或读取）同一个对象呢。

[代码]()

non-recursive的优越性：把程序的逻辑错误暴露出来。死锁比较容易debug，把各个线程的调用栈打出来。或者可以用PTHREAD_MUTEX_ERRORCHECK一下子就能找到错误（前提是MutexLock带debug选项）。

如果一个函数既可能在已加锁的情况下调用，又可能在未加锁的情况下调用，那么就拆成两个函数：

1．跟原来的函数同名，函数加锁，转而调用第2个函数。
2．给函数名加上后缀WithLockHold，不加锁，把原来的函数体搬过来。

```c++
void post(const Foo& f)
{
  MutexLockGuard lock(mutex);
  postWithLockHold(f);//不用担心开销,编译器会自动内联的
}
void postWithLockHold(const Foo& f)
{
  assert(mutex.isLockedByThisThread());//防止数据损坏(误用不加锁版本)
  foos.push_back(f);
}
```

### 死锁

如果坚持只使用Scoped Locking，那么在出现死锁的时候很容易定位。

自己与自己死锁的例子: [代码]()

两个线程死锁的例子: [代码]()

main()线程是先调用Inventory::printAll再调用Request::print，而threadFunc()线程是先调用Request::～Request再调用Inventory::remove。这两个调用序列对两个mutex的加锁顺序正好相反，于是造成了经典的死锁。

![](images\7.jpg)

Inventory class的mutex的临界区由灰底表示，Request class的mutex的临界区由斜纹表示。一旦main()线程中的printAll()在另一个线程的～Request()和remove()之间开始执行，死锁已不可避免。

这里也出现了第1章所说的对象析构的race condition，即一个线程正在析构对象，另一个线程却在调用它的成员函数。

解决死锁的办法很简单，要么把print()移出printAll()的临界区，要么把remove()移出～Request()的临界区。

## 2.2 条件变量（condition variable）

互斥器（mutex）是加锁原语，用来排他性地访问共享数据，它不是等待原语。在使用mutex的时候，我们一般都会期望加锁不要阻塞，总是能立刻拿到锁。然后尽快访问数据，用完之后尽快解锁，这样才能不影响并发性和性能。

如果需要等待某个条件成立，我们应该使用条件变量（conditionvariable）。条件变量顾名思义是一个或多个线程等待某个布尔表达式为真，即等待别的线程“唤醒”它。

条件变量只有一种正确使用的方式，几乎不可能用错。对于wait端：

1．必须与mutex一起使用，该布尔表达式的读写需受此mutex保护。
2．在mutex已上锁的时候才能调用wait()。
3．把判断布尔条件和wait()放到while循环中。

```c++
muduo::MutexLock mutex;
muduo::Condition cond(mutex);
std::deque<int> queue ;
int dequeue()
{
    MutexLockGuard lock(mutex);
    while (queue.empty) //必须用循环,避免假唤醒;必须在判断之后再 wait()
    {
        cond.wait();//这一步会原子地unlock mutex并进入等待,不会与enqueue 死锁 
        //wait执行完毕时会自动重新加锁
    }
    assert(!queue.empty());
	int top = queue.front();
	queue.pop_front();
	return top;
}
```

对于signal/broadcast端：

1．不一定要在mutex已上锁的情况下调用signal（理论上）。
2．在signal之前一般要修改布尔表达式。
3．修改布尔表达式通常要用mutex保护（至少用作full memory barrier）。
4．注意区分signal与broadcast：“broadcast通常用于表明状态变化，signal通常用于表示资源可用。

```c++
void enqueue(int x)
{
    MutexLockGuard lock(mutex);
    queue.push_back(x);
    cond.notify() //可以移出临界区之外
}
```

【代码】

条件变量是非常底层的同步原语，很少直接使用，一般都是用它来实现高层的同步措施，如BlockingQueue<T>或CountDownLatch。【代码】

倒计时（CountDownLatch）是一种常用且易用的同步手段。它主要有两种用途：

- 主线程发起多个子线程，等这些子线程各自都完成一定的任务之后，主线程才继续执行。通常用于主线程等待多个子线程完成初始化。
- 主线程发起多个子线程，子线程都等待主线程，主线程完成其他一些任务之后通知所有子线程开始执行。通常用于多个子线程等待主线程发出“起跑”命令。

【代码】

------

互斥器和条件变量构成了多线程编程的全部必备同步原语，用它们即可完成任何多线程同步任务，二者不能相互替代。

## 2.3 不要用读写锁和信号量

读写锁（Readers-Writer lock，简写为rwlock）是个看上去很美的抽象，它明确区分了read和write两种行为。

- 从正确性方面来说，一种典型的易犯错误是在持有read lock的时候修改了共享数据。这种错误的后果跟无保护并发读写共享数据是一样的。
- 从性能方面来说，读写锁不见得比普通mutex更高效。

- reader lock可能允许提升（upgrade）为writer lock，也可能不允许提升。可能造成程序崩溃或者死锁。

- 通常reader lock是可重入的，writer lock是不可重入的。但是为了防止writer饥饿，writer lock通常会阻塞后来的reader lock，因此reader lock在重入的时候可能死锁。
- 在追求低延迟读取的场合也不适用读写锁
- 如果确实对并发读写有极高的性能要求，可以考虑read-copy-update

------

信号量（Semaphore）：条件变量配合互斥器可以完全替代其功能，而且更不易用错。信号量的另一个问题在于它有自己的计数值，而通常我们自己的数据结构也有长度值，这就造成了同样的信息存了两份，需要时刻保持一致，这增加了程序员的负担和出错的可能。如果要控制并发度，可以考虑用muduo::ThreadPool。

## 2.4 封装MutexLock、MutexLockGuard、Condition

【代码MutexLockk】【代码MutexLockGuard】

MutexLock的附加值在于提供了isLockedByThisThread()函数，用于程序断言。

【代码Condition】

boost::thread 同步原语的庞杂设计：

- Concept有四种Lockable、TimedLockable、SharedLockable、UpgradeLockable。
- Lock有六种：lock_guard、unique_lock、shared_lock、upgrade_lock、upgrade_to_unique_lock、scoped_try_lock。
- Mutex有七种：mutex、try_mutex、timed_mutex、recursive_mutex、recursive_try_mutex、recursive_timed_mutex、shared_mutex。

如果一个class要包含MutexLock和Condition，请注意它们的声明顺序和初始化顺序，mutex_ 应先于condition_ 构造，并作为后者的构造参数：

【代码CountDownLatch】

## 2.5 线程安全的Singleton实现

【代码Singleton】

使用方法也很简单：

```c++
Foo& foo = Singleton<Foo>::instance
```

这个Singleton通过atexit(3)提供了销毁功能。另外，这个Singleton只能调用默认构造函数，如果用户想要指定T的构造方式，我们可以用模板特化。

## 2.6 sleep(3)不是同步原语

sleep不具备memory barrier(内存栅障)语义，它不能保证内存的可见性.

生产代码中线程的等待可分为两种：

- 一种是等待资源可用（要么等在select/poll/epoll_wait上，要么等在条件变量上）；
- 一种是等着进入临界区（等在mutex上）以便读写共享数据。后一种等待通常极短，否则程序性能和伸缩性就会有问题。

如果需要等待一段已知的时间，应该往eventloop里注册一个timer，然后在timer的回调函数里接着干活，因为线程是个珍贵的共享资源，不能轻易浪费（阻塞也是浪费）。

如果等待某个事件发生，那么应该采用条件变量或IO事件回调，不能用sleep来轮询。

## 2.7 借shared_ptr实现copy-on-write

解决2.1的几个未决问题：

- post()和traverse()死锁。
- 把Request::print()移出Inventory::printAll()临界区。
- 解决Request对象析构的race condition。
- 用普通mutex替换读写锁

解决办法都基于同一个思路，那就是用shared_ptr来管理共享数据。原理如下：

- shared_ptr是引用计数型智能指针，如果当前只有一个观察者，那么引用计数的值为1。
- 对于write端，如果发现引用计数为1，这时可以安全地修改共享对象，不必担心有人正在读它。
- 对于read端，在读之前把引用计数加1，读完之后减1，这样保证在读的期间其引用计数大于1，可以阻止并发写。

【代码CopyOnWrite_test】

关键看write端的post()该如何写：如果g_foos.unique()为true，我们可以放心地在原地修改FooList。如果g_foos.unique()为false，说明这时别的线程正在读取FooList，我们不能原地修改，而是复制一份，在副本上修改。这样就避免了死锁。

```c++
void post(const Foo& f)
{
  printf("post\n");
  MutexLockGuard lock(mutex);
  if (!g_foos.unique())//说明别的线程正在读取FooList
  {
    g_foos.reset(new FooList(*g_foos));//不能原地修改（避免死锁）
    printf("copy the whole list\n");
  }
  assert(g_foos.unique());
  g_foos->push_back(f);
}
```

注意这里临界区包括整个函数，其他写法都是错的。

```c++
//错误一︰直接修改 g_foos所指的FooList
void post(const Foo& f)
{
  MutexLockGuard lock(mutex);
  g_foos->push_back(f);
}
//错误二︰试图缩小临界区．把 copying移出临界区
void post(const Foo& f)
{
  FooListPtr newFoos(new FooList(*g_foos));
  g_foos->push_back(f);
  MutexLockGuard lock(mutex);  
  g_foos = newFoos;//或者g_foos.swap(newFoos);
}
//错误三∶把临界区拆成两个小的，把 copying放到临界区之外
void post(const Foo& f)
{
  FooListPtr oldFoos;
  {
      MutexLockGuard lock(mutex);
      oldFoos=g_foos;
  }
  FooListPtr newFoos(new FooList(*oldFoos));
  g_foos->push_back(f);
  MutexLockGuard lock(mutex);  
  g_foos = newFoos;//或者g_foos.swap(newFoos);
}

```

------

解决把Request::print()移出Inventory::printAll()临界区有两个做法。

- 其一很简单，把requests_复制一份，在临界区之外遍历这个副本。它复制了整个std::set中的每个元素，开销可能会比较大。
- 第二种做法的要点是用shared_ptr管理std::set，在遍历的时候先增加引用计数，阻止并发修改。

【代码RequestInventory_test】

------

用普通mutex替换读写锁的一个例子

一个多线程的C++程序，24h x 5.5d运行。有几个工作线程ThreadWorker{0, 1, 2, 3}，处理客户发过来的交易请求；另外有一个背景线程ThreadBackground，不定期更新程序内部的参考数据。这些线程都跟一个hash表(std::map替代)打交道，工作线程只读，背景线程读写，必然要用到一些同步机制，防止数据损坏。

最简单的同步办法是用读写锁：工作线程加读锁，背景线程加写锁。但是读写锁的开销比普通mutex要大，而且是写锁优先，会阻塞后面的读锁。利用shared_ptr和非重入mutex实现同步，就不必用读写锁，这能降低工作线程延迟。

【代码Customer】

# 第三章 多线程服务器的适用场合与常用编程模型
## 3.1 进程与线程
“进程（process）”是操作里最重要的两个概念之一（另一个是文件），粗略地讲，一个进程是“内存中正在运行的程序”。

线程的特点是共享地址空间，从而可以高效地共享数据。一台机器上的多个进程能高效地共享代码段（操作系统可以映射为同样的物理内存），但不能共享数据。

## 3.2 单线程服务器的常用编程模型
在高性能的网络程序中，使用得最为广泛的恐怕要数“non-blocking IO＋IO multiplexing”这种模型，即Reactor模式。

在“non-blocking IO＋IO multiplexing”这种模型中，程序的基本结构是一个事件循环（event loop），以事件驱动（event-driven）和事件回调的方式实现业务逻辑：
``` c++
while (!done)
{
  int timeout_ms = max (1000,getNextTimedCallback();
  int retval = ::poll(fds,nfds,timeout_ms) ;
  if (retval<0) {
    //处理错误,回调用户的 error handler
  }else {
    //处理到期的timers，回调用户的 timer handler
    if(retval >0) {
      //处理IO 事件,回调用户的I0 event handler
    }
  }
}
```
这里select(2)/poll(2)有伸缩性方面的不足，Linux下可替换为epoll(4)，其他操作系统也有对应的高性能替代品。

优点：编程不难，效率也不错。不仅可以用于读写socket，连接的建立（connect(2)/accept(2)）甚至DNS解析都可以用非阻塞方式进行，以提高并发度和吞吐量（throughput），对于IO密集的应用是个不错的选择。     
缺点：它要求事件回调函数必须是非阻塞的。对于涉及网络IO的请求响应式协议，它容易割裂业务逻辑，使其散布于多个回调函数之中，相对不容易理解和维护。

## 3.3 多线程服务器的常用编程模型
大概有这么几种：      
1．每个请求创建一个线程，使用阻塞式IO操作。在Java 1.4引入
NIO之前，这是Java网络编程的推荐做法。可惜伸缩性不佳。     
2．使用线程池，同样使用阻塞式IO操作。与第1种相比，这是提高
性能的措施。      
3．使用non-blocking IO＋IO multiplexing。即Java NIO的方式。     
4．Leader/Follower等高级模式。  

在默认情况下，使用第3种，即non-blocking IO＋one loop per
thread模式来编写多线程C++网络服务程序。

### one loop per thread
此种模型下，程序里的每个IO线程有一个event loop（或者叫Reactor），用于处理读写和定时事件（无论周期性的还是单次的）     

这种方式的好处是：    
- 线程数目基本固定，可以在程序启动的时候设置，不会频繁创建
与销毁。
- 可以很方便地在线程间调配负载。
- IO事件发生的线程是固定的，同一个TCP连接不必考虑事件并
发。    

Eventloop代表了线程的主循环，需要让哪个线程干活，就把timer
或IOchannel（如TCP连接）注册到哪个线程的loop里即可。

对实时性有要求的connection可以单独用一个线程；数据量大的connection可以独占一个线程，并把数据处理任务分摊到另几个计算线程中（用线程池）；其他次要的辅助性connections可以共享一个线程。

### 线程池
对于没有IO而光有计算任务的线程，使用event loop有点浪费，可以用一种补充方案，即用blocking queue实现的任务队列（TaskQueue)：      
``` c++
typedef boost::function<void()> Functor;
BlockingQueue<Functor> taskQueue;//线程安全的阻塞队列
void workerThread()
{
  while (running)// running变量是个全局标志
  {
    Functor task = taskQueue.take(); //this block
    task();//在产品代码中需要考虑异常处理
  }
}
```
用这种方式实现线程池特别容易，以下是启动容量（并发数）为N的线程池：
``` c++
int N=num_of_computing_threads;
for(int i=0;i<N;++i)
{
  create_thread(&WorkerThread); //伪代码：启动线程
}
//使用
Foo foo; // Foo 有calc()成员函数
boost::function<void()> task =boost::bind(&Foo::calc,&Foo);
taskQueue.post(task);
```

除了任务队列，还可以用BlockingQueue<T>实现数据的生产者消费者队列，即T是数据类型9而非函数对象，queue的消费者(s)从中拿到数据进行处理.
[代码BlockingQueue]()
[代码BoundedBlockingQueue]()

### 推荐模式
总结起来，推荐的C++多线程服务端编程模式为：one (event) loop per thread+ thread pool。   
- event loop（也叫IO loop）用作IO multiplexing，配合non-blockingIO和定时器。
- thread pool用来做计算，具体可以是任务队列或生产者消费者队
列。

程序里具体用几个loop、线程池的大小等参数需要根据应用来设定，基本的原则是“阻抗匹配”，使得CPU和IO都能高效地运作。

## 3.4 进程间通信只用TCP
Linux下进程间通信（IPC）的方式有：    
- 匿名管道（pipe）
- 具名管道（FIFO）
- POSIX消息队列
- 共享内存
- 信号（signals）等等

同步原语（synchronization primitives）也很多：      
- 互斥器（mutex）
- 条件变量（condition variable）
- 读写锁（reader-writer lock）
- 文件锁（recordlocking）
- 信号量（semaphore）等等。

进程间通信首选Sockets（主要指TCP），其好处在于：
- 可以跨主机，具有伸缩性，可以把进程分散到同一局域网的多台机器上。
- 进程结束时操作系统会关闭所有文件描述符，不会留下垃圾，容易地恢复。
- TCP port由一个进程独占，可以防止程序重复启动。
- 两个进程通过TCP通信，如果一个崩溃了，操作系统会关闭连接，
另一个进程几乎立刻就能感知。
- TCP协议是可记录、可重现的
- TCP还能跨语言，服务端和客户端不必使用同一种语言。
- TCP连接是可再生的，连接的任何一方都可以退出再启动，重建连接之后就能继续工作。

### 分布式系统中使用TCP长连接通信
从宏观上看，一个分布式系统是由运行在多台机器上的多个进程组成的，进程之间采用TCP长连接通信。使用TCP长连接的好处有两点：      
- 一是容易定位分布式系统中的服务之间的依赖关系。
  >  只要在机器上运行netstat -tpna | grep :port就能立刻列出用到某服务的客户端地址。然后在客户端的机器上用netstat或lsof命令找出是哪个进程发起的连接
- 二是通过接收和发送队列的长度也较容易定位网络或程序故障。
  > 在正常运行的时候，netstat打印的Recv-Q和Send-Q都应该接近0，或者在0附近摆动。
  > - 如果Recv-Q保持不变或持续增加，则通常意味着服务进程的处理速度变慢，可能发生了死锁或阻塞。
  > - 如果Send-Q保持不变或持续增加，有可能是对方服务器太忙、来不及处理,也有可能是网络中间某个路由器或交换机故障造成丢包，甚至对方服务器掉线.

## 3.5 多线程服务器的适用场合
服务器开发: 跑在多核机器上的Linux用户态的没有用户界面的长期运行的网络应用程序，通常是分布式系统的组成部件。     
开发服务端程序的一个基本任务是处理并发连接，现在服务端网络编程处理并发连接主要有两种方式：
- 当“线程”很廉价时，一台机器上可以创建远高于CPU数目的“线程”。这时一个线程只处理一个TCP连接，通常使用阻塞IO。
- 当线程很宝贵时，一台机器上只能创建与CPU数目相当的线程。这时一个线程要处理多个TCP连接上的IO，通常使用非阻塞IO和IO multiplexing。

如果要在一台多核机器上提供一种服务或执行一个任务，可用的模式有：
1. 运行一个单线程的进程: 不可伸缩的，不能发挥多核机器的计算能力。
2. 运行一个多线程的进程: 多线程程序难写，而且与模式3相比并没有什么优势。
3. 运行多个单线程的进程: 目前公认的主流模式。它有以下两种子模式:
   - 简单地把模式1中的进程运行多份
   - 主进程+woker进程
4. 运行多个多线程的进程: 不但没有结合2和3的优点，反而汇聚了二者的缺点。

### 必须用单线程的场合
有两种场合必须使用单线程：    
1. 程序可能会fork(2)；
2. 限制程序的CPU占用率。  

一个程序fork(2)之后一般有两种行为：
1. 立刻执行exec()，变身为另一个程序。例如shell和inetd；
2. 不调用exec()，继续运行当前程序。要么通过共享的文件描述符与父进程通信，协同完成任务；要么接过父进程传来的文件描述符,独立完成工作.

### 单线程程序的优缺点
优点：    
- 结构简单（一个基于IO multiplexing的event loop）
缺点：
- Event loop是非抢占的，可能发生了优先级反转，这个缺点可以用多线程来克服，这也是多线程的主要优势。

### 适用多线程程序的场景
多线程的适用场景是：提高响应速度，让IO和“计算”相互重叠，降低延时。虽然多线程不能提高绝对性能，但能提高平均响应性能。

一个程序要做成多线程的，大致要满足：
- 有多个CPU可用。单核机器上多线程没有性能优势。
- 线程间有共享数据，即内存中的全局状态。
- 共享的数据是可以修改的，而不是静态的常量表。如果数据不能
修改，那么可以在进程间用shared memory。
- 提供非均质的服务。即，事件的响应有优先级差异，我们可以用
专门的线程来处理优先级高的事件。防止优先级反转。
- 程序要有相当的计算量（减少时延，提高吞吐量）
- 利用异步操作。
- 能扩容。一个好的多线程程序应该能享受增加CPU数目带来的
好处
- 具有可预测的性能。随着负载增加，性能缓慢下降，超过某个临
界点之后会急速下降。线程数目一般不随负载变化。
- 多线程能有效地划分责任与功能，让每个线程的逻辑比较简单，
任务单一，便于编码。 

**例子一：机群管理软件**   
假设要管理一个Linux服务器机群，这个机群里有8个计算节点，1
个控制节点。机器的配置都是一样的，双路四核CPU，千兆网互联。这个软件由3个程序组成：
- 运行在控制节点上的master，这个程序监视并控制整个机群的状
态。
- 运行在每个计算节点上的slave，负责启动和终止job，并监控本
机的资源。
- 供最终用户使用的client命令行工具，用于提交job。

slave是个“看门狗进程”，它会启动别的job进程，因此必须是个单线程程序。另外它不应该占用太多的CPU资源，这也适合单线程模型。 

master应该是个模式2的多线程程序（10个线程）：
- 4个用于和slaves通信的IO线程：处理8个TCP connections能有效地降低延迟
- 1个logging线程：异步地往本地硬盘写log
- 1个数据库IO线程：读写数据库
- 2个和clients通信的IO线程：用多线程也能降低客户响应时间
- 1个主线程，用于做些背景工作，比如job调度
- 1个pushing线程，用于主动广播机群的状态：这样用户不用主动轮询

**例子二：TCP聊天服务器**    
服务的特点是并发连接之间有数据交换，从一个连接收到的数据要转发给其他多个连接。

不能按模式3的做法，把多个连接分到多个进程中分别处理（这会带来复杂的进程间通信），而只能用模式1或者模式2。

如果功能进一步复杂化，加上关键字过滤、黑名单、防灌水等等功
能，甚至要给聊天内容自动加上相关连接，每一项功能都会占用CPU资
源。这时就要考虑模式2了。

一个多线程服务程序中的线程大致可分为3类：
- IO线程，这类线程的主循环是IO multiplexing，阻塞地等在
select/poll/epoll_wait系统调用上。
- 计算线程，这类线程的主循环是blockingqueue，阻塞地等在
condition variable上。（一般位于thread pool中）
- 第三方库所用的线程，比如logging，又比如database
connection。

### 3.6 “多线程服务器的适用场合”例释

**1．Linux能同时启动多少个线程？**    

对于32-bit Linux，一个进程的地址空间是4GiB，其中用户态能访问3GiB左右，而一个线程的默认栈（stack）大小是10MB，心算可知，一个进程大约最多能同时启动300个线程。对于64-bit系统，线程数目可大大增加。

**2．多线程能提高并发度吗？**   

如果指的是“并发连接数”，则不能。    
假如单纯采用thread per connection的模型，那么并发
连接数最多300。
采用one loop per thread，单个event loop处理1万个并发长连接并不罕见，一个multi loop的多线程程序应该能轻松支持5万并发链接。  
小结：thread per connection不适合高并发场合，其可伸缩性不佳。one loop per thread的并发度足够大，且与CPU数目成正比。

**3．多线程能提高吞吐量吗？**   

对于计算密集型服务，不能。    
如果用thread per request的模型，每个客户请求用一个线程去处理，那么当并发请求数大于某个临界值T′时，吞吐量反而会下降，因为线程多了以后上下文切换的开销也随之增加。    
为了在并发请求数很高时也能保持稳定的吞吐量，我们可以用线程池，线程池的大小应该满足“阻抗匹配原则”。    
线程池也不是万能的，如果响应一次请求需要做比较多的计算那么用线程池是合理的，能简化编程。如果在一次请求响应中，主要时间是在等待IO，那么为了进一步提高吞吐量，往往要用其他编程模型，比如Proactor。

**4. 多线程能降低响应时间吗？**   

如果设计合理，充分利用多核资源的话，可以。在突发（burst）请
求时效果尤为明显。    
例一：多线程处理输入    
例二：多线程分担负载

**5. 多线程程序如何让IO和“计算”相互重叠，降低延迟？**

基本思路是，把IO操作（通常是写操作）通过BlockingQueue交给
别的线程去做，自己不必等待。    
例一：日志
> 单独用一个logging线程，负责写磁盘文件，通过一个或多个BlockingQueue对外提供接口。 别的线程要写日志的时候，先把消息（字符串）准备好，然后往queue里一塞就行，基本不用等待。这样服务线程的计算就和logging线程的磁盘IO相互重叠，降低了服务线程的响应时间。   

例二：memcached客户端
> 用memcached来保存用户最后发帖的时间:     
如果用同步IO，会增加延迟,利用多线程：对于set操作，调用方只要把key和value准备好，调用一下asyncSet()函数，把数据往BlockingQueue上一放就能立即返回，延迟很小。剩下的事就留给memcached客户端的线程去操心，而服务线程不受阻碍。

**6. 为什么第三方库往往要用自己的线程？**    
event loop模型没有标准实现。如果自己写代码，尽可以按所用
Reactor的推荐方式来编程。但是第三方库不一定能很好地适应并融入这个event loop framework，有时需要用线程来做一些串并转换。   
例一：libmemcached只支持同步操作    
例二：MySQL的官方C API不支持异步操作

**7. 什么是线程池大小的阻抗匹配原则？**   
如果池中线程在执行任务时，密集计算所占的时间比重为P（0＜
P≤1），而系统一共有C个CPU，为了让这C个CPU跑满而又不过载，线程池大小的经验公式T＝C/P。

**8．除了推荐的Reactor＋thread poll，还有别的non-trivial多线程编程模型吗？**    
有，Proactor。如果一次请求响应中要和别的进程打多次交道，那
么Proactor模型往往能做到更高的并发度。

**9．模式2和模式3a该如何取舍？**    
在其他条件相同的情况下，可以根据工作集（work set）的大小来取舍。工作集是指服务程序响应一次请求所访问的内存大小。

如果工作集较大，那么就用多线程，避免CPU cache换入换出，影
响性能；否则，就用单线程多进程，享受单线程编程的便利。

# 第四章 C++多线程系统编程精要

学习多线程编程面临的最大的思维方式的转变有两点：  
- 当前线程可能随时会被切换出去，或者说被抢占（preempt）了。
- 多线程程序中事件的发生顺序不再有全局统一的先后关系。

当线程被切换回来继续执行下一条语句（指令）的时候，全局数据
（包括当前进程在操作系统内核中的状态）可能已经被其他线程修改
了。

在多核系统中，多个线程是并行执行的，我们甚至没有统一的全局时钟来为每个事件编号。在没有适当同步的情况下，多个CPU上运行的多个线程中的事件发生先后顺序是无法确定的。

多线程程序的正确性必须通过适当的同步来让当前线程能看到其他线程的事件的结果：
``` c++
bool running=false; // 全局标志
void threadFunc()
{
  while(running)
  {
    //get task from queue
  }
}

void start()
{
  muduo::Thread t(threadFunc);
  t.start();
  running = true; //应该放在t.start()之前
}
```
## 4.1 基本线程原语的选用

11个最基本的Pthreads函数是：
- 2个：线程的创建和等待结束（join）。封装为muduo::Thread。
- 4个：mutex的创建、销毁、加锁、解锁。封装为muduo::MutexLock。
- 5个：条件变量的创建、销毁、等待、通知、广播。封装为muduo::Condition。

用这三样东西（thread、mutex、condition）可以完成任何多线程编程任务。当然我们一般也不会直接使用它们（mutex除外），而是使用更高层的封装，例如mutex::ThreadPool和mutex::CountDownLatch等。

Pthreads还提供了其他一些原语：
- pthread_once，封装为muduo::Singleton<T>。其实不如直接用全局变量。
- pthread_key*，封装为muduo::ThreadLocal<T>。可以考虑用
__thread替换之。不建议使用
- pthread_rwlock，读写锁通常应慎用。muduo没有封装读写锁，这
是有意的。
- sem_*，避免用信号量（semaphore）。它的功能与条件变量重
合，但容易用错。
- pthread_{cancel, kill}。程序中出现了它们，则通常意味着设计出了问题。

不推荐使用读写锁的原因是它往往造成提高性能的错觉（允许多个线程并发读），实际上在很多情况下，与使用最简单的mutex相比，它
实际上降低了性能。

## 4.2 C/C++系统库的线程安全性
线程的出现立刻给系统函数库带来了冲击：
- errno不再是一个全局变量，因为每个线程可能会执行不同的系统
库函数。
- 有些“纯函数”不受影响，例如memset/strcpy/snprintf等等
- 有些影响全局状态或者有副作用的函数可以通过加锁来实现线程
安全，例如malloc/free、printf、fread/fseek等等。
- 有些返回或使用静态空间的函数不可能做到线程安全，因此要提供另外的版本，例如asctime_r/ctime_r/gmtime_r、stderror_r、strtok_r等等。
- 传统的fork()并发模型不再适用于多线程程序

编写线程安全程序的一个难点在于线程安全是不可组合的（composable）。

尽管C++03标准没有明说标准库的线程安全性，但我们可以遵循一
个基本原则：凡是非共享的对象都是彼此独立的，如果一个对象从始至终只被一个线程用到，那么它就是安全的。另外一个事实标准是：共享的对象的read-only操作是安全的，前提是不能有并发的写操作。

C++的标准库容器和std::string都不是线程安全的，只有std::allocator保证是线程安全的。一方面的原因是为了避免不必要的性能开销，另一方面的原因是单个成员函数的线程安全并不具备可组合性（composable）。

C++标准库中的绝大多数泛型算法是线程安全的，因为这些都是无状态纯函数。只要输入区间是线程安全的，那么泛型函数就是线程安全的。

C++的iostream不是线程安全的，因为流式输出
``` c++
std::count << "Now is " <<time(NULL);
```
等价于两个函数调用
``` c++
std::cout.opreate<<("Now is ").operate<<(time(NULL));
```
即便ostream::operator<<()做到了线程安全，也不能保证其他线程不会在两次函数调用之前向stdout输出其他字符。   

对于“线程安全的stdout输出”这个需求，我们可以改用printf，以达到安全性和输出的原子性。但是这等于用了全局锁，任何时刻只能有一个线程调用printf，恐怕不见得高效。在多线程程序中高效的日志需要特殊设计。

## 4.3 Linux上的线程标识

POSIX threads库提供了pthread_self函数用于返回当前进程的标识符，其类型为pthread_t。pthread_t不一定是一个数值类型（整数或指针），也有可能是一个结构体，因此Pthreads专门提供了pthread_equal函数用于对比两个线程标识符是否相等。这就带来一系列问题，包括：
- 无法打印输出pthread_t，因为不知道其确切类型。也就没法在日志中用它表示当前线程的id。
- 无法比较pthread_t的大小或计算其hash值，因此无法用作关联容器的key。
- 无法定义一个非法的pthread_t值，用来表示绝对不可能存在的线程id，因此MutexLock class没有办法有效判断当前线程是否已经持有本锁。
- pthread_t值只在进程内有意义，与操作系统的任务调度之间无法建立有效关联。

因此，pthread_t并不适合用作程序中对线程的标识符。

在Linux上，建议使用gettid(2)系统调用的返回值作为线程id，这么做的好处有：
- 它的类型是pid_t，其值通常是一个小整数13，便于在日志中输出。
- 在现代Linux中，它直接表示内核的任务调度id，因此在/proc文件
系统中可以轻易找到对应项：/proc/tid或/prod/pid/task/tid。
- 在其他系统工具中也容易定位到具体某一个线程，例如在top(1)中
我们可以按线程列出任务。
- 任何时刻都是全局唯一的，并且由于Linux分配新pid采用递增轮回
办法，短时间内启动的多个线程也会具有不同的线程id。
- 0是非法值，因为操作系统第一个进程init的pid是1。

但是glibc并没有封装这个系统调用，需要我们自己实现       
muduo::CurrentThread::tid()采取的办法是用__thread变量来缓存gettid(2)的返回值，这样只有在本线程第一次调用的时候才进行系统调用，以后都是直接从thread local缓存的线程id拿到结果，效率无忧。
具体代码见[CurrentThread]()和[Thread]()

## 4.4 线程的创建与销毁的守则
线程的创建和销毁是编写多线程程序的基本要素，线程的创建比销
毁要容易得多，只需要遵循几条简单的原则：
- 程序库不应该在未提前告知的情况下创建自己的“背景线程”。
  > 可能会导致高估系统的可用资源，结果处理关键任务不及时，达不到预设的性能指标。而且一旦程序中有不止一个线程，就很难安全地fork()了
- 尽量用相同的方式创建线程，例如muduo::Thread。
  > 这样容易在线程的启动和销毁阶段做一些统一的簿记工作，。也可以统计当前有多少活动线程15，进程一共创建了多少线程，每个线程的用途分别是什么。
- 在进入main()函数之前不应该启动线程。
  > 这会影响全局对象的安全构造。如果一个库需要创建线程，那么应该进入main()函数之后再调用库的初始化函数去做。
- 程序中线程的创建最好能在初始化阶段全部完成。
  > 避免因为负载急剧增加而导致机器失去正常响应

线程的销毁有几种方式：
- 自然死亡。从线程主函数返回，线程正常退出。
- 非正常死亡。从线程主函数抛出异常或线程触发segfault信号等非
法操作
- 自杀。在线程中调用pthread_exit()来立刻退出线程。
- 他杀。其他线程调用pthread_cancel()来强制终止某个线程。

线程正常退出的方式只有一种，即自然死亡。任何从外部强行终止
线程的做法和想法都是错的。

如果确实需要强行终止一个耗时很长的计算任务，而又不想在计算
期间周期性地检查某个全局退出标志，那么可以考虑把那一部分代码
fork()为新的进程，这样杀（kill(2)）一个进程比杀本进程内的线程要安全得多。

一般而言，会让Thread对象的生命期长于线程，然后通过Thread::join()来等待线程结束并释放线程资源。如果Thread对象的生命期短于线程，那么就没有机会释放pthread_t了。muduo::Thread没有提供detach()成员函数。

### exit(3)在C++中不是线程安全的
exit(3)函数在C++中的作用除了终止进程，还会析构全局对象和已
经构造完的函数静态对象。这有潜在的死锁可能，考虑下面这个例子。
``` c++
void someFunctionMayCallExit()
{
  exit(1);
}
class Global0bject // : boost : :noncopyablei
{
  public:
    void doit()
    {
      MutexLockGuard lock(mutex_);
      someFunctionMayCallExit();
    }
    ~Global0bject()
    {
      printf(" G1obal0bject:-Global0bject\n");
      MutexLockGuard lock (mutex_);//此此处发生死锁
      // clean up
      printf("Global0bject:~Global0bject cleanning\n");}
  }
  private:
    MutexLock mutex;
}

Global0bject g_obj;

int main(
{
  g_obj.doit();
}
```
GlobalObject::doit()函数辗转调用了exit()，从而触发了全局对象g_obj的析构。GlobalObject的析构函数会试图加锁mutex_，而此时mutex_已经被GlobalObject::doit()锁住了，于是造成了死锁。

## 4.5 善用__thread关键字
__thread是GCC内置的线程局部存储设施（thread local storage）。它的实现非常高效，比pthread_key_t快很多。

__thread使用规则：只能用于修饰POD类型，不能修饰class类型，因为无法自动调用构造函数和析构函数。__thread可以用于修饰全局变量、函数内的静态变量，但是不能用于修饰函数的局部变量或者class的普通成员变量。另外，__thread变量的初始化只能用编译期常量。例如：
``` c++
__thread string t_obj1(" Chen Shuo");//错误，不能调用对象的构造函数
thread string* t_obj2 = new string;//错误，初始化必须用编译期常量
__thread string* t_obj3 = NULL;//正确，但是需要手工初始化并销毁对象
```

## 多线程与IO
操作文件描述符的系统调用本身是线程安全的，不用担心多个线程同时操作文件描述符会造成进程崩溃或内核崩溃。

但是，多个线程同时操作同一个socket文件描述符确实很麻烦，需要考虑的情况如下：
- 如果一个线程正在阻塞地read(2)某个socket，而另一个线程close(2)了此socket。
- 如果一个线程正在阻塞地accept(2)某个listening socket，而另一个
线程close(2)了此socket。
- 更糟糕的是，一个线程正准备read(2)某个socket，而另一个线程close(2)了此socket；第三个线程又恰好open(2)了另一个文件描述符，其fd号码正好与前面的socket相同。这样程序的逻辑就混乱了

如果给每个TCP socket配一把锁，让同时只能有一个线程读或写此socket，似乎可以“解决”问题，但这样还不如直接始终让同一个线程来操作此socket来得简单。

对于非阻塞IO，情况是一样的，而且收发消息的完整性与原子性几乎不可能用锁来保证，因为这样会阻塞其他IO线程。

用多个线程read或write同一个文件也不会提速。不仅如此，多个线程分别read或write同一个磁盘上的多个文件也不见得能提速。

--------------

为了简单起见，多线程程序应该遵循的原则是：
- 每个文件描述符只由一个线程操作，从而轻松解决消息收发的顺序性问题，也避免了关闭文件描述符的各种race condition。
- 一个线程可以操作多个文件描述符，但一个线程不能操作别的线程拥有的文件描述符。

epoll也遵循相同的原则 :
- 应该把对同一个epoll fd的操作（添加、删除、修改、等待）
都放到同一个线程中执行。（muduo::EventLoop::wakeup()）
  > 当然，一般的程序不会直接使用epoll、read、write，这些底层操作都由网络库代劳了。    
  这条规则有两个例外：对于磁盘文件，在必要的时候多个线程可以同时调用pread(2)/pwrite(2)来读写同一个文件；

## 4.7 用RAII包装文件描述符
在程序刚刚启动的时候，0是标准输入，1是标准输出，2是标准错误。这时如果我们新打开一个文件，它的文件描述符会是3。这种分配文件描述符的方式稍不注意就会造成串话。

在C++里解决这个问题的办法很简单：RAII   
用Socket对象包装文件描述符，所有对此文件描述符的读写操作都通过此对象进行，在对象的析构函数里关闭文件描述符。

为了防止访问失效的对象或者发生网络串话，muduo使shared_ptr来管理TcpConnection的生命期。这是唯一一个采用引用计数方式管理生命期的对象。

## 4.8 RAII与fork()
在编写C++程序的时候，我们总是设法保证对象的构造和析构是成
对出现的，否则就几乎一定会有内存泄漏。利用这一特性，我们可以用对象来包装资源，把资源管理与对象生命期管理统一起来（RAII）。但是，假如程序会fork()，这一假设就会被破坏了。
``` c++
int main()
{
  Foo foo; // 调用构造函数
  fork();  // fork 为两个进程
  foo.doit(); //在父子进程中都使用foo
  //析构函数会被调用两次，父进程和子进程各一次
}
```
fork()之后,子进程会继承地址空间和文件描述符，因此用于管理动态内存和文件描述符的RAII class都能正常工作。但是子进程不会继承：
- 父进程的内存锁，mlock(2)、mlockall(2)。
- 父进程的文件锁，fcntl(2)。
- 父进程的某些定时器，setitimer(2)、alarm(2)timer_create(2)等等。
- 其他

## 4.9 多线程与fork()

多线程与fork()的协作性很差。fork()一般不能在多线程程序中调用，因为Linux的fork()只克隆当前线程的thread of control，不克隆其他线程。fork()之后，除了当前线程之外，其他线程都消失了。

fork()之后子进程中只有一个线程，其他线程都消失了，这就造成
一个危险的局面。其他线程可能正好位于临界区之内，持有了某个锁，而它突然死亡，再也没有机会去解锁了。如果子进程试图再对同一个mutex加锁，就会立刻死锁。

## 4.10 多线程与signal
Linux/Unix的信号（signal）与多线程可谓是水火不容。在多线程时代，signal的语义更为复杂。信号分为两类：
- 发送给某一线程（SIGSEGV）
- 发送给进程中的任一线程（SIGTERM）

还要考虑掩码（mask）对信号的屏蔽等。特别是在signal handler中不能调用任何Pthreads函数，不能通过condition variable来通知其他线程。

在多线程程序中，使用signal的第一原则是不要使用signal。包括：
- 不要用signal作为IPC的手段，包括不要用SIGUSR1等信号来触发
服务端的行为。
- 也不要使用基于signal实现的定时函数，包括alarm/ualarm/setitimer/timer_create、sleep/usleep等等。
- 不主动处理各种异常信号（SIGTERM、SIGINT等等），只用默认
语义：结束进程。
- 在没有别的替代方法的情况下（比方说需要处理SIGCHLD信号），把异步信号转换为同步的文件描述符事件。

## 4.11 Linux新增系统调用的启示
凡是会创建文件描述符的syscall一般都增加了额外的flags参数，可以直接指定O_NONBLOCK和FD_CLOEXEC，例如：
- accept4 - 2.6.28
- eventfd2 - 2.6.27
- inotify_init1 - 2.6.27
- pipe2 - 2.6.27
- signalfd4 - 2.6.27
- timerfd_create - 2.6.25

O_NONBLOCK的功能是开启“非阻塞IO”，而文件描述符默认是阻
塞的。这些创建文件描述符的系统调用能直接设定O_NONBLOCK选
项。

另外，以下新系统调用可以在创建文件描述符时开启FD_CLOEXEC选项：
- dup3 - 2.6.27
- epoll_create1 - 2.6.27
- socket - 2.6.27
FD_CLOEXEC的功能是让程序exec()时，进程会自动关闭这个文件描述符。而文件描述默认是被子进程继承的

# 第五章 高效的多线程日志
“日志（logging）”有两个意思：
- 诊断日志（diagnostic log）即log4j、logback、slf4j、glog、g2log、log4cxx、log4cpp、log4cplus、Pantheios、ezlogger等常用日志库提供的日志功能。
- 交易日志（transaction log）即数据库的write-ahead log、文件系统的journaling等，用于记录状态变更，通过回放日志可以逐步恢复每一次修改之后的状态。

本章的“日志”是前一个意思，即文本的、供人阅读的日志，通常用
于故障诊断和追踪（trace），也可用于性能分析。日志通常是分布式系统中事故调查时的唯一线索，用来追寻蛛丝马迹，查出元凶。

在服务端编程中，日志是必不可少的，在生产环境中应该做到“Log Everything All The Time”4。对于关键进程，日志通常要记录：
1. 收到的每条内部消息的id（还可以包括关键字段、长度、hash
等）；
2. 收到的每条外部消息的全文；
3. 发出的每条消息的全文，每条消息都有全局唯一的id；
4. 关键内部状态的变更，等等。

每条日志都有时间戳，这样就能完整追踪分布式系统中一个事件的
来龙去脉。

一个日志库大体可分为前端（frontend）和后端（backend）两部分：
- 前端是供应用程序使用的接口（API），并生成日志消息（log
message）；
- 后端则负责把日志消息写到目的地（destination）。

这两部分的接口有可能简单到只有一个回调函数：
``` c++
void  output(const char* message,int len);
```
其中的message字符串是一条完整的日志消息，包含日志级别、时间戳、源文件位置、线程id等基本字段，以及程序输出的具体消息内容。

难点在于将日志数据从多个前端高效地传输到后端。这是一个典型的多生产者-单消费者问题:
- 对生产者（前端）而言，要尽量做到低延迟、低CPU开销、无阻塞；
- 对消费者（后端）而言，要做到足够大的吞吐量，并占用较少资源。

## 5.1 功能需求

常规的通用日志库如log4j/logback通常会提供丰富的功能，但这些功能不一定全都是必需的。  
1.  日志消息有多种级别（level），如TRACE、DEBUG、INFO、
WARN、ERROR、FATAL等。
2. 日志消息可能有多个目的地（appender），如文件、socket、
SMTP等。
3. 日志消息的格式可配置（layout），例如org.apache.log4j.PatternLayout。
4. 可以设置运行时过滤器（filter），控制不同组件的日志消息的
级别和目的地。

除了第一项之外，其余三项都是非必需的功能。

对于分布式系统中的服务进程而言，日志的目的地（destination）
只有一个：本地文件。日志文件的滚动（rolling）是必需的，这样可以简化日志归档（archive）的实现。rolling的条件通常有两个：
- 文件大小（例如每写满1GB就换下一个文件）
- 时间（例如每天零点新建一个日志文件，不论前一个文件有没有写满）。

一个典型的日志文件的文件名如下：
``` 
logfile_test.2012060-144022.hostname.3605.log
```
1. logfile_test是进程的名字。
2. 文件的创建时间（GMT时区）
3. 主机名称
4. 进程id
5. 统一的后缀名.log

往文件写日志的一个常见问题是，程序崩溃的办法：
- 其一是定期（默认3秒）将缓冲区内的日志消息flush到硬
盘；
- 其二是每条内存中的日志消息都带有cookie（或者叫哨兵值/sentry），其值为某个函数的地址，这样通过在core dump文件中查找cookie就能找到尚未来得及写入磁盘的消息。

muduo日志库的默认消息格式：
![](images\8.jpg)

## 5.2 性能需求
只有日志库足够高效，程序员才敢在代码中输出足够多的诊断信息，减小运维难度，提升效率。高效性体现在几方面：
- 每秒写几千上万条日志的时候没有明显的性能损失。
- 能应对一个进程产生大量日志数据的场景，例如1GB/min。
- 不阻塞正常的执行流程。
- 在多线程程序中，不造成争用（contention）。

为了实现这样的性能指标，muduo日志库的实现有几点优化措施值
得一提：
- 时间戳字符串中的日期和时间两部分是缓存的，一秒之内的多条
日志只需重新格式化微秒部分。
- 日志消息的前4个字段是定长的，因此可以避免在运行期求字符串
长度（不会反复调用strlen）。
- 线程id是预先格式化为字符串，在输出日志消息时只需简单拷贝几
个字节。
- 每行日志消息的源文件名部分采用了编译期计算来获得basename，避免运行期strrchr(3)开销。

## 5.3 多线程异步日志
多线程程序对日志库提出了新的需求：线程安全，即多个线程可以
并发写日志，两个线程的日志消息不会出现交织。

线程安全不难办到，简单的办法是用一个全局mutex保护IO，或者每个线程单独写一个日志文件，但这两种做法的高效性就堪忧了。前者会造成全部线程抢一个锁，后者有可能让业务线程阻塞在写磁盘操作上。

一个多线程程序的每个进程最好只写一个日志文件，这样分析日志更容易，不必在多个文件中跳来跳去。解决办法不难想到，用一个背景线程负责收集日志消息，并写入日志文件，其他业务线程只管往这个“日志线程”发送日志消息，这称为“异步日志”。

需要一个“队列”来将日志前端的数据传送到后端（日志线程），但这个“队列”不必是现成的`BlockingQueue<std::string>`，因为不用每次产生一条日志消息都通知（notify()）接收方。

-------

muduo日志库采用的是双缓冲（double buffering）技术，基本思路是准备两块buffer：A和B，前端负责往buffer A填数据（日志消息），后端负责将buffer B的数据写入文件。当buffer A写满之后，交换A和B，让后端将buffer A的数据写入文件，而前端则往buffer B填入新的日志消息，如此往复。

用两个buffer的好处是在新建日志消息的时候不必等待磁盘文件操作，也避免每条新日志消息都触发（唤醒）后端日志线程。

数据结构如下：[代码AsyncLogging.h]()
``` c++
typedef boost::ptr_vector<LargeBuffer> BufferVector;typedef BufferVector:: auto_type BufferPtr;
muduo:: MutexLock  mutex_;
muduo:: Condition  cond_;
BufferPtr currentBuffer_; //当前缓冲
BufferPtr nextBuffer_;    //预备缓冲
BufferVector buffers_;    //待写入文件的已填满的缓冲
```
- `LargeBuffer`类型是`FixedBuffer classtemplate`的一份具体实现（instantiation），其大小为4MB，可以存至少1000条日志消息。      
- boost::ptr_vector<T>::auto_type类型类似C++11中的std::unique_ptr，具备移动语义（move semantics），而且能自动管理对象生命期。
- mutex_用于保护后面的四个数据成员。
- buffers_存放的是供后端写入的buffer。

发送方代码：
``` c++
void AsyncLogging::append(const char* logline, int len)
{
  muduo::MutexLockGuard lock(mutex_);
  if (currentBuffer_->avail() > len) //剩余的空间足够大
  {
    //直接把日志消息拷贝（追加）到当前缓冲中
    currentBuffer_->append(logline, len);
  }
  else//当前缓冲已经写满
  {
    buffers_.push_back(std::move(currentBuffer_));

    if (nextBuffer_)
    {
      //把预备好的另一块缓冲移用为当前缓冲
      currentBuffer_ = std::move(nextBuffer_);//移动而非复制
    }
    else //allocate a new one
    {
      //如果前端写入速度太快，一下子把两块缓冲都用完了
      currentBuffer_.reset(new Buffer); // Rarely happens
    }
    //追加日志消息
    currentBuffer_->append(logline, len);
    cond_.notify();
  }
}
```
再来看接收方（后端）实现:
``` c++
void AsyncLogging::threadFunc()
{
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  BufferVector buffersToWrite;//reserve()从略
  while (running_)
  {
    muduo::MutexLockGuard lock(mutex_);
    if (buffers_.empty())  // unusual usage!
    {
      cond_.waitForSeconds(flushInterval_);
    }
    buffers_.push_back(std::move(currentBuffer_));//移动而非复制
    currentBuffer_ = std::move(newBuffer1);//移动而非复制
    buffersToWrite.swap(buffers_);//内部指针交换，而非复制
    if (!nextBuffer_)
    {
      nextBuffer_ = std::move(newBuffer2);//移动而非复制
    }
    // output buffersTowrite to file
    // re-fill newBuffer1 and newBuffer2
  }
  // flush output
}
```
# 第6章 muduo网络库简介

用Python实现了一个简单的“Hello”协议，客户端发来姓名，服务端返回问候语和服务器的当前时间:

hello-server.py
```python
#!/usr/bin/python

import socket, time

serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serversocket.bind(('', 8888))
serversocket.listen(5)

while True:
(clientsocket, address) = serversocket.accept() # 等待客户端连接
data = clientsocket.recv(4096) # 接收姓名
datetime = time.asctime()+'\n'
clientsocket.send('Hello ' + data) # 发回问候
clientsocket.send('My time is ' + datetime) # 发送服务器当前时间
clientsocket.close() # 关闭连接
```
hello-client.py
```python
# 省略import 等
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((sys.argv[1], 8888)) # 服务器地址由命令行指定
sock.send(os.getlogin() + '\n') # 发送姓名
message = sock.recv(4096) # 接收响应
print message # 打印结果
sock.close() # 关闭连接
```
在同一台机器上运行上面的服务端和客户端:
```shell
$ ./hello-client.py localhost
Hello schen
My time is Sun May 13 12:56:44 2012
```
但是连接同一局域网的另外一台服务器时，收到的数据是不完整的:
```shell
$ ./hello-client.py atom
Hello schen
```
原因是高级语言（Java、Python 等）的Sockets 库并没有对Sockets API 提供更高层的封装，直接用它编写网络程序很容易掉到陷阱里。

## 6.1 muduo库

**目录结构**

![](images\9.jpg)

**基础库**

![](images\10.jpg)

**网络核心库**    

muduo 是基于Reactor 模式的网络库，其核心是个事件循环EventLoop，用于响应计时器和IO 事件。muduo 采用基于对象（object-based）而非面向对象（objectoriented）的设计风格，其事件回调接口多以boost::function + boost::bind 表达，用户在使用muduo 的时候不需要继承其中的class。

灰底表示用户不可见的内部类。

![](images\11.jpg)

**网络附属库**     
在使用的时候需要链接相应的库，例如-lmuduo_http、-lmuduo_inspect 等等。HttpServer 和Inspector 暴露出一个
http 界面，用于监控进程的状态。

![](images\12.jpg)

**代码结构**

头文件明确分为客户可见和客户不可见两类

![](images\13.jpg)

对于使用muduo 库而言，只需要掌握5 个关键类：Buffer、
EventLoop、TcpConnection、TcpClient、TcpServer。

网络核心库的头文件包含关系，用户可见的为白底，用户不
可见的为灰底。

![](images\14.jpg)

**公开接口**
- Buffer ：数据的读写通过buffer 进行。用户代码不需要调用read(2)/write(2)，只需要处理收到的数据和准备好要发送的数据。
- InetAddress：封装IPv4 地址（end point），注意，它不能解析域名，只认IP 地址。因为直接用gethostbyname(3) 解析域名会阻塞IO 线程。
- EventLoop：事件循环（反应器Reactor），每个线程只能有一个EventLoop 实体，它负责IO 和定时器事件的分派。它用eventfd(2) 来异步唤醒，这有别于传统的用一对pipe(2) 的办法。它用TimerQueue 作为计时器管理，用Poller 作为IO multiplexing。
- EventLoopThread：启动一个线程，在其中运行EventLoop::loop()。
- TcpConnection：整个网络库的核心，封装一次TCP 连接，注意它不能发起连接。
- TcpClient：用于编写网络客户端，能发起连接，并且有重试功能。
- TcpServer：用于编写网络服务器，接受客户的连接。

> 在这些类中，TcpConnection 的生命期依靠shared_ptr 管理（即用户和库共同控制）。Buffer 的生命期由TcpConnection 控制。其余类的生命期由用户控制。Buffer和InetAddress 具有值语义，可以拷贝；其他class 都是对象语义，不可以拷贝。

**内部实现**

![](images\15.jpg)

- Channel 是selectable IO channel，负责注册与响应IO事件，注意它不拥有filedescriptor。它是Acceptor、Connector、EventLoop、TimerQueue、TcpConnection的成员，生命期由后者控制。
- Socket 是一个RAII handle，封装一个file descriptor，并在析构时关闭fd。它是Acceptor、TcpConnection 的成员，生命期由后者控制。EventLoop、TimerQueue也拥有fd，但是不封装为Socket class。
- SocketsOps 封装各种Sockets 系统调用。
- Poller 是PollPoller 和EPollPoller 的基类，采用“电平触发”的语意。它是EventLoop 的成员，生命期由后者控制。
- PollPoller 和EPollPoller 封装poll(2) 和epoll(4) 两种IO multiplexing 后端。poll 的存在价值是便于调试，因为poll(2) 调用是上下文无关的，用strace(1) 很容易知道库的行为是否正确。
- Connector 用于发起TCP 连接，它是TcpClient 的成员，生命期由后者控制。
- Acceptor 用于接受TCP 连接，它是TcpServer 的成员，生命期由后者控制。
- TimerQueue 用timerfd 实现定时，这有别于传统的设置poll/epoll_wait 的等待时长的办法。TimerQueue 用std::map 来管理Timer，常用操作的复杂度是O(logN)，N 为定时器数目。它是EventLoop 的成员，生命期由后者控制。
- EventLoopThreadPool 用于创建IO 线程池，用于把TcpConnection 分派到某个EventLoop 线程上。它是TcpServer 的成员，生命期由后者控制。

**例子**

![](images\16.jpg)
![](images\17.jpg)

**线程模型**

muduo 的线程模型符合one loop per thread + thread pool 模型。每个线程最多有一个EventLoop，每个TcpConnection 必须归某个EventLoop 管理，所有的IO会转移到这个线程。

一个file descriptor 只能由一个线程读写。TcpConnection 所在的线程由其所属的EventLoop 决定，这样可以很方便地把不同的TCP 连接放到不同的线程去，也可以把一些TCP 连接放到一个线程里。TcpConnection 和EventLoop 是线程安全的，可以跨线程调用。

TcpServer 直接支持多线程，它有两种模式：
- 单线程，accept(2) 与TcpConnection 用同一个线程做IO。
- 多线程，accept(2) 与EventLoop 在同一个线程，另外创建一个EventLoop-ThreadPool，新到的连接会按round-robin 方式分配到线程池中。

## 6.2 使用教程

muduo 只支持Linux 2.6.x 下的并发非阻塞TCP 网络编程，它的核心是每个IO线程一个事件循环，把IO 事件分发到回调函数上。

**思路**

- 注册一个收数据的回调函数，网络库收到数据会调用该函数，直接把数据提供给该函数，供该函数使用。
- 注册一个接受连接的回调函数，网络库接受了新连接会回调该函数，直接把新的连接对象传给该函数，供该函数使用。
- 需要发送数据的时候，只管往连接中写，网络库会负责无阻塞地发
送。
- 事件处理函数也应该避免阻塞，否则会让网络服务失去响应。

**TCP 网络编程最本质的三个半事件:**   
1. 连接的建立，包括服务端接受（accept）新连接和客户端成功发起（connect）连接。TCP 连接一旦建立，客户端和服务端是平等的，可以各自收发数据。
2. 连接的断开，包括主动断开（close、shutdown）和被动断开（read(2) 返回0）。
3. 消息到达，文件描述符可读。这是最为重要的一个事件，对它的处理方式决定了网络编程的风格（阻塞还是非阻塞，如何处理分包，应用层的缓冲如何设计，等等）。   
4. 消息发送完毕，这算半个。对于低流量的服务，可以不必关心这个事件；另外，这里的“发送完毕”是指将数据写入操作系统的缓冲区，将由TCP 协议栈负责数据的发送与重传，不代表对方已经收到数据。

muduo 的使用非常简单，不需要从指定的类派生，也不用覆写虚函数，只需要注册几个回调函数去处理前面提到的三个半事件就行了。

**echo 回显服务的实现: [代码]()**

1. 定义EchoServer class，不需要派生自任何基类。
``` c++
#include "muduo/net/TcpServer.h"

// RFC 862
class EchoServer
{
 public:
  EchoServer(muduo::net::EventLoop* loop,
             const muduo::net::InetAddress& listenAddr);

  void start();  // calls server_.start();

 private:
  void onConnection(const muduo::net::TcpConnectionPtr& conn);

  void onMessage(const muduo::net::TcpConnectionPtr& conn,
                 muduo::net::Buffer* buf,
                 muduo::Timestamp time);

  muduo::net::TcpServer server_;
};
```
在构造函数里注册回调函数。
```c++
EchoServer::EchoServer(muduo::net::EventLoop* loop,
                       const muduo::net::InetAddress& listenAddr)
  : server_(loop, listenAddr, "EchoServer")
{
  //基于事件编程
  server_.setConnectionCallback(//注册的事件处理函数
      std::bind(&EchoServer::onConnection, this, _1));
  server_.setMessageCallback(
      std::bind(&EchoServer::onMessage, this, _1, _2, _3));
}
```
2. 实现EchoServer::onConnection() 和EchoServer::onMessage()。
```c++
void EchoServer::start()
{
  server_.start();
}

void EchoServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
  LOG_INFO << "EchoServer - " << conn->peerAddress().toIpPort() << " -> "
           << conn->localAddress().toIpPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");
}

void EchoServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
                           muduo::net::Buffer* buf,
                           muduo::Timestamp time)
{
  muduo::string msg(buf->retrieveAllAsString());
  LOG_INFO << conn->name() << " echo " << msg.size() << " bytes, "
           << "data received at " << time.toString();
  conn->send(msg);//把收到的数据原封不动地发回客户端。
}
```









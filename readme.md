## 更多内容

[我的博客](https://www.cnblogs.com/HeyLUMouMou/tag/mit6.828/)
整理了JOS的部分实现原理，以及它与xv6和linux的某些对比


[JOS详解(PDF版)](https://github.com/SixPlusAndTimes/jos_labs/blob/lab5/JOS%E8%AF%A6%E8%A7%A3.pdf) 

- 梳理了我在实验过程中的心得
- 比较了xv6和JOS两个操作系统, 尤其是文件系统那一块，由于JOS不是宏内核，所以具体实现与xv6、linux的实现有很大差别。我自行阅读xv6源码将其整理出来，其中日志层的实现是JOS没有的。
- 其中也补充了linux相关的内容，尤其是进程调度那一方面。

## 启动JOS
### 使用qemu启动
~~~
cd mit6.828
make clean
make qemu
~~~
### 退出qemu
按ctrl + a 再按x
## 可能的优化
### 磁盘block缓存的LRU剔除策略
JOS使用缺页中断的方式将磁盘缓冲块读如内存，然而JOS缺少一种剔除策略，这些缓冲块将永远存放与物理内存中。

我对其进行了一些优化：限制物理内存对磁盘块的缓冲（我是限定了30），当缓冲块满时，使用**LRU剔除策略**选择一个牺牲块移出内存。

具体实现方式参考了xv6的实现。
- 定义了一个容量为30的blockcaches数组，其中blockcache结构有两个指针，这是构成双向链表的基础，还有一个vaddr的成员，记录该缓冲块对应的虚拟地址。
- cache_head, 是blockcache结构形成链表的头部
- cache_size, 表示文件服务进程当前缓冲块的数量，初始值为0， 当cache_size == BLOCK_CACHE_CAP时，会使用LRU剔除策略选择要牺牲的块

init_blockcaches函数则把这些缓冲块初始化，并形成双向链表。
~~~c
#define BLOCK_CACHE_CAP 30
struct blockcache{
	struct blockcache* prev;
	struct blockcache* next;
	void* vaddr;
};
struct blockcache blockcaches[BLOCK_CACHE_CAP];
struct blockcache cache_head;
static int cache_size;
static void init_blockcaches(){
	// 初始化 buffercache 链表
	cache_head.next = &cache_head;
	cache_head.prev = &cache_head;
	cache_head.vaddr = 0 ; 
	cache_size = 0;

	struct blockcache* bc;
	for (bc = blockcaches; bc < blockcaches + BLOCK_CACHE_CAP; bc++) {
		bc->next = cache_head.next;
		bc->prev = &cache_head;
		bc->vaddr = 0;
		cache_head.next->prev = bc;
		cache_head.next = bc;
	}
} 
// 在bc_init()中添加init_blockcaches函数的调用
void
bc_init(void)
{
    //... 
	init_blockcaches();
	// ..
}
~~~
另一个主要改动在bc_pgfault函数中，从磁盘读如一个block后，对cache_size递增，判断是否需要剔除一个block：
~~~c
static void
bc_pgfault(struct UTrapframe *utf)
{
    // ...
	// 从磁盘读如一个block后
	/* 以下代码是cache block的LRU剔除策略*/
	if (++cache_size == BLOCK_CACHE_CAP + 1) {
		// cache容量满，剔除最后一个cache
		void* evict_addr = cache_head.prev->vaddr;
		if (uvpt[PGNUM(evict_addr)] & PTE_D) { // 如果此块已脏，flush到磁盘
			flush_block(evict_addr);
		}
		if ((sys_page_unmap(thisenv->env_id, evict_addr)) < 0) { // 解除映射
			panic("bc.c bc_pgfault, sys_page_unmap failed\n ");
		}
		cache_size--;
		// 记录新的地址
		cache_head.prev->vaddr = addr;

		struct blockcache* new_cache = cache_head.prev;
		// 把这个cacheblock移动到链表头部
		remove_cachelist(new_cache);
		add_head_cachelist(new_cache);

	}else {
		// cache容量未满，从头到尾扫描一个可以用的cache
		// cprintf("\t debug Log : get a available cache\n");
		struct blockcache* bc = cache_head.next;
		for (; bc != &cache_head; bc = bc->next) {
			if (bc->vaddr == 0) {
				bc->vaddr = addr;
				break;
			}
		}
	}
}
~~~
两个操作链表的辅助函数remove_cachelist、add_head_cachelist见源码，就不单独列出了。
## git
### 显示一共有多上远程仓库连接
~~~shell
git remote
~~~

### 显示本地分支
~~~shell
git branch
~~~
### 切换分支

~~~shell
git checkout 分支名
~~~
### push到自定义仓库
~~~shell
git push 自己的仓库
~~~


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


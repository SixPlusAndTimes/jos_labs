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

## 更多内容
[我的博客](https://www.cnblogs.com/HeyLUMouMou/tag/mit6.828/)整理了JOS的部分实现原理，以及它与xv6和linux的某些对比
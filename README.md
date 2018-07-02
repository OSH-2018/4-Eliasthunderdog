# Meltdown attack demo

## 实验环境

fedora 25 with linux kernel version 4.8.6-300
gcc 6.4.1

## 实验要点

### 基本思想

一种side-channel attack，其实就是利用乱序执行过程中cache状态的改变，通过测量cache的访问时间得出具体是拿一页被装入了cache，间接知道访问位置的内存信息。

m <- [victim] \
array[m * page_size]

以上两个操作中，第一个执行时候会产生异常，但因为乱序执行，下面的指令也会被执行，导致cache中被装入一个页，通过flush-measure的步骤，我们就可以知道哪一个页被装入，从而知道[victim]

### 具体实现步骤


### 经验教训
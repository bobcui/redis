1. sysctl vm.overcommit_memory=1

overcommit_memory文件指定了内核针对内存分配的策略，其值可以是0、1、2。                               
0， 表示内核将检查是否有足够的可用内存供应用进程使用；如果有足够的可用内存，内存申请允许；否则，内存申请失败，并把错误返回给应用进程。 
1， 表示内核允许分配所有的物理内存，而不管当前的内存状态如何。
2， 表示内核允许分配超过所有物理内存和交换空间总和的内存


2. echo never > /sys/kernel/mm/transparent_hugepage/enabled

对于redis而言，开启THP的优势在于：
    - fork子进程的时间大幅减少。fork进程的主要开销是拷贝页表、fd列表等进程数据结构。由于页表大幅较小（2MB / 4KB = 512倍）,fork的耗时也会大幅减少。
劣势在于：
    - fork之后，父子进程间以copy-on-write方式共享地址空间。如果父进程有大量写操作，并且不具有locality，会有大量的页被写，并需要拷贝。同时，由于开启THP，每个页2MB，会大幅增加内存拷贝。
http://blog.csdn.net/chosen0ne/article/details/46625359


3. set up cpu cores - 1 redis servers

4. try to use more than one databases in one redis server

5. use ziplist properly

6. know types well, use them properly

7. know rdb & aof well, use them in the proper scenario

8. keep connection alive if possible

9. use commands info, slowlog & latency to find the 
    info commandstats
    slowlog get 5
    latency doctor

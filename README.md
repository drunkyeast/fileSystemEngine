## fileSystemEngine  淘宝文件系统引擎,小文件存储优化问题.
TFS: Taobao File System


shift+alt+f vscode中格式化代码规范
git push -u origin main 这个-u参数它会将本地分支与远程分支关联，方便后续的 git push 和 git pull 操作。这样，你可以在之后直接使用 git push 或 git pull，而不需要每次指定远程和分支。
-u 就是 --set-upstream的简称

## 一些知识点记录
`g++ mmap_file_op_test.cpp mmap_file_op.cpp file_op.cpp mmap_file.cpp -o mmap_file_op_test`
这里.cpp文件的顺序不影响结果, 以前学csapp链接部分时, 与一些静态链接库一起编译时才需要注意链接顺序问题, 因为链接器是按顺序解析符号的.

vim中: $可移动到末尾, `:goto 4000` 移动到4000byte的位置

统计.h.cpp文件总行数: `apt-get install silversearcher-ag` + `ag '' --cpp -l | xargs wc -l`

## 设计类图
这个可以了解, 不用掌握. Martin写代码还行, 讲知识点就是依托答辩.

## 总结一下这个项目
2025年2月9日晚基本完成, 花了4天时间. .h.cpp文件总共2000行.
1. 大致介绍: 项目是做一个分布式文件系统, 模拟的是TFS(taobao file system), 但后续这个项目似乎被淘宝的另一个分布式文件存储项目PanGu给取代.  因为淘宝网站有大量的小文件, 如果每个小文件直接当作一个普通文件存储在linux系统中, 在读取文件效率很低(逐级遍历目录, 根据Inode number查询Inode table... 最后确定文件存储的位置, 这个过程可以优化). 另外每个文件都有一个inode也会占用额外的空间. 可以把各种小文件的inode信息集合起来, 作为块索引IndexHandler, 且将IndexHandler用mmap映射到内存中, 让找到文件存储位置的过程得到加速. 然后将文件存储的内容放在一个主块(mainblock)中, 最后还有拓展块(这个小项目没有用到). 后面还有一个哈希桶,也是加速线性查找的速度.
2. 代码分析: 首先我要创建两个目录,./index和./mainblock, index的内容要映射到内存中. 然后用block_init_test.cpp, 创建索引文件和主块文件. 然后我存储一个文件,见block_write_test.cpp, 首先要将IndexHandle load加载一下,这里面就会映射到内存. 然后打开主块,写入主块中. 注意主块文件不需要映射到内存, 所以两者的类分别是largefile::IndexHandle与largefile::FileOperation. 我说这个是想让我以后看到这儿时能知道几个类之间的关系,继承,成员变量... 其实然后再看看block_read_test.cpp, block_delete_test.cpp等等就能搞明白了. 这里再说一下用到的数据结构是哈希桶, 名字听起来很帅, 实际上就是每个文件有一个uint64_t的file_id,然后file_id % bocket_size(例如可以是1000). 然后相当于有1000个链表(链表实际是以用数组形式存在),这样做的目的也是为了加速检索吧. 不然要一个个线性查找. 例如写入一个文件, 要在索引中创建一个meta_info, 记录了文件在主块的偏移位置和大小, 然后这个meta_info用哈希桶分配到某个slot(槽)中, 然后链表插入操作(本身是数组形式存储,但又next_meta_offset_这个成员变量). 然后还要更新一下块信息, 例如现在存储的文件数++,未使用的数据起始偏移+=file_size.... 以上说的是写入文件,IndexHandle的变化, 然后要把数据再写入主块中, 接口用的是mainblock->pread. 这个mainblock并没有映射, mainblock就理解成一个fd就行. 说完block_write_test, 后面的读和删除都差不多. 删除的话需要额外处理块信息的一些东西, 例如删除文件数量++, 以及可重用链表结点, 因为IndexHandle是映射到内存的, 内存宝贵, 删除一个metainfo时, 加入可重用链表中, 下次再添加文件时就从这里面选择. 然后是主块的清理工作, 再删除文件达到一定比例例如20%时, 再在空闲时间如夜深人静的晚上, 再把主块中存储的东西进行整理, 清理碎片嘛. 写得很乱, 我也不知道以后会不会再看.
3. 吐槽: 这个项目写得不mordern, printf, cout, perror, fprintf(stderr), cerr都再混用. 另外没有用try catch, 而是用ret值去处理异常. "当年的淘宝的大牛可能时加班做的", uint64_t居然又会强制转化成int32_t.
4. 收获:  namespace的应用很好, "common.h"的设计也很好如里面的错误码/全局变量/枚举值等待, 异常处理,很多错误情况的处理,内存delete等等要注意, 这就是"防御性编程"吗?. vim可以:goto 4000操作等等, 以及cpp在编译时顺序无所谓只是动态静态链接库的顺序有时要注意. 还有一些杂七杂八的收获, 反正学了整整4天, 干货和知识点收获的不多.
5. 改进: 以后有机会用mordern C++重构一下呗,然后把类这些重新设计, 真的感觉类的设计接口的设计蛮重要的. 以及优化block_write/read/delete. 我希望能实用起来, 例如我1000个小文件(可以时我的博客文章), 我按照顺序write/read/delete等测试...... 算了吧我肯定时不会做这些的. 

## 最后的废话
可以把类的设计, 索引文件和主块的结构图和关键的数据结构也上传一下, 但我懒得搞了. 主要是我认为自己以后回顾这个东西的概率很低, 而且看看代码似乎就够了?

## github提交问题
这里没写.gitignore, 主块文件不需要上传啊, 且主块64MB超过了github文件上限, 其实只需要上传.h和.cpp, 我写一些.gitignore吧.
```
git rm -r --cached .
git add .
git status
或者
git rm -r --cached index/ mainblock/ .vscode/
git status
```
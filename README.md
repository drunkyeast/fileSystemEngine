## fileSystemEngine  淘宝文件系统引擎,小文件存储优化问题.


shift+alt+f vscode中格式化代码规范
git push -u origin main 这个-u参数它会将本地分支与远程分支关联，方便后续的 git push 和 git pull 操作。这样，你可以在之后直接使用 git push 或 git pull，而不需要每次指定远程和分支。
-u 就是 --set-upstream的简称

## 一些知识点记录
`g++ mmap_file_op_test.cpp mmap_file_op.cpp file_op.cpp mmap_file.cpp -o mmap_file_op_test`
这里.cpp文件的顺序不影响结果, 以前学csapp链接部分时, 与一些静态链接库一起编译时才需要注意链接顺序问题, 因为链接器是按顺序解析符号的.

vim中: $可移动到末尾, `:goto 4000` 移动到4000byte的位置

统计.h.cpp文件总行数: `apt-get install silversearcher-ag` + `ag '' --cpp -l | xargs wc -l`

## **在Linux中下修改一个现有的elf可执行程序** 

1. 首先了解 elf 文件结构:

   1. **可重定位文件:** 包含适合于与其他目标文件链接来创建可执行文件或者共享目标文件的代码和数据。
   2. **可执行文件:**  包含适合于执行的一个程序,此文件规定了exec() 如何创建一个程序的进程映像。
   3. **共享目标文件:**  包含可在两种上下文中链接的代码和数据。首先链接编辑器可以将它和其它可重定位文件和共享目标文件一起处理,生成另外一个目标文件。其次,动态链接器(Dynamic Linker)可能将它与某个可执行文件以及其它共享目标一起组合,创建进程映像。

   **详细细节参考：** [Linux及安全实践四——ELF文件格式分析](https://www.cnblogs.com/lxq20135309/p/5551658.html) ， ELF格式可执行程序的代码嵌入（同目录下的 pdf 文件）

2. 知道 Linux 下的汇编与 WIN32 汇编的区别

   1. [Linux 汇编语言开发指南](https://www.ibm.com/developerworks/cn/linux/l-assembly/) 
   2. [linux下汇编语言开发总结](https://blog.csdn.net/q_l_s/article/details/52722667) 
   3. [Linux 汇编器：对比 GAS 和 NASM](https://www.ibm.com/developerworks/cn/linux/l-gas-nasm.html) 

3. 开始动手实现向现有 elf 文件中插入功能，如：让程序首先执行创建一个 output.txt 文件，并写入 hello world 

   1. 先生成一个 elf 程序，并通过 FILE *fp = fopen("test", "rb+") 读取该 elf 文件
   2. 接着获取 elf 文件的入口地址，程序头，节区头等信息
   3. 然后写出要添加的附加功能的汇编程序并编译获取机器码，通过 readelf -e elf文件名 查看 elf  文件的程序入口地址等信息，并根据 elf 文件的程序地址等信息修改生成的机器码中相应的地址信息
   4. 将修改好的机器码添加进elf的text段的最后（为了方便，此处我将数据也一并添加进了text段）
   5. 修改elf 的入口地址为嵌入的程序首地址，当嵌入程序执行结束再跳转到原 elf 文件的首地址，使程序继续执行原本的 elf 文件程序，再修改 elf 的程序头，节区头
   6. 通过 fclose(fp) 关闭 elf 文件

4. 懂得如何根据文件信息修改对应的机器码

   **参考：**

   1. [ELF文件病毒的分析和编写](https://blog.csdn.net/luojiafei/article/details/7206063)  
   2. [自己动手写病毒—ELF文件病毒](https://blog.csdn.net/luojiafei/article/details/7225818) 

     ​	


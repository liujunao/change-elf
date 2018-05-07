.data				 # 数据段声明

	msg:   .ascii  "hello, world\n"
	len= .-msg
	filename:  .ascii  "output.txt"

.text				 # 代码段声明

.global _start		 # 指定入口函数

_start:

    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    movq $5, %rax
    movq $filename, %rbx
    movq $03101, %rcx
    movq $0666, %rdx
    int $0x80		# 调用内核功能
    movq %rax,%rbx
    movq $4, %rax	#系统调用号(sys_write) 
    movq $msg, %rcx #要写入的字符串
    movq $len, %rdx #字符串长度
    int  $0x80 	    #调用内核功能
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    				# 退出程序
    movl $1, %eax	# 系统调用号(sys_exit) 
    movl $0, %ebx	# 参数一：退出代码
    int $0x80		# 调用内核功能

.data
	msg: .asciz "Hello world!\n"
	mlen = . - msg
	filename: .asciz "output.txt"
	flen = . - filename

.text
.global _start

_start:
	pushq %rax
	pushq %rbx
	pushq %rcx
	pushq %rdx
	movq $5, %rax
	movq $filename, %rbx
	movq $0101, %rcx
	movq $0644, %rdx
	int $0x80
	movq %rax, %rbx
	movq $4, %rax
	movq $msg, %rcx
	movq $mlen, %rdx
	int $0x80
	movq $6, %rax
	int $0x80
	popq %rdx
	popq %rcx
	popq %rbx
	popq %rax

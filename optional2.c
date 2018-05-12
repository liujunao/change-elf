#include <stdio.h>
#include <string.h>
#include <elf.h>

/**
 * 要嵌入的执行程序的代码长度为 71 == 0x0047
 * 要嵌入的数据的长度 hello, world：15 == 0x000f
 * 我的测试 elf 文件的入口地址为 0x400470
 * 我的测试 elf 文件的第一个 LOAD 地址(外加功能写入的起始地址)是 0x4007e4
 */
 //code为要插入的附加功能代码
char code[] = {
	0x50, 0x53, 0x51, 0x52,					  //pushq %rax pushq %rbx pushq %rcx pushq %rdx
	0x48, 0xc7, 0xc0, 0x05, 0x00, 0x00, 0x00, //movq $5, %rax
	0x48, 0xc7, 0xc3, 0x3a, 0x08, 0x40, 0x00, //movq $filename, %rbx,要将此处的地址换成 elf 的（节区空闲段地址 + 0x0047 + 0x000f == 0x4007e4 + 0x0047 + 0x000f）
	0x48, 0xc7, 0xc1, 0x41, 0x06, 0x00, 0x00, //movq $03101, %rcx
	0x48, 0xc7, 0xc2, 0xb6, 0x01, 0x00, 0x00, //movq $0666, %rdx
	0xcd, 0x80,								  //int $0x80
	0x48, 0x89, 0xc3,						  //movq %rax,%rbx
	0x48, 0xc7, 0xc0, 0x04, 0x00, 0x00, 0x00, //movq $4, %rax
	0x48, 0xc7, 0xc1, 0x2b, 0x08, 0x40, 0x00, //movq $msg, %rcx，要将此处的地址换成 elf 的（节区空闲段地址 + 0x0047 == 0x4007e4 + 0x0047 ）
	0x48, 0xc7, 0xc2, 0x0d, 0x00, 0x00, 0x00, //movq $len, %rdx
	0xcd, 0x80,								  //int $0x80
	0x5a, 0x59, 0x5b, 0x58,					  //popq %rdx popq %rcx popq %rbx popq %rax

	0xbd, 0x70, 0x04, 0x40, 0x00, 0xff, 0xe5, //跳转指令，跳转到原始 elf 的入口地址 0x400470

	0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x2c, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x0a, 0x6f, 0x75, //hello, world
	0x6f, 0x75, 0x74, 0x70, 0x75, 0x74, 0x2e, 0x74, 0x78, 0x74, 0x00};						  //output.txt

int main()
{
	//我的 Linux 系统是64位，所以定义为64位单元,这个根据 Linux 系统位数的不同而改变
	typedef uint64_t Unit64;

	//打开要修改的测试elf文件,这里的elf测试文件名叫 test
	FILE *fp = fopen("newtest", "rb+");

	Elf64_Ehdr e_header; //ELF头
	Elf64_Phdr p_header; //程序头
	Elf64_Shdr s_header; //节区头

	//读取elf头，定位到程序头表，节区头表的位置；
	fread(&e_header, sizeof(Elf64_Ehdr), 1, fp);
	//程序入口地址
	Unit64 entry = e_header.e_entry;
	//程序头部的偏移量
	Unit64 phdr = e_header.e_phoff;
	//节区头部的偏移量
	Unit64 shdr = e_header.e_shoff;

	//e_phentsize为程序头表中每个表项的大小
	//获取到程序头，并赋给 p_header
	//p_off为程序头偏移地址
	Unit64 p_off = phdr + 2 * e_header.e_phentsize;
	//找到程序头的位置
	fseek(fp, p_off, SEEK_SET);
	fread(&p_header, sizeof(Elf64_Phdr), 1, fp);

	//获取到节区头，并赋给 s_header
	//s_off偏移地址，表偏移加上表项偏移
	Unit64 s_off = shdr + e_header.e_shnum * e_header.e_shentsize;
	fseek(fp, s_off, SEEK_SET);
	fread(&s_header, sizeof(Elf64_Phdr), 1, fp);

	//要嵌入的机器码的大小
	Unit64 size = sizeof(code);

	/**
	 * 写入嵌入的代码
	 */
	fseek(fp, p_header.p_filesz, SEEK_SET); //定位到段的末尾
	for (int i = 0; i < size; i++)
	{
		fwrite(&code[i], sizeof(char), 1, fp);
	}

	/**
	 * 修改程序入口地址
	 */
	entry = p_header.p_vaddr + p_header.p_filesz; //原始程序入口地址
	fseek(fp, 24, SEEK_SET);
	fwrite(&entry, sizeof(Unit64), 1, fp); //修改程序入口地址
	fseek(fp, 8, SEEK_CUR);
	fwrite(&shdr, sizeof(Unit64), 1, fp); //修改节区头偏移量

	/**
	 * 修改程序头
	 */
	p_header.p_filesz += 1000; //修改该段的节区总长度，加上嵌入代码长度
	p_header.p_memsz += size;  //修改该段内存大小，加上嵌入代码长度
	//定位到 p_filesz 成员变量
	fseek(fp, phdr + e_header.e_phnum * e_header.e_phentsize, SEEK_SET);
	//SEEK_CUR是以当前位置加上偏移确定位置
	
	//结构体中的第四个属性，每个属性8个字节
	fseek(fp, 32, SEEK_CUR);
	//修改程序头的 p_filesz（该段的节区的总长度）
	fwrite(&p_header.p_filesz, sizeof(Unit64), 1, fp);
	//修改程序头的 p_memsz（段在内存中占用大小）
	fwrite(&p_header.p_memsz, sizeof(Unit64), 1, fp); //p_memsz 就在 p_filesz 之后，所以不用重新定位

	/**
	 * 修改节区头
	 */
	s_header.sh_size += size; //修改该段的节区大小，加上嵌入代码长度
	fseek(fp, s_off, SEEK_SET);
	fseek(fp, 32, SEEK_CUR);
	//修改节区头的 sh_size（节区大小）
	fwrite(&s_header.sh_size, sizeof(Unit64), 1, fp);

	fclose(fp);
}
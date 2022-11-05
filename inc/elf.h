#ifndef JOS_INC_ELF_H
#define JOS_INC_ELF_H

#define ELF_MAGIC 0x464C457FU	/* "\x7FELF" in little endian */

struct Elf { // elf 文件头结构体， 共52字节
	uint32_t e_magic;	// must equal ELF_MAGIC
	uint8_t e_elf[12];
	uint16_t e_type;
	uint16_t e_machine;
	uint32_t e_version;
	uint32_t e_entry;	// 程序的入口地址
	uint32_t e_phoff;  // 程序头表偏移地址
	uint32_t e_shoff;  // 节头表偏移地址
	uint32_t e_flags;
	uint16_t e_ehsize; // elf 文件头大小
	uint16_t e_phentsize; // 程序头表大小
	uint16_t e_phnum;		//程序头个数
	uint16_t e_shentsize;  // 节头表大小
	uint16_t e_shnum;      // 节头个数
	uint16_t e_shstrndx;	//节区字符串表在节头表中的下标
};

struct Proghdr { // 程序头结构
	uint32_t p_type; // 该段的类型，比如说可装载段，可装载段会被加载到内存
	uint32_t p_offset; // 该程序段在磁盘上相对于 文件起始的偏移地址
	uint32_t p_va; // 该段加载到内存时的虚拟地址，exec函数中用到
	uint32_t p_pa; // 该段加载到内存时的物理地址，启动时加载内核 elf 文件时用到
	uint32_t p_filesz;// 该段在磁盘上的大小，单位字节
	uint32_t p_memsz; // 该段在内存上的大小， 单位字节
	uint32_t p_flags;
	uint32_t p_align;
};

struct Secthdr {
	uint32_t sh_name;
	uint32_t sh_type;
	uint32_t sh_flags;
	uint32_t sh_addr;
	uint32_t sh_offset;
	uint32_t sh_size;
	uint32_t sh_link;
	uint32_t sh_info;
	uint32_t sh_addralign;
	uint32_t sh_entsize;
};

// Values for Proghdr::p_type
#define ELF_PROG_LOAD		1

// Flag bits for Proghdr::p_flags
#define ELF_PROG_FLAG_EXEC	1
#define ELF_PROG_FLAG_WRITE	2
#define ELF_PROG_FLAG_READ	4

// Values for Secthdr::sh_type
#define ELF_SHT_NULL		0
#define ELF_SHT_PROGBITS	1
#define ELF_SHT_SYMTAB		2
#define ELF_SHT_STRTAB		3

// Values for Secthdr::sh_name
#define ELF_SHN_UNDEF		0

#endif /* !JOS_INC_ELF_H */

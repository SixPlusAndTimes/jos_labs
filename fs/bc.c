
#include "fs.h"

#define BLOCK_CACHE_CAP 30
struct blockcache{
	struct blockcache* prev;
	struct blockcache* next;
	void* vaddr;
};
struct blockcache blockcaches[BLOCK_CACHE_CAP];
struct blockcache cache_head;
static int cache_size;
static void init_blockcaches(){
	// 初始化 buffercache 链表
	cache_head.next = &cache_head;
	cache_head.prev = &cache_head;
	cache_head.vaddr = 0 ; 
	cache_size = 0;

	struct blockcache* bc;
	for (bc = blockcaches; bc < blockcaches + BLOCK_CACHE_CAP; bc++) {
		bc->next = cache_head.next;
		bc->prev = &cache_head;
		bc->vaddr = 0;
		cache_head.next->prev = bc;
		cache_head.next = bc;
	}

	// cprintf("debuglog : init_blockcaches done\n");
} 
// Return the virtual address of this disk block.
void*
diskaddr(uint32_t blockno)
{
	if (blockno == 0 || (super && blockno >= super->s_nblocks))
		panic("bad block number %08x in diskaddr", blockno);
	return (char*) (DISKMAP + blockno * BLKSIZE);
}

// Is this virtual address mapped?
bool
va_is_mapped(void *va)
{
	return (uvpd[PDX(va)] & PTE_P) && (uvpt[PGNUM(va)] & PTE_P);
}

// Is this virtual address dirty?
bool
va_is_dirty(void *va)
{
	return (uvpt[PGNUM(va)] & PTE_D) != 0;
}

void remove_cachelist(struct blockcache* bc) {
	bc->prev->next = bc->next;
	bc->next->prev = bc->prev;
}

void add_head_cachelist(struct blockcache* bc) {
	struct blockcache* temp = cache_head.next;

	cache_head.next = bc;
	bc->prev = &cache_head;

	bc->next = temp;
	temp->prev = bc;
}
// Fault any disk block that is read in to memory by
// loading it from disk.
static void
bc_pgfault(struct UTrapframe *utf)
{
	cprintf("bc_pgfault:---------------  \n");
	// 与fork的pgfault相比，这里的处理函数需要处理 PTE_D, 而fork的pgfault需要处理PTE_COW
	void *addr = (void *) utf->utf_fault_va;
	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;
	int r;

	// Check that the fault was within the block cache region
	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("page fault in FS: eip %08x, va %08x, err %04x",
		      utf->utf_eip, addr, utf->utf_err);

	// Sanity check the block number.
	if (super && blockno >= super->s_nblocks)
		panic("reading non-existent block %08x\n", blockno);

	// Allocate a page in the disk map region, read the contents
	// of the block from the disk into that page.
	// Hint: first round addr to page boundary. fs/ide.c has code to read
	// the disk.
	//
	// LAB 5: you code here:
	addr = (void *) ROUNDDOWN(addr, PGSIZE);
	// 分配一个物理页并与addr建立映射
	if ( (r = sys_page_alloc(0, addr, PTE_P | PTE_W | PTE_U)) < 0) {
		panic("bc.c:bc_pgfault sys_page_alloc failed\n");
	}
	//BLKSECTS : sectors per block
	// 从磁盘读如 BLKSIZE大小的内容，JOS中BLKSIZE = PGSIZE
	if ( (r = ide_read(blockno * BLKSECTS, addr, BLKSECTS)) < 0) {
		panic("bc.c:bc_pgfault ide_read failed\n");
	}
	// Clear the dirty bit for the disk block page since we just read the
	// block from disk
	if ((r = sys_page_map(0, addr, 0, addr, uvpt[PGNUM(addr)] & PTE_SYSCALL)) < 0)
		panic("in bc_pgfault, sys_page_map: %e", r);

	// Check that the block we read was allocated. (exercise for
	// the reader: why do we do this *after* reading the block
	// in?)
	if (bitmap && block_is_free(blockno))
		panic("reading free block %08x\n", blockno);

	/* 以下代码是cache block的LRU剔除策略*/
	// cprintf("debug log : cache_size = %d", cache_size);
	
	if (++cache_size == BLOCK_CACHE_CAP + 1) {
		// cache容量满，剔除最后一个cache
		void* evict_addr = cache_head.prev->vaddr;
		if (uvpt[PGNUM(evict_addr)] & PTE_D) { // 如果此块已脏，flush到磁盘
			flush_block(evict_addr);
		}
		if ((sys_page_unmap(thisenv->env_id, evict_addr)) < 0) { // 解除映射
			panic("bc.c bc_pgfault, sys_page_unmap failed\n ");
		}
		cache_size--;
		// 记录新的地址
		cache_head.prev->vaddr = addr;

		struct blockcache* new_cache = cache_head.prev;
		// 把这个cacheblock移动到链表头部
		remove_cachelist(new_cache);
		add_head_cachelist(new_cache);

	}else {
		// cache容量未满，从头到尾扫描一个可以用的cache
		// cprintf("\t debug Log : get a available cache\n");
		struct blockcache* bc = cache_head.next;
		for (; bc != &cache_head; bc = bc->next) {
			if (bc->vaddr == 0) {
				bc->vaddr = addr;
				break;
			}
		}
	}
}

// Flush the contents of the block containing VA out to disk if
// necessary, then clear the PTE_D bit using sys_page_map.
// If the block is not in the block cache or is not dirty, does
// nothing.
// Hint: Use va_is_mapped, va_is_dirty, and ide_write.
// Hint: Use the PTE_SYSCALL constant when calling sys_page_map.
// Hint: Don't forget to round addr down.
void
flush_block(void *addr)
{
	uint32_t blockno = ((uint32_t)addr - DISKMAP) / BLKSIZE;

	if (addr < (void*)DISKMAP || addr >= (void*)(DISKMAP + DISKSIZE))
		panic("flush_block of bad va %08x", addr);
	int r;
	// LAB 5: Your code here.
	addr = ROUNDDOWN(addr,PGSIZE);
	// If the block is not in the block cache or is not dirty, does nothing
	if (!va_is_mapped(addr) || ! va_is_dirty(addr)) {
		return ;
	}
	// Flush the contents of the block containing VA out to disk
	if ((r = ide_write(blockno * BLKSECTS,addr,BLKSECTS)) < 0 ) {
		panic("bc.c:flush_block ide_write failed");
	}
	// clear the PTE_D bit using sys_page_map.Use the PTE_SYSCALL constant when calling sys_page_map.
	if ((r = sys_page_map(0,addr,0,addr, uvpt[PGNUM(addr)] & PTE_SYSCALL) ) < 0 ) {
		panic("bc.c:flush_block sys_page_map failed");
	}
}

// Test that the block cache works, by smashing the superblock and
// reading it back.
static void
check_bc(void)
{
	struct Super backup;
	// cprintf("debug\n");

	// back up super block
	memmove(&backup, diskaddr(1), sizeof backup);
// cprintf("afeter memmove\n");
	// smash it
	strcpy(diskaddr(1), "OOPS!\n");
	flush_block(diskaddr(1));
	assert(va_is_mapped(diskaddr(1)));
	assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!va_is_mapped(diskaddr(1)));
//
	// read it back in
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memmove(diskaddr(1), &backup, sizeof backup);
	flush_block(diskaddr(1));

	// Now repeat the same experiment, but pass an unaligned address to
	// flush_block.

	// back up super block
	memmove(&backup, diskaddr(1), sizeof backup);
//
	// smash it
	strcpy(diskaddr(1), "OOPS!\n");

	// Pass an unaligned address to flush_block.
	flush_block(diskaddr(1) + 20);
	assert(va_is_mapped(diskaddr(1)));

	// Skip the !va_is_dirty() check because it makes the bug somewhat
	// obscure and hence harder to debug.
	//assert(!va_is_dirty(diskaddr(1)));

	// clear it out
	sys_page_unmap(0, diskaddr(1));
	assert(!va_is_mapped(diskaddr(1)));

	// read it back in
	assert(strcmp(diskaddr(1), "OOPS!\n") == 0);

	// fix it
	memmove(diskaddr(1), &backup, sizeof backup);
	flush_block(diskaddr(1));

	cprintf("block cache is good\n");
}

void
bc_init(void)
{
	struct Super super;
	set_pgfault_handler(bc_pgfault);
	// cprintf("before check bc\n");
	init_blockcaches();
	check_bc();

	// cache the super block by reading it once
	memmove(&super, diskaddr(1), sizeof super);
}


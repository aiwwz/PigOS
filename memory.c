#include "bootpack.h"


unsigned int memtest(unsigned int start, unsigned int end){
	char flag486 = 0;
	unsigned int eflag, cr0, i;
	//确认CPU
	eflag = io_load_eflags(); //获取标志寄存器状态
	eflag |= EFLAGS_AC_BIT; //将AC-bit置1
	io_store_eflags(eflag);
	eflag = io_load_eflags(); //再次获取，查看寄存器是否设置成功
	if((eflag & EFLAGS_AC_BIT) != 0) //只有486及以上才能被设置
		flag486 = 1;
	eflag &= ~EFLAGS_AC_BIT; //将AC-bit置0，恢复初始状态
	
	if(flag486){ //如果是486及以上则禁止高速缓存
		cr0 = load_cr0();
		cr0 |= CRO_CACHE_DISABLE; //禁止缓存
		store_cr0(cr0);
	}
	
	i = memtest_sub(start, end);
	
	if(flag486){ //允许缓存
		cr0 = load_cr0();
		cr0 &= ~CRO_CACHE_DISABLE;
		store_cr0(cr0);
	}
	
	return i;
}

/*
****编译器优化使之失效，无法正确工作，该函数已用汇编改写****
unsigned int memtest_sub(unsigned int start, unsigned int end){
	unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
	for(i = start; i <= end; i+=0x1000){
		p = (unsigned int*)i + 0xffc; //只检查末尾4个字节
		old = *p;
		*p = pat0;
		*p ^= 0xffffffff;
		if(*p != pat1){
			*p = old;
			return 1024*1024;
		}
		*p ^= 0xffffffff;
		if(*p != pat0){
			*p = old;
			return 2*1024*1024;
		}
		*p = old;
	}
	return i;
}
*/

void memman_init(struct MEMMAN *mem){
	mem->frees = 0;
	mem->losts = 0;
	mem->lostsize = 0;
	return;
}

unsigned int memman_total(struct MEMMAN *mem){ //所剩空间合计大小
	unsigned int t = 0;
	int i;
	for(i = 0; i < mem->frees; ++i)
		t += mem->free[i].size;
	return t;
}

unsigned int memory_alloc(struct MEMMAN *mem, unsigned int size){
	unsigned int i, a;
	for(i = 0; i < mem->frees; ++i){
		if(mem->free[i].size >= size){ //找到足够的空间
			a = mem->free[i].addr;
			mem->free[i].addr += size;
			mem->free[i].size -= size;
			if(mem->free[i].size == 0){
				mem->frees--;
				for(; i < mem->frees; ++i){
					mem->free[i] = mem->free[i+1];
				}
			}
			return a;
		}
	}
	return 0;
}

int memory_free(struct MEMMAN *mem, unsigned int addr, unsigned int size){
	int i, j;
	for(i = 0; i < mem->frees; ++i){
		if(mem->free[i].addr > addr)
			break;
	}
	/*free[i-1].addr < addr < free[i]*/
	
	if(i > 0){ //前面有可用内存
		/*查看前面的可用内存是否可以连接*/
		if(mem->free[i-1].addr + mem->free[i-1].size == addr){
			/*可与前面的可用内存连接*/
		
			mem->free[i-1].size += size; //归纳到一起
		
			if(i < mem->frees){
				/*查看后面的可用内存是否可以连接*/
				if(addr+size == mem->free[i].addr){
					/*也可与后面的内存连接*/
					mem->free[i-1].size += size; //把三处归纳到一起
					/*删除free[i]*/
					mem->frees--;
					for(; i < mem->frees; ++i){ //i之后的free[]整体前移
						mem->free[i] = mem->free[i+1];
					}
				}
			}
			return 0; //与前面相连归纳成功
		}
	}
	/*不能与前面相连，查看能否与后面相连*/
	if(i < mem->frees){ //后面有可用内存
		/*查看后面的可用内存是否可以连接*/
		if(addr + size == mem->free[i].addr){
			/*可与后面的内存连接*/
			mem->free[i].addr = addr; //与后面的内存归纳到一起
			mem->free[i].size += size;
			return 0; //归纳成功
		}
	}
	/*前后均无可直接相连的内存*/
	/*直接添加新free*/
	if(mem->frees < MEMMAN_FREES){
		for(j = mem->frees; j > i; --j){
			mem->free[j] = mem->free[j-1];
		}
		mem->free[i].addr = addr; //添加新free成功
		mem->free[i].size = size;
		mem->frees++;
		return 0; //成功
	}
	/*无法继续添加free*/
	mem->losts++;
	mem->lostsize += size;
	return -1; //释放内存失败
}

/*以单字节为单位很容易造成内存空间含有大量无法使用的细小碎片，
所以我们使用4kb为单位来管理内存，即对用户请求分配和释放内存时
均以请求内存向上取4kb的整数倍来处理。*/
unsigned int memory_alloc_4k(struct MEMMAN *mem, unsigned int size){
	unsigned int a;
	size = (size + 0x00000fff) & 0xfffff000;  //size向上取整为4kb的倍数
	a = memory_alloc(mem, size);
	return a;
}

int memory_free_4k(struct MEMMAN *mem, unsigned int addr, unsigned int size){
	size += 0x00000fff;
	size &= 0xfffff000;
	return memory_free(mem, addr, size);
}
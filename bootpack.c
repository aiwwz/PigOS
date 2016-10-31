#include <stdio.h>
#include "bootpack.h"

extern struct FIFO keyfifo, mousefifo;

void HariMain(){
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADR_BOOTINFO;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN*)MEMMAN_ADDR;
	char s[40], mcursor[256], keybuf[128], mousebuf[128], old_back[256];
	int mx, my, i;
	unsigned int memtotal;

	//初始化GDT，IDT
	init_gdtidt();
	//初始化PIC
	init_pic();
	// IDT/PIC的初始化已经完成，于是开放CPU的中断
	io_sti();

	fifo_init(&keyfifo,  128, keybuf);
	fifo_init(&mousefifo, 128, mousebuf);
	
	io_out8(PIC0_IMR, 0xf9); // 开放PIC1和键盘中断(11111001)
	io_out8(PIC1_IMR, 0xef); // 开放鼠标中断(11101111)
	
	init_keyboard();
	
	//初始化自定义的调色板
	init_palette();
	//初始化屏幕
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
	
	init_mouse_cursor(mcursor);
	init_old_back(old_back);
	
	mx = (binfo->scrnx - 16) / 2;	//求画面中心坐标
	my = (binfo->scrny - 19 - 16) / 2;
	putblock(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	
	enable_mouse(&mdec);
	
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memory_free(memman, 0x4000000, memtotal - 0x00400000);
	
	sprintf(s, "memory: %dMB  free: %dKB", memtotal/(1024*1024), memman_total(memman)/1024);
	putstr_asc(binfo->vram, binfo->scrnx, 0, 45, BLACK, s);
	
	for(;;){
		io_cli();	//屏蔽中断
		if(fifo_status(&keyfifo) + fifo_status(&mousefifo) == 0){
			io_stihlt();  //如果无事可做则取消屏蔽中断并休息
		}
		else
			if(fifo_status(&keyfifo) != 0){
				i = fifo_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(binfo->vram, binfo->scrnx, BACK, 0, 25, 15, 40);
				putstr_asc(binfo->vram, binfo->scrnx, 0, 25, BLACK, s);
			}
			else 
				if(fifo_status(&mousefifo) != 0){
					i = fifo_get(&mousefifo);
					io_sti();
					if(mouse_decode(&mdec, i) != 0){ //鼠标数据的三个字节集齐，显示出来 
						//mouse status
						sprintf(s, "%02X %02X %02X", mdec.mouse_buf[0], mdec.mouse_buf[1], mdec.mouse_buf[2]);
						boxfill8(binfo->vram, binfo->scrnx, BACK, 32, 25, 31 + 8 * 8, 40);
						putstr_asc(binfo->vram, binfo->scrnx, 32, 25, BLACK, s);
						
						//mouse
						put_back(binfo->vram, binfo->scrnx, 16, 16, mx, my, old_back, 16);	//复原背景					mx += mdec.x;
						mx += mdec.x;
						my += mdec.y;
						if(mx < 0)
							mx = 0;
						if(mx > binfo->scrnx - 16)	
							mx = binfo->scrnx - 16;
						if(my < 0)
							my = 0;
						if(my > binfo->scrny - 16)
							my = binfo->scrny - 16;						
						
						sprintf(s, "(%d,%d)", mx, my);
						boxfill8(binfo->vram, binfo->scrnx, BACK, binfo->scrnx - 75, binfo->scrny - 36, binfo->scrnx - 75 + 9 * 8 - 1, binfo->scrny - 21);
						putstr_asc(binfo->vram, binfo->scrnx, binfo->scrnx - 75, binfo->scrny - 36, BLACK, s);
						
						save_back(binfo->vram, binfo->scrnx, 16, 16, mx, my, old_back, 16); //保存背景
						putblock(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
					}
				}	
	}
}
#include <stdio.h>
#include "bootpack.h"

extern struct FIFO keyfifo, mousefifo;

void HariMain(){
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADR_BOOTINFO;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN*)MEMMAN_ADDR;
	struct SHEET *sht_back, *sht_mouse, *sht_win;
	struct SHTCTL *shtctl;
	char s[40], keybuf[128], mousebuf[128];
	unsigned char buf_mouse[256], *buf_back, *buf_win;
	int mx, my, i;
	unsigned int memtotal, count = 0;

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
	
	//先将内存管理初始化
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memory_free(memman, 0x00001000, 0x0009e000); //?
	memory_free(memman, 0x4000000, memtotal - 0x00400000);	
	
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	sht_win = sheet_alloc(shtctl);
	buf_back = (char *)memory_alloc_4k(memman, binfo->scrnx * binfo->scrny);
	buf_win = (char *)memory_alloc_4k(memman, 160 * 52);
	init_screen(buf_back, binfo->scrnx, binfo->scrny);	
	init_mouse_cursor(buf_mouse, 99);
	make_window(buf_win, 160, 52, "Counter");
	sheet_set_buf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
	sheet_set_buf(sht_mouse, buf_mouse, 16, 16, 99);
	sheet_set_buf(sht_win, buf_win, 160, 52, -1);
	mx = (binfo->scrnx - 16) / 2;	//求画面中心坐标
	my = (binfo->scrny - 19 - 16) / 2;
	sheet_slide(sht_back, 0, 0);
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win, 80, 72);
	sheet_set_height(sht_back, 0);
	sheet_set_height(sht_win, 1);
	sheet_set_height(sht_mouse, 2);
	enable_mouse(&mdec);
	
	sprintf(s, "memory: %dMB  free: %dKB", memtotal/(1024*1024), memman_total(memman)/1024);
	putstr_asc(buf_back, binfo->scrnx, 0, 45, WHITE	, s);
	
	sheet_refresh(sht_back, 0, 45, binfo->scrnx, 61);
	
	for(;;){
		count++;  //计数器
		sprintf(s, "%010d", count);
		boxfill8(buf_win, 160, LIGHT_GRAY, 40, 28, 120, 44);
		putstr_asc(buf_win, 160, 40, 28, BLACK, s);
		sheet_refresh(sht_win, 40, 28, 120, 44);
		
		io_cli();	//屏蔽中断
		if(fifo_status(&keyfifo) + fifo_status(&mousefifo) == 0){
			io_sti();  //如果无事可做则取消屏蔽中断并休息
		}
		else{
			if(fifo_status(&keyfifo) != 0){
				i = fifo_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(buf_back, binfo->scrnx, BACK, 0, 25, 15, 40);
				putstr_asc(buf_back, binfo->scrnx, 0, 25, WHITE, s);
				sheet_refresh(sht_back, 0, 25, 16,41);
			}
			else {
				if(fifo_status(&mousefifo) != 0){
					i = fifo_get(&mousefifo);
					io_sti();
					if(mouse_decode(&mdec, i) != 0){ //鼠标数据的三个字节集齐，显示出来 
						//mouse status
						sprintf(s, "%02X %02X %02X", mdec.mouse_buf[0], mdec.mouse_buf[1], mdec.mouse_buf[2]);
						boxfill8(buf_back, binfo->scrnx, BACK, 32, 25, 31 + 8 * 8, 40);
						putstr_asc(buf_back, binfo->scrnx, 32, 25, WHITE, s);
						sheet_refresh(sht_back, 32, 25, 31 + 8 * 8, 40);
						//mouse
						mx += mdec.x;
						my += mdec.y;
						if(mx < 0)
							mx = 0;
						if(mx > binfo->scrnx - 1)	
							mx = binfo->scrnx - 1;
						if(my < 0)
							my = 0;
						if(my > binfo->scrny - 1)
							my = binfo->scrny - 1;						
						
						sprintf(s, "(%d,%d)", mx, my);
						boxfill8(buf_back, binfo->scrnx, BACK, binfo->scrnx - 75, binfo->scrny - 36, binfo->scrnx - 75 + 9 * 8 - 1, binfo->scrny - 21);
						putstr_asc(buf_back, binfo->scrnx, binfo->scrnx - 75, binfo->scrny - 36, WHITE, s);						
						sheet_refresh(sht_back, binfo->scrnx - 75, binfo->scrny - 36, binfo->scrnx - 75 + 9 * 8 - 1, binfo->scrny - 21);
						sheet_slide(sht_mouse, mx, my);
					}
				}	
			}
		}	
	}
}


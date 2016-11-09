#include <stdio.h>
#include "bootpack.h"

void HariMain(){
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADR_BOOTINFO;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN*)MEMMAN_ADDR;
	struct SHEET *sht_back, *sht_mouse, *sht_win;
	struct SHTCTL *shtctl;
	struct FIFO timerfifo, timerfifo2, timerfifo3;
	struct TIMER *timer, *timer2, *timer3;
	char s[40], keybuf[128], mousebuf[128], timerbuf[8], timerbuf2[8], timerbuf3[8];
	unsigned char buf_mouse[256], *buf_back, *buf_win;
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
	init_pit();
	io_out8(PIC0_IMR, 0xf8); // 开放PIC1和键盘和PIT中断(11111000)
	io_out8(PIC1_IMR, 0xef); // 开放鼠标中断(11101111)	


	/*初始化三个计时器*/
	fifo_init(&timerfifo,  8, timerbuf);
	timer = timer_alloc();
	timer_init(timer, &timerfifo, 1);
	timer_settime(timer, 1000);
	fifo_init(&timerfifo2, 8, timerbuf2);
	timer2 = timer_alloc();
	timer_init(timer2, &timerfifo2, 1);
	timer_settime(timer2, 300);
	fifo_init(&timerfifo3, 8, timerbuf3);	
	timer3 = timer_alloc();
	timer_init(timer3, &timerfifo3, 1);
	timer_settime(timer3, 50);
	
	//初始化自定义的调色板
	init_palette();
	enable_mouse(&mdec);
	init_keyboard();
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
	
	sprintf(s, "memory: %dMB  free: %dKB", memtotal/(1024*1024), memman_total(memman)/1024);
	putstr_asc(buf_back, binfo->scrnx, 0, 45, WHITE	, s);
	
	sheet_refresh(sht_back, 0, 45, binfo->scrnx, 61);
	
	for(;;){
		//计数器
		sprintf(s, "%010d", timerctl.count);
		boxfill8(buf_win, 160, LIGHT_GRAY, 40, 28, 120, 44);
		putstr_asc(buf_win, 160, 40, 28, BLACK, s);
		sheet_refresh(sht_win, 40, 28, 120, 44);
		
		io_cli();	//屏蔽中断
		if(fifo_status(&keyfifo) + fifo_status(&mousefifo) + fifo_status(&timerfifo) + fifo_status(&timerfifo2) + fifo_status(&timerfifo3) == 0){
			io_sti();  //如果无事可做则取消屏蔽中断并休息
		}
		else if(fifo_status(&keyfifo) != 0){
			i = fifo_get(&keyfifo);
			io_sti();
			sprintf(s, "%02X", i);
			boxfill8(buf_back, binfo->scrnx, BACK, 0, 25, 15, 40);
			putstr_asc(buf_back, binfo->scrnx, 0, 25, WHITE, s);
			sheet_refresh(sht_back, 0, 25, 16,41);
		}
		else if(fifo_status(&mousefifo) != 0){
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
		else if(fifo_status(&timerfifo) != 0){
			i = fifo_get(&timerfifo);
			io_sti();
			putstr_asc(buf_back, binfo->scrnx, 0, 64, WHITE, "10[sec]");
			sheet_refresh(sht_back, 0, 64, 56, 80);
		}
		else if(fifo_status(&timerfifo2) != 0){
			i = fifo_get(&timerfifo2);
			io_sti();
			putstr_asc(buf_back, binfo->scrnx, 0, 80, WHITE, "3[sec]");
			sheet_refresh(sht_back, 0, 80, 56, 96);
		}
		else if(fifo_status(&timerfifo3) != 0){ //模拟光标
			i = fifo_get(&timerfifo3);
			io_sti();
			if(i != 0){
				timer_init(timer3, &timerfifo3, 0);
				boxfill8(buf_back, binfo->scrnx, WHITE, 8, 96, 15, 111);
			}
			else{
				timer_init(timer3, &timerfifo3, 1);
				boxfill8(buf_back, binfo->scrnx, BACK, 8, 96, 15, 111);
			}
			timer_settime(timer3, 50);
			sheet_refresh(sht_back, 8, 96, 16, 112);
		}
	}
}


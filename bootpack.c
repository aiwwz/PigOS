#include <stdio.h>
#include "bootpack.h"

void putstr_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void task_b_main();

void HariMain(){
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADR_BOOTINFO;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN*)MEMMAN_ADDR;
	struct SHEET *sht_back, *sht_mouse, *sht_win;
	struct SHTCTL *shtctl;
	struct FIFO fifo;
	struct TIMER *timer, *timer2, *timer3;
	char s[40];
	int fifobuf[128];
	unsigned char buf_mouse[256], *buf_back, *buf_win; //sheet.buf
	int mx, my, wx, wy, i, cursor_x,  cursor_c;
	unsigned int memtotal;
	//键盘扫描码
	static char keytable[0x54] = { 
		  0,   0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',   0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',   0,   0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';','\'',   0,   0,'\\', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/',   0, '*',   0, ' ',   0,   0,   0,   0,   0,   0,
		  0,   0,   0,   0,   0,   0,   0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.'
	};

	//多任务
	struct TASK *task_b;
	
	//初始化GDT，IDT
	init_gdtidt();
	//初始化PIC
	init_pic();
	// IDT/PIC的初始化已经完成，于是开放CPU的中断
	io_sti();
	init_pit();
	
	fifo_init(&fifo, 128, fifobuf);
	enable_mouse(&fifo, 512, &mdec);
	init_keyboard(&fifo, 256);
	io_out8(PIC0_IMR, 0xf8); // 开放PIC1和键盘和PIT中断(11111000)
	io_out8(PIC1_IMR, 0xef); // 开放鼠标中断(11101111)	

	
	/*初始化三个计时器*/
	timer = timer_alloc();
	timer_init(timer, &fifo, 10);
	timer_settime(timer, 1000);
	timer2 = timer_alloc();
	timer_init(timer2, &fifo, 3);
	timer_settime(timer2, 300);	
	timer3 = timer_alloc();
	timer_init(timer3, &fifo, 1);
	timer_settime(timer3, 50);
	
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
	sheet_set_buf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
	sheet_set_buf(sht_mouse, buf_mouse, 16, 16, 99);
	sheet_set_buf(sht_win, buf_win, 160, 52, -1);
	make_window(buf_win, 160, 52, "Text");
	make_textbox(sht_win, 8, 28, 144, 16, WHITE);
	cursor_x = 8;//光标位置
	cursor_c = BLACK;//光标颜色
	mx = (binfo->scrnx - 16) / 2;	//求画面中心坐标
	my = (binfo->scrny - 19 - 16) / 2;
	sheet_slide(sht_back, 0, 0);
	sheet_slide(sht_mouse, mx, my);
	sheet_slide(sht_win, 240, 120);
	sheet_set_height(sht_back, 0);
	sheet_set_height(sht_win, 1);
	sheet_set_height(sht_mouse, 3);	
	sprintf(s, "memory: %dMB  free: %dKB", memtotal/(1024*1024), memman_total(memman)/1024);
	putstr_asc(buf_back, binfo->scrnx, 0, 45, WHITE	, s);
	sheet_refresh(sht_back, 0, 45, binfo->scrnx, 61);
	

	task_init(memman);
	task_b = task_alloc();
	task_b->tss.eip = (int)&task_b_main;
	task_b->tss.esp = memory_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
	task_b->tss.es = 1 * 8;
/*cs设置为GDT2号段，也就是本系统程序所在段是因为我们即将
调用的程序task_b_main就在本系统程序中, 只是为了测试而已*/
	task_b->tss.cs = 2 * 8; 
	task_b->tss.ss = 1 * 8;
	task_b->tss.ds = 1 * 8;
	task_b->tss.fs = 1 * 8;
	task_b->tss.gs = 1 * 8;
	*((int *)(task_b->tss.esp + 4)) = (int)sht_back; //通过栈将sht_back传给task_b_main
	task_run(task_b);
	for(;;){
		io_cli();	//屏蔽中断
		if(fifo_status(&fifo) == 0){
			io_stihlt();  //如果无事可做则取消屏蔽中断并休息
		}
		else{
			i = fifo_get(&fifo);
			io_sti();
			if(256 <= i && i <= 511){ //键盘数据
				sprintf(s, "%02X", i - 256);
				putstr_asc_sht(sht_back, 0, 25, WHITE, BACK, s, 2);
				if(i < 0x54 + 256){
					if(keytable[i - 256] != 0 && cursor_x < 144){
						s[0] = keytable[i - 256];
						s[1] = '\0';
						putstr_asc_sht(sht_win, cursor_x, 28, BLACK, WHITE, s, 1);
						cursor_x += 8;
					}
				}
				if(i == 0x0E + 256 && cursor_x > 8){ //如果是退格键切光标不在初始位置
					/*先把光标消去再退后*/
					putstr_asc_sht(sht_win, cursor_x, 28, WHITE, WHITE, " ", 1);
					cursor_x -= 8;
				}
				/*光标再显示，不能等到时钟中断到来再显示，否则不连贯*/
				boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			}
			else if(512 <= i && i <= 767){ //鼠标数据
				if(mouse_decode(&mdec, i - 512) != 0){ //鼠标数据的三个字节集齐，显示出来 
					//mouse status
					sprintf(s, "[lbr]%02X %02X %02X", mdec.mouse_buf[0], mdec.mouse_buf[1], mdec.mouse_buf[2]);
					if(mdec.btn & 0x01){ s[1] = 'L'; }
					if(mdec.btn & 0x02){ s[3] = 'R'; }
					if(mdec.btn & 0x04){ s[2] = 'B'; }
					putstr_asc_sht(sht_back, 32, 25, WHITE, BACK, s, 13);
					//mouse
					wx = mx; wy = my; //用于记录窗口的位移
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
					putstr_asc_sht(sht_back, sht_back->bxsize - 75, sht_back->bysize - 36, WHITE, BACK, s, 9);
					sheet_slide(sht_mouse, mx, my);
					//控制窗口
					if((mdec.btn & 0x01) != 0
						&& sht_win->vx0 <= wx && wx < sht_win->vx0 + sht_win->bxsize
						&& sht_win->vy0 <= wy && wy < sht_win->vy0 + 20){ //点击上边栏区
						if(sht_win->vx0 + sht_win->bxsize - 21 <= wx 
						&& wx < sht_win->vx0 + sht_win->bxsize - 6 
						&& sht_win->vy0 + 5 <= wy 
						&& wy < sht_win->vy0 + 19){ //关闭窗口
							sheet_free(sht_win);
						}
						else{ //移动窗口
							wx = mx - wx;
							wy = my - wy;
							sheet_slide(sht_win, sht_win->vx0 + wx, sht_win->vy0 + wy);
						}
					}
				}
			}
			else if(i == 10){
				putstr_asc_sht(sht_back, 0, 64, WHITE, BACK, "10[sec]", 7);

			}
			else if(i == 3){
				putstr_asc_sht(sht_back, 0, 80, WHITE, BACK, "3[sec]", 6);
			}
			else if(i <= 1){ //光标
				if(i == 1){ 
					timer_init(timer3, &fifo, 0);
					cursor_c = WHITE;
				}
				else{
					timer_init(timer3, &fifo, 1);
					cursor_c = BLACK;
				}
				timer_settime(timer3, 50);
				boxfill8(sht_win->buf, sht_win->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
				sheet_refresh(sht_win, cursor_x, 28, cursor_x + 8, 44);
			}
		}
	}
}
/******************************
先涂上背景色，再在上面写上字符
x,y :显示位置的坐标
c	:字符颜色(color)
b	:背景颜色(back color)
s	:字符串地址
l 	:字符串长度
*******************************/
void putstr_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l){
	boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
	putstr_asc(sht->buf, sht->bxsize, x, y, c, s);
	sheet_refresh(sht, x, y, x + l * 8, y + 16);
	return;
}

void task_b_main(struct SHEET *sht_back){
	struct FIFO fifo;
	struct TIMER *timer_put, *timer_speed;
	int i, fifobuf[128], count = 0, count0 = 0;
	
	char s[20];
	fifo_init(&fifo, 128, fifobuf);
	timer_put = timer_alloc();
	timer_init(timer_put, &fifo, 1);
	timer_settime(timer_put, 1);
	timer_speed = timer_alloc();
	timer_init(timer_speed, &fifo, 2);
	timer_settime(timer_speed, 100); //每秒更新一次当前的速度
	for(;;){
		count++;
		io_cli();
		if(fifo_status(&fifo) == 0){
			io_sti();
		}
		else{
			i = fifo_get(&fifo);
			io_sti();
			if(i == 1){
				sprintf(s, "%10d", count);
				putstr_asc_sht(sht_back, 10, 200, WHITE, BACK, s, 10);
				timer_settime(timer_put, 1);
			}
			else if(i == 2){
				sprintf(s, "%10dHZ", count - count0);
				putstr_asc_sht(sht_back, 10, 216, WHITE, BACK, s, 13);
				timer_settime(timer_speed, 100);
				count0 = count;
			}
		}
	}
}
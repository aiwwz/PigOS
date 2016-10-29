#include <stdio.h>
#include "bootpack.h"

struct MOUSE_DEC{
	unsigned char mouse_buf[3], mouse_phase;
	int btn, x, y;
};

extern struct FIFO keyfifo, mousefifo;
//void wait_KBC_sendready();
void init_keyboard();
void enable_mouse();
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);

void HariMain(){
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADR_BOOTINFO;
	struct MOUSE_DEC mdec;
	char s[40], mcursor[256], keybuf[128], mousebuf[128];
	int mx, my, i;

	//初始化GDT，IDT
	init_gdtidt();
	//初始化PIC
	init_pic();
	io_sti();

	fifo_init(&keyfifo,  128, keybuf);
	fifo_init(&mousefifo, 128, mousebuf);
	
	io_out8(PIC0_IMR, 0xf9); // 接收PIC1和键盘中断的许可(11111001)
	io_out8(PIC1_IMR, 0xef); // 接受鼠标中断的许可(11101111)
	init_keyboard();
	
	//初始化自定义的调色板
	init_palette();
	//初始化屏幕
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
	
	mx = (binfo->scrnx - 16) / 2;	//求画面中心坐标
	my = (binfo->scrny - 19 - 16) / 2;
	//显示鼠标
	init_mouse_cursor(mcursor, BACK);
	putblock(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
		
	enable_mouse();
	mdec.mouse_phase = 0;
	
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
						sprintf(s, "%02X %02X %02X", mdec.mouse_buf[0], mdec.mouse_buf[1], mdec.mouse_buf[2]);
						boxfill8(binfo->vram, binfo->scrnx, BACK, 32, 25, 31 + 8 * 8, 40);
						putstr_asc(binfo->vram, binfo->scrnx, 32, 25, BLACK, s);
						//mouse coordinate
						mx += mdec.x;
						my += mdec.y;
						sprintf(s, "(%d,%d)", mx, my);
						boxfill8(binfo->vram, binfo->scrnx, BACK, binfo->scrnx - 75, binfo->scrny - 36, binfo->scrnx - 75 + 9 * 8 - 1, binfo->scrny - 21);
						putstr_asc(binfo->vram, binfo->scrnx, binfo->scrnx - 75, binfo->scrny - 36, BLACK, s);
					}
				}	
	}
}

#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47


void wait_KBC_sendready(){
	for(;;){
		if((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0){
			break;
		}
	}
	return;
}

void init_keyboard(){
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE); //set mode
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE); //use mouse mode
	return;
}
#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

void enable_mouse(){
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	
	return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data){
	if(mdec->mouse_phase == 0){
		if(data == 0xfa)//等待鼠标的0xfa状态
			mdec->mouse_phase = 1;
		return 0;
	}
	if(mdec->mouse_phase == 1){
		mdec->mouse_buf[0] = data;
		mdec->mouse_phase = 2;
		return 0;
	}
	if(mdec->mouse_phase == 2){
		mdec->mouse_buf[1] = data;
		mdec->mouse_phase = 3;
		return 0;
	}
	if(mdec->mouse_phase == 3){
		mdec->mouse_buf[2] = data;
		mdec->mouse_phase = 1;
		mdec->btn = mdec->mouse_buf[0] & 0x07;
		mdec->x = (char)mdec->mouse_buf[1]; //转化为带符号类型
		mdec->y = (char)mdec->mouse_buf[2];
		mdec->y = - mdec->y;
		return 1;
	}
	return -1;
}

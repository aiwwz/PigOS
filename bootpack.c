#include <stdio.h>
#include "bootpack.h"

void HariMain(){
	struct BOOTINFO *binfo = (struct BOOTINFO*) ADR_BOOTINFO;
	char s[40], mcursor[256];
	int mx, my;

	//初始化GDT，IDT
	init_gdtidt();
	//初始化PIC
	init_pic();
	
	io_sti();
	
	//初始化自定义的调色板
	init_palette();
	//初始化屏幕
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
	
	mx = (binfo->scrnx - 16) / 2;	//求画面中心坐标
	my = (binfo->scrny - 19 - 16) / 2;
	//显示鼠标
	init_mouse_cursor(mcursor, 15);
	putblock(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	
	//显示鼠标的坐标
	sprintf(s, "(%d,%d)", mx, my);
	putstr_asc(binfo->vram, binfo->scrnx, binfo->scrnx - 75, binfo->scrny - 36, 0, s);
	
	io_out8(PIC0_IMR, 0xf9); // 接收PIC1和键盘中断的许可(11111001)
	io_out8(PIC1_IMR, 0xef); // 接受鼠标中断的许可(11101111)

	for(;;)
		io_hlt;
}

/*中断处理*/

#include "bootpack.h"

/*PIC初始化*/
void init_pic(){
	io_out8(PIC0_IMR,  0xff ); //禁止所有中断
	io_out8(PIC1_IMR,  0xff ); //禁止所有中断
	
	io_out8(PIC0_ICW1, 0x11 ); //边沿触发模式(edge trigger mode)
	io_out8(PIC0_ICW2, 0x20 ); //IRQ0-7由INT 0x20-0x27接收
	io_out8(PIC0_ICW3, 0x04 ); //PIC1由IRQ2连接,对于主PIC，第几号IRQ与从PIC相连用8位来设定
	io_out8(PIC0_ICW4, 0x01 ); //无缓冲区模式
	
	io_out8(PIC1_ICW1, 0x11 ); //边沿触发模式(edge trigger mode)
	io_out8(PIC1_ICW2, 0x28 ); //IRQ8-15由INT 0x28-0x2f接收
	io_out8(PIC1_ICW3, 2    ); //PIC1由IRQ2连接，对于从PIC，第几号IRQ与主PIC相连用三位设定
	io_out8(PIC1_ICW4, 0x01 ); //无缓冲区模式
	
	io_out8(PIC0_IMR,  0xfb ); //11111011 PIC1以外全部禁止
	io_out8(PIC1_IMR,  0xff ); //11111111 禁止所有中断
	
	return;
}

/*键盘中断处理程序(0x21号)*/
void inthandler21(int *esp){
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8(binfo->vram, binfo->scrnx, 3, 0, 30, 32 * 8 - 1, 45);
	putstr_asc(binfo->vram, binfo->scrnx, 0, 30, 0, "INT 21 (IRQ -1) : PS/2 keyboard");
	for(;;){
		io_hlt();
	}
}

/*鼠标中断处理程序(0x2c号)*/
void inthandler2c(int *esp){
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8(binfo->vram, binfo->scrnx, 3, 0, 30, 32 * 8 - 1, 45);
	putstr_asc(binfo->vram, binfo->scrnx, 0, 30, 0, "INT 2C (IRQ-12) : PS/2 mouse");
	for (;;) {
		io_hlt();
	}
}

void inthandler27(int *esp)
/* PIC0からの不完全割り込み対策 */
/* Athlon64X2機などではチップセットの都合によりPICの初期化時にこの割り込みが1度だけおこる */
/* この割り込み処理関数は、その割り込みに対して何もしないでやり過ごす */
/* なぜ何もしなくていいの？
	→  この割り込みはPIC初期化時の電気的なノイズによって発生したものなので、
		まじめに何か処理してやる必要がない。									*/
{
	io_out8(PIC0_OCW2, 0x67); /* IRQ-07受付完了をPICに通知(7-1参照) */
	return;
}
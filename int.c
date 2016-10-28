/*中断处理*/
#include <stdio.h>
#include "bootpack.h"

#define PORT_KEYDAT		0x0060	//端口号0x0060是键盘设备

/*键盘缓冲区*/
struct FIFO keyfifo;
/*鼠标缓冲区*/
struct FIFO mousefifo;

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
	unsigned char data;
	io_out8(PIC0_OCW, 0x61); //通知PIC0 IRQ-01已经处理完毕
	data = io_in8(PORT_KEYDAT);	//从键盘读入信息
	fifo_put(&keyfifo, data);
	return;
}

/*鼠标中断处理程序(0x2c号)*/
void inthandler2c(int *esp){
	unsigned char data;
	io_out8(PIC1_OCW, 0x64); //通知PIC1 IRQ-01已经处理完毕
	io_out8(PIC0_OCW, 0x62); //通知PIC0 IRQ-02已经处理完毕
	data = io_in8(PORT_KEYDAT);
	fifo_put(&mousefifo, data);
	return;
}

void inthandler27(int *esp)
/* PIC0からの不完全割り込み対策 */
/* Athlon64X2機などではチップセットの都合によりPICの初期化時にこの割り込みが1度だけおこる */
/* この割り込み処理関数は、その割り込みに対して何もしないでやり過ごす */
/* なぜ何もしなくていいの？
	→  この割り込みはPIC初期化時の電気的なノイズによって発生したものなので、
		まじめに何か処理してやる必要がない。									*/
{
	io_out8(PIC0_OCW, 0x67); /* IRQ-07受付完了をPICに通知(7-1参照) */
	return;
}
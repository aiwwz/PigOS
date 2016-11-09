#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(){
	int i;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	for(i = 0; i < MAX_TIMER; i++){
		timerctl.timer[i].flags  = 0; //定时器未使用
	}
	return;
}

/*时钟中断处理程序(0x20号)*/
void inthandler20(int *esp){
	int i;
	io_out8(PIC0_OCW, 0x60);
	timerctl.count++;
	for(i = 0; i < MAX_TIMER; i++){
		if(timerctl.timer[i].flags == TIMER_FLAGS_USING){
			timerctl.timer[i].timeout--;
			if(timerctl.timer[i].timeout == 0){
				timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
				fifo_put(timerctl.timer[i].fifo, timerctl.timer[i].data);
			}
		}
	}
	return;
}

struct TIMER* timer_alloc(){
	int i;
	for(i = 0; i < MAX_TIMER; i++){
		if(timerctl.timer[i].flags == 0){
			timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timer[i];
		}
	}
	return 0;
}

void timer_free(struct TIMER *timer){
	timer->flags = 0;
	return;
}

void timer_init(struct TIMER *timer, struct FIFO *fifo, unsigned char data){
	timer->fifo = fifo;
	timer->data = data;
	return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout){
	timer->timeout = timeout;
	timer->flags = TIMER_FLAGS_USING;
	return;
}


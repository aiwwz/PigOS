#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(){
	int i;
	struct TIMER *post; //哨兵
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	for(i = 0; i < MAX_TIMER; i++){
		timerctl.timers[i].flags  = 0; //计时器未使用
	}
	/*创建哨兵*/
	post = timer_alloc();
	post->timeout = 0xffffffff;
	post->flags = TIMER_FLAGS_ALLOC;
	post->next_timer = NULL;
	timerctl.timer0 = post;
	timerctl.next_time = post->timeout; //next_time == 0xffffffff
	
	return;
}

/*时钟中断处理程序(0x20号)*/
void inthandler20(int *esp){
	char ts = 0;
	struct TIMER *timer;
	io_out8(PIC0_OCW, 0x60);
	timerctl.count++; //计时器主变量
	if(timerctl.next_time > timerctl.count){
		return;
	}
	
	for(timer = timerctl.timer0; timer->timeout <= timerctl.count; timer = timer->next_timer){
		//超时
		timer->flags = TIMER_FLAGS_ALLOC;
		if(timer != task_timer){ //非多道程序的时间片到
			fifo_put(timer->fifo, timer->data);	
		}
		else{
			ts = 1;
		}
	}
	timerctl.timer0 = timer;
	timerctl.next_time = timerctl.timer0->timeout;
	if(ts == 1){
		task_switch();
	}
	return;
}

struct TIMER* timer_alloc(){
	int i;
	for(i = 0; i < MAX_TIMER; i++){
		if(timerctl.timers[i].flags == 0){
			timerctl.timers[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers[i];
		}
	}
	return 0;
}

void timer_free(struct TIMER *timer){
	timer->flags = 0;
	return;
}

void timer_init(struct TIMER *timer, struct FIFO *fifo, int data){
	timer->fifo = fifo;
	timer->data = data;
	return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout){
	int flags;
	struct TIMER *s , *t;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
	flags = io_load_eflags();
	io_cli();
	/*因为有哨兵，所以要么最前，要么中间*/
	//插在最前面
	if(timer->timeout <= timerctl.timer0->timeout){
		timer->next_timer = timerctl.timer0;
		timerctl.timer0 = timer;
		timerctl.next_time = timer->timeout;
		io_store_eflags(flags);
		return;
	}
	//不在最前面，找位子
	t = timerctl.timer0;
	for(;;){
		s = t;
		t = t->next_timer;
		if(timer->timeout <= t->timeout){
			/*插入到s和t之间*/
			s->next_timer = timer;
			timer->next_timer = t;
			io_store_eflags(flags); //其他的不变
			return;
		}
	}
}

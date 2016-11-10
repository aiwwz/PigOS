#include "bootpack.h"

struct TIMERCTL timerctl;

void init_pit(){
	int i;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	timerctl.next_time = 0xffffffff; //因为现在没有正在运行的计时器
	timerctl.using = 0;
	for(i = 0; i < MAX_TIMER; i++){
		timerctl.timers[i].flags  = 0; //计时器未使用
	}
	return;
}

/*时钟中断处理程序(0x20号)*/
void inthandler20(int *esp){
	int i;
	struct TIMER *timer;
	io_out8(PIC0_OCW, 0x60);
	timerctl.count++; //计时器主变量
	if(timerctl.next_time > timerctl.count){
		return;
	}
	timer = timerctl.timer0;
	for(i = 0; i < timerctl.using; i++){
		//timers的定时器都处于动作中，所以不确认flags
		if(timer->timeout > timerctl.count){
			break;
		}
		else{
			//超时
			timer->flags = TIMER_FLAGS_ALLOC;
			fifo_put(timer->fifo, timer->data);
			timer = timer->next_timer;
		}
	}
	timerctl.using -= i;
	timerctl.timer0 = timer;
	if(timerctl.using > 0){
		timerctl.next_time = timerctl.timer0->timeout;
	}
	else{
		timerctl.next_time = 0xffffffff;	
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
	int flags, i, j;
	struct TIMER *s , *t;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
	flags = io_load_eflags();
	io_cli();
	timerctl.using++;
	/*处于运行的只有添加的这一个*/
	if(timerctl.using == 1){
		timerctl.timer0 = timer;
		timer->next_timer = NULL;
		timerctl.next_time = timerctl.timer0->timeout;
		io_store_eflags(flags);
		return;
	}
	/*处于运行的不止添加的这一个*/
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
		if(t == NULL){
			break;
		}
		if(timer->timeout <= t->timeout){
			/*插入到s和t之间*/
			s->next_timer = timer;
			timer->next_timer = t;
			io_store_eflags(flags); //其他的不变
			return;
		}
	}
	//只剩末尾啦
	s->next_timer = timer;
	timer->next_timer = NULL;	
	io_store_eflags(flags);
	
	return;
}













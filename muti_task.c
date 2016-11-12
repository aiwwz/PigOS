#include "bootpack.h"
struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman){
	int i;
	struct TASK *task;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR*)ADR_GDT;
	taskctl = (struct TASKCTL *)memory_alloc_4k(memman, sizeof(struct TASKCTL));
	for(i = 0; i < MAX_TASKS; i++){
		taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
		taskctl->tasks0[i].flags = 0;
		set_segdesc(gdt + TASK_GDT0 + i, 103, (int)&taskctl->tasks0[i].tss, AR_TSS32);
	}
	task = task_alloc();
	task->flags = 2; //活动中标志
	taskctl->running = 1;
	taskctl->now = 0;
	taskctl->tasks[0] = task;
	load_tr(task->sel);
	task_timer = timer_alloc();
	timer_settime(task_timer, 2);
	return task;
}

struct TASK *task_alloc(){
	int i;
	struct TASK *task;
	for(i = 0; i < MAX_TASKS; i++){
		if(taskctl->tasks0[i].flags == 0){
			task = &taskctl->tasks0[i];
			task->flags = 1; //正在使用中标志
			task->tss.eflags = 0x000000202;
			task->tss.eax = 0;
			task->tss.ecx = 0;
			task->tss.edx = 0;
			task->tss.ebx = 0;
			task->tss.ebp = 0;
			task->tss.esi = 0;
			task->tss.edi = 0;
			task->tss.es = 0;
			task->tss.cs = 0;
			task->tss.ss = 0;
			task->tss.ds = 0;
			task->tss.fs = 0;
			task->tss.gs = 0;
			task->tss.ldtr = 0;
			task->tss.iomap = 0x40000000;
			return task;  
		}
	}
	return 0;  //无法继续添加任务
}

void task_run(struct TASK *task){
	task->flags = 2;  //活动中标志
	taskctl->tasks[taskctl->running++] = task;
	return;
}

void task_switch(){
	timer_settime(task_timer, 2);
	if(taskctl->running >= 2){
		if(++taskctl->now == taskctl->running){
			taskctl->now = 0;
		}
		taskswitch(0, taskctl->tasks[taskctl->now]->sel);
	}
	return; //只有一个运行中的任务则不切换
}
 
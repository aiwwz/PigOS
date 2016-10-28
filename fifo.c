/*设备缓冲区*/
#include "bootpack.h"

void fifo_init(struct FIFO *fifo, int size, unsigned char *buf){
	fifo->buf = buf;
	fifo->next_w = 0;
	fifo->next_r = 0;
	fifo->size = size;	//size of buffer.
	fifo->free = size;
	fifo->flags = 0;	
	return;
}

int fifo_put(struct FIFO *fifo, unsigned char data){
	if(fifo->free == 0){
		fifo->flags = 1; //overflow!
		return -1;
	}
	else{
		fifo->buf[fifo->next_w] = data;
		fifo->next_w++;
		if(fifo->next_w == fifo->size)
			fifo->next_w = 0;
		fifo->free--;
	}
	return 0;
}

int fifo_get(struct FIFO *fifo){
	int data;
	if(fifo->free == fifo->size){
		return -1;   //buffer is empty.
	}
	data = fifo->buf[fifo->next_r];
	if(++fifo->next_r == fifo->size)
		fifo->next_r = 0;
	fifo->free++;
	return data;
}

int fifo_status(struct FIFO *fifo){
	return fifo->size - fifo->free;
}
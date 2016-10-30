/*keyboard_mouse*/

#include "bootpack.h"

//keyboard
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
 
 
 //mouse
 void enable_mouse(struct MOUSE_DEC *mdec){
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->mouse_phase = 0;
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
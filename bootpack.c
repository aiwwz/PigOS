//定义在naskfunc.nas中
void io_hlt();	//CPU休息
void io_cli();	//设置io中断
void io_out8(int port, int data);	//
int io_load_eflags();
void io_store_eflags(int eflags);
//定义在本文件中
void init_palette(void); 	//palette  n.调色板
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen(char *vram,int x, int y);
void putfont(char *vram, int xsize, int x, int y, char c, char *font);

struct BOOTINFO{
	char cyls, leds, vmode, reserve;  //reserve保留，尚无意义
	short scrnx, scrny;
	char *vram;
};

void HariMain(){
	struct BOOTINFO *binfo = (struct BOOTINFO*) 0x0ff0;
	
	//初始化自定义的调色板
	init_palette();
	
	//初始化屏幕
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
	
	//字符A
	static char font_A[16] = {
		0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24,
		0x24, 0x7e, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
	};
	//显示字符A
	putfont(binfo->vram, binfo->scrnx, 50, 50, 3, font_A);
	
	for(;;)
		io_hlt;
}

void init_palette(){
	/*这里设为static unsigned一是为了防止赋值，直接存储，
	  二是为了防止char将0xff判断为-1。*/
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,	/*  0:黒   	*/
		0xff, 0x00, 0x00,	/*  1:亮红 	*/
		0x00, 0xff, 0x00,	/*  2:亮绿 	*/
		0xff, 0xff, 0x00,	/*  3:亮黄 	*/
		0x00, 0x00, 0xff,	/*  4:亮蓝 	*/
		0xff, 0x00, 0xff,	/*  5:亮紫 	*/
		0x00, 0xff, 0xff,	/*  6:浅亮蓝*/
		0xff, 0xff, 0xff,	/*  7:白 	*/
		0xc6, 0xc6, 0xc6,	/*  8:亮灰  */
		0x84, 0x00, 0x00,	/*  9:暗红  */
		0x00, 0x84, 0x00,	/* 10:暗绿  */
		0x84, 0x84, 0x00,	/* 11:暗黄  */
		0x00, 0x00, 0x84,	/* 12:暗青  */
		0x84, 0x00, 0x84,	/* 13:暗紫  */
		0x00, 0x84, 0x84,	/* 14:浅暗蓝*/
		0x84, 0x84, 0x84	/* 15:暗灰  */
	};
	set_palette(0, 15, table_rgb);
	return;
}

void set_palette(int start, int end, unsigned char *rgb){
	int i, eflags;
	eflags = io_load_eflags();	//记录中断许可标注的值
	io_cli();					//将中断许可标志置0，禁止中断
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags(eflags);	//恢复许可标志的值
	return;
}
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1){
	int x, y;
	for(x = x0; x <= x1; ++x)
		for(y = y0; y <= y1; ++y)
			vram[x + y * xsize] = c;
	return;
}

void init_screen(char *vram, int x, int y){
	//主面板
	boxfill8(vram, x,	15,	0,       0,	x - 1,	y - 20);
	//任务栏
	boxfill8(vram, x,	8,	0,  y - 19,	x - 1,	y - 19);
	boxfill8(vram, x,	7,	0,  y - 18,	x - 1,	y - 18);
	boxfill8(vram, x,	8,	0,  y - 17,	x - 1,	y -  1);
	//button
	boxfill8(vram, x,	7,	3,  y - 15,	59,		y - 15);
	boxfill8(vram, x, 	7,	2,  y - 15,	 2, 	y -  4);
	boxfill8(vram, x,  15,	3,  y -  4,	59,		y -  4);
	boxfill8(vram, x,  15, 59,  y - 15,	59,		y -  5);
	boxfill8(vram, x,	0,	2,  y -  3,	59,		y -  3);
	boxfill8(vram, x,	0, 60,  y - 15,	60,		y -  3);
}

void putfont(char *vram, int xsize, int x, int y, char c, char *font){
	int i;
	char d; //data
	char *p;
	for(i = 0; i <= 16; ++i){
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
}
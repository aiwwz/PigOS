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

#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15

void HariMain(){
	char *vram;
	
	init_palette();  //初始化自定义的调色板
	
	vram = (char*)0xa0000;	
	int xsize = 320, ysize = 200;
	//主面板
	boxfill8(vram, xsize, COL8_008484,  0,         0,          xsize -  1, ysize - 20);
	//任务栏
	boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 19, xsize -  1, ysize - 19);
	boxfill8(vram, xsize, COL8_FFFFFF,  0,         ysize - 18, xsize -  1, ysize - 18);
	boxfill8(vram, xsize, COL8_C6C6C6,  0,         ysize - 17, xsize -  1, ysize -  1);
	//button
	boxfill8(vram, xsize, COL8_FFFFFF,  3,         ysize - 15, 59,         ysize - 15);
	boxfill8(vram, xsize, COL8_FFFFFF,  2,         ysize - 15,  2,         ysize -  4);
	boxfill8(vram, xsize, COL8_848484,  3,         ysize -  4, 59,         ysize -  4);
	boxfill8(vram, xsize, COL8_848484, 59,         ysize - 15, 59,         ysize -  5);
	boxfill8(vram, xsize, COL8_000000,  2,         ysize -  3, 59,         ysize -  3);
	boxfill8(vram, xsize, COL8_000000, 60,         ysize - 15, 60,         ysize -  3);
	
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


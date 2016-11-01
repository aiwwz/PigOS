#include "bootpack.h"
 
void make_window(unsigned char *buf, int xsize, int ysize, char *title){
	static char closebtn[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};
	int x, y;
	char c;
	boxfill8(buf, xsize, LIGHT_GRAY, 	0,		0,  xsize - 1, 0        );
	boxfill8(buf, xsize, WHITE, 		1,		1,  xsize - 2, 1        );
	boxfill8(buf, xsize, LIGHT_GRAY,	0,		0,          0, ysize - 1);
	boxfill8(buf, xsize, WHITE,			1,		1,          1, ysize - 2);
	boxfill8(buf, xsize, BACK, 	xsize - 2,		1,  xsize - 2, ysize - 2);
	boxfill8(buf, xsize, BLACK, xsize - 1, 		0,  xsize - 1, ysize - 1);
	boxfill8(buf, xsize, LIGHT_GRAY, 	2,      2,  xsize - 3, ysize - 3);
	boxfill8(buf, xsize, 12, 			3,      3,  xsize - 4, 		  20);
	boxfill8(buf, xsize, BACK,			1,ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, BLACK, 		0,ysize - 1, xsize - 1, ysize - 1);
	putstr_asc(buf, xsize, 24, 4, BLACK, title);
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			c = closebtn[y][x];
			if (c == '@') {
				c = BLACK;
			} else if (c == '$') {
				c = BACK;
			} else if (c == 'Q') {
				c = LIGHT_GRAY;
			} else {
				c = WHITE;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
		}
	}
	return;
 }
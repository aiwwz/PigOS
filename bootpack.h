//asmhead.nas
struct BOOTINFO{	// 0x0ff0-0x0fff
	char cyls;		// 启动区读硬盘读到何处为止
	char leds;		// 启动时键盘led状态
	char vmode;		// 显卡模式为多少位彩色
	char reserve;	// reserve保留，尚无意义
	short scrnx, scrny;	//分辨率
	char *vram;		//显存地址
};
#define ADR_BOOTINFO	0x00000ff0

//naskfunc.nas
void io_hlt();	//CPU休息
void io_cli();	//设置io中断
void io_sti();
void io_stihlt();
void io_out8(int port, int data);
int	 io_in8(int port);
int  io_load_eflags();
void io_store_eflags(int eflags);
void load_gdtr(int limit, int adrr);
void load_idtr(int limit, int adrr);
int  load_cr0();
void store_cr0(int cr0);
void asm_inthandler21();
void asm_inthandler27();
void asm_inthandler2c();
unsigned int memtest_sub(unsigned int start, unsigned int end);

//graphic.c
void init_palette(void); 	//palette  n.调色板
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen(char *vram,int x, int y);
void putfont(char *vram, int xsize, int x, int y, char c, char *font);
void putstr_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor(char *mouse);
void putblock(char *vram, int vxsize, int vysize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);
void save_back(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);
void put_back(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);
void init_old_back(char *old_back);
#define BLACK	0
#define RED		1
#define GREEN	2
#define YELLOW	3
#define BLUE	4
#define PURPLE	5
#define WHITE	7
#define LIGHT_GRAY	8
#define BACK	15

//dsctbl.c
struct SEGMENT_DESCRIPTOR{
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};
struct GATE_DESCRIPTOR{
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};
void init_gdtidt();
void set_segdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_INTGATE32	0x008e

// int.c
struct KEYBUF {
	unsigned char data[32];
	int next_r, next_w;
	int len;
};
void init_pic(void);
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);
#define PIC0_ICW1		0x0020
#define PIC0_OCW		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

// fifo.c
struct FIFO{
	unsigned char *buf;
	int next_w, next_r, size, free, flags;
};
void fifo_init(struct FIFO *fifo, int size, unsigned char *buf);
int fifo_put(struct FIFO *fifo, unsigned char data);
int fifo_get(struct FIFO *fifo);
int fifo_status(struct FIFO *fifo);

// keyboard_mouse.c
#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47
void wait_KBC_sendready();
void init_keyboard();
/*---boundary---*/
struct MOUSE_DEC{
	unsigned char mouse_buf[3], mouse_phase;
	int btn, x, y;
};
#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4
void enable_mouse(struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char data);

//memory_test.c
#define EFLAGS_AC_BIT		0x00040000
#define CRO_CACHE_DISABLE	0x60000000
#define MEMMAN_FREES 1024
#define MEMMAN_ADDR 0x003c0000
struct FREEMEM{
	unsigned int addr, size;
};
struct MEMMAN{
	int frees, losts, lostsize;
	struct FREEMEM free[MEMMAN_FREES]; //最多可分配1024段
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *mem);
unsigned int memman_total(struct MEMMAN *mem); //所剩空间合计大小
unsigned int memory_alloc(struct MEMMAN *mem, unsigned int size);
int memory_free(struct MEMMAN *mem, unsigned int addr, unsigned int size);


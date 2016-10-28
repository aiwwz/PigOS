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
void io_out8(int port, int data);
int  io_load_eflags();
void io_store_eflags(int eflags);
void load_gdtr(int limit, int adrr);
void load_idtr(int limit, int adrr);
void asm_inthandler21();
void asm_inthandler27();
void asm_inthandler2c();

//graphic.c
void init_palette(void); 	//palette  n.调色板
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen(char *vram,int x, int y);
void putfont(char *vram, int xsize, int x, int y, char c, char *font);
void putstr_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor(char *mouse, char bc);
void putblock(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize);

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

/* int.c */
void init_pic(void);
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

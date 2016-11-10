; PigOS--OS boot asm
; Tab = 4

BOTPAK	EQU		0x00280000
DSKCAC	EQU		0x00100000
DSKCAC0	EQU		0x00008000

; 有关BOOT_INFO
CYLS	EQU		0x0ff0		; 设定启动区
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2		; 关于颜色数目的信息。颜色的位数。
SCRNX	EQU		0x0ff4		; 分辨率X (screen x)
SCRNY	EQU		0x0ff6		; 分辨率Y (screen y)
VRAM	EQU		0x0ff8		; 图像缓冲区的开始地址

	ORG		0xc200
;画面模式设定	
	MOV		BX,0x4101			; VBA显卡，640*480*8位彩色
	MOV 	AX,0x4f02
	INT		0x10
	MOV		BYTE [VMODE],8	; 记录画面模式
	MOV		WORD [SCRNX],640
	MOV		WORD [SCRNY],480
	MOV		DWORD [VRAM],0xe0000000

; 用BIOS取得键盘上各种LED指示灯的状态
	MOV		AH,0x02
	INT		0x16			; keyboard BIOS
	MOV		[LEDS],AL
	
; PIC关闭一切中断，根据AT兼容机的规格，如果要初始化PIC
; 必须在CLI之前进行， 否则有时会挂起，随后进行PIC的初始化

		MOV		AL,0xff
		OUT		0x21,AL
		NOP						; 如果连续执行OUT指令，有些机种会无法正常运行
		OUT		0xa1,AL
		CLI						; 禁止CPU级别的中断

; 相当于以下C程序：
; io_out(PIC0_IMR, 0xff); // 禁止主PIC的全部中断
; io_out(PIC1_IMR, 0xff); // 禁止从PIC的全部中断
; io_cli(); //禁止CPU级别的中断

; 为了让CPU能访问1MB以上的内存空间，设定A20GATE

		CALL	waitkbdout
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout

; 相当于以下C程序：
; #define KEYCMD_WRITE_OUTPORT		0xd1
; #define KBC_OUTPORT_A20G_ENABLE	0xdf
; //A20GATE的设定
; wait_KBC_sendready();
; io_out8(PORT_KEYCMD, KEYCMD_WRITE_OUTPORT);
; wait_KBC_sendready();
; //让A20GATE信号线变成ON的状态, 使内存1MB以上的部分变为可使用状态。
; io_out8(PORT_KEYCMD, KBC_OUTPORT_A20G_ENABLE); 
; wait_KBC_sendready(); //为了等待A20GATE的处理完成，其实是多余的
		
		
; 切换到保护模式

[INSTRSET "i486p"]				; “想要使用486指令”的叙述，为了能使用386以后的LGDT,EAX,CR0等关键字

		LGDT	[GDTR0]			; 设定临时GDT
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; 设定bit31为0（为了禁止颂）
		OR		EAX,0x00000001	; 设定bit0为1（为了切换到保护模式）
		MOV		CR0,EAX
		JMP		pipelineflush
pipelineflush:
		MOV		AX,1*8			;  可以读写的段 32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; bootpack的传送

		MOV		ESI,bootpack	; 传送源
		MOV		EDI,BOTPAK		; 传送目的地址
		MOV		ECX,512*1024/4
		CALL	memcpy

; 硬盘数据最终传送到它本来的位置去

; 首先从启动区开始

		MOV		ESI,0x7c00		; 传送源
		MOV		EDI,DSKCAC		; 传送目的地址
		MOV		ECX,512/4
		CALL	memcpy

; 剩下的全部

		MOV		ESI,DSKCAC0+512	; 传送源
		MOV		EDI,DSKCAC+512	; 传送目的地址
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; 从柱面数变换为字节数/4
		SUB		ECX,512/4		; 减去IPL
		CALL	memcpy
		
; 相当于以下C程序：
; memcpy(bootpack, BOTPAK, 512*1024/4); //从bootpack的地址开始的512KB内容复制到0x00280000号地址中去
; memcpy(0x7c00,   DSKCAC, 512/4); //从0x7c00复制512字节到0x00100000，即将启动扇区复制到1MB以后的内存中去
; memcpy(DSKCAC0+512, cyls * 512*18*2/4 - 512/4); //将始于0x00008200的磁盘内容复制到0x00100200去

; 必须由asmhead来完成的工作，至此全部完成！

;以后就由bootpack来接班！
; bootpack的启动

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 没有要传送的东西时
		MOV		ESI,[EBX+20]	; 传送源
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 传送目的地址
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; 栈的初始值
		JMP		DWORD 2*8:0x0000001b

waitkbdout:
		IN		AL,0x64
		AND		AL,0x02
		IN		AL,0x60			; 空读（为了清空数据接收缓冲区中的垃圾数据）
		JNZ		waitkbdout		; AND的结果如果不是0，就跳到waitkbdout
		RET

; 复制内存的程序
memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4	; 一次复制4个字节
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			
		RET


		ALIGNB	16
GDT0:
		RESB	8				; NULL selector
		DW		0xffff,0x0000,0x9200,0x00cf	; 可读写的段
		DW		0xffff,0x0000,0x9a28,0x0047	; 可执行的段（bootpack用）

		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:

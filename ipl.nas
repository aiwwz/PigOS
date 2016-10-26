; PigOS-ipl
; TAB=4
		ORG		0x7c00			; ORG指明程序的装载地址 

; 以下这段程序是标准FAT12格式软盘专用的代码
		JMP		entry
		DB		0x90
		DB		" PigOS  "		; 启动区的名称可以是任意的字符串（8字节）
		DW		512				; 每个扇区的大小（必须是512字节）
		DB		1				; 簇的大小（必须是1个扇区）
		DW		1				; FAT的起始位置（一般从第一个扇区开始）
		DB		2				; FAT的个数（必须为2）
		DW		224				; 根目录的大小（一般设成224项）
		DW		2880			; 该磁盘的大小（必须是2280扇区）
		DB		0xf0			; 磁盘的种类（必须是0xf0）
		DW		9				; FAT的长度（必须是9扇区）
		DW		18				; 1个磁道有几个扇区（必须是18个）
		DW		2				; 磁头数（必须是2）
		DD		0				; 不使用分区，必须是0
		DD		2880			; 重写一次磁盘大小
		DB		0,0,0x29		; 意义不明 ，固定
		DD		0xffffffff		; （可能是）卷标号码
		DB		"HARIBOTEOS "	; 磁盘的名称（11字节）
		DB		"FAT12   "		; 磁盘格式名称（8字节）
		RESB	18				; 先空出18字节

; 程序核心
entry:
		MOV		AX,0			; 初始化寄存器
		MOV		SS,AX
		MOV		SP,0x7c00		; 指向启动区
		MOV		DS,AX			; 初始化段寄存器

; 读磁盘（在这里是虚拟软盘）
		MOV		AX,0x0820
		MOV		ES,AX
		MOV		CH,0			; 柱面0
		MOV		DH,0			; 磁头0
		MOV		CL,2			; 扇区2

		MOV		AH,0x02			; AH=0x02 : 读盘
		MOV		AL,1			; 1个扇区
		MOV		BX,0
		MOV		DL,0x00			; A驱动器
		INT		0x13			; 调用磁盘BIOS
		JC		error

; 

fin:
		HLT						; 让CPU停止，等待指令
		JMP		fin				; 无限循环

error:
		MOV		SI,msg
putloop:
		MOV		AL,[SI]			; 将内存中的存储的字符赋给AL
		ADD		SI,1			; 给SI加1
		CMP		AL,0
		JE		fin				; 显示完毕，跳转到fin
		MOV		AH,0x0e			; 显示一个文字
		MOV		BX,15			; 指定字符颜色
		INT		0x10			; 调用显卡BIOS（0x10号调用）
		JMP		putloop

; 信息显示部分
msg:
		DB		0x0a, 0x0a		; 换行两次
		DB		"load error！"
		DB		0x0a			; 换行
		DB		0

		RESB	0x7dfe-$		; 填写0x00，直到0x001fe

		DB		0x55, 0xaa		; 设计者规定第一扇区最后两个字节必须为55 AA
								; 不然计算机会认为这张盘上没有所需的启动程序

; 因为启动区只需要最初的512字节，所以启动区以外的部分先注释掉								
; 以下是启动区以外部分的输出
		;DB		0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
		;RESB	4600
		;DB		0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
		;RESB	1469432

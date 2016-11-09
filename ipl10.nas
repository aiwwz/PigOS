; PigOS--IPL
; Tab = 4
CYLS	EQU		10				; 定义个宏CYLS = 10
		
		ORG		0x7c00			; ORG指明程序装载进内存的地址 

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

; 读磁盘，将磁盘指定位置的数据读入指定内存地址处
		MOV		AX,0x0820
		MOV		ES,AX			; ES:BX = 缓冲地址（指定的内存地址）
		MOV		CH,0			; 柱面0
		MOV		DH,0			; 磁头0
		MOV		CL,2			; 扇区2
readloop:
		MOV		SI,0			; 用于记录读取是否失败
retry:
		MOV		AH,0x02			; AH=0x02 : 读盘
		MOV		AL,1			; 处理1个扇区（512字节）
		MOV		BX,0
		MOV		DL,0x00			; A驱动器
		INT		0x13			; 调用磁盘BIOS
		JNC		next			; 读取成功，跳到next继续读取
		ADD		SI,1			; 若失败将SI加1
		CMP		SI,5
		JAE		error			; 错误次数超过5次，不再尝试，跳转到error
		MOV		AH,0x00			; 这三条指令复位软盘状态
		MOV		DL,0x00			; A驱动器
		INT		0x13			; 重置驱动器
		JMP		retry
next:
		MOV		AX,ES
		ADD		AX,0x0020
		MOV		ES,AX			; 把内存地址向后移512字节
		ADD		CL,1			; 将CL加1指向下一个扇区
		CMP		CL,18
		JBE		readloop		; 若CL<=18则继续读取
		MOV		CL,1			; 将扇区重置为1
		ADD		DH,1
		CMP		DH,2
		JB		readloop		; 若磁头号小于2则继续读软盘反面
		MOV		DH,0			; 若磁头号等于2则重置为0
		ADD		CH,1			; 将柱面号CH加1
		CMP		CH,CYLS
		JB		readloop		; 若柱面号<CYLS则继续读下一柱面
		
; 读入完成后跳转到系统程序
		MOV		[0x0ff0],CH
		JMP		0xc200

error:
		MOV		SI,msg
putloop:
		MOV		AL,[SI]			; 将内存中的存储的字符赋给AL（DS为默认段寄存器）
		ADD		SI,1			; 给SI加1
		CMP		AL,0
		JE		fin				; 显示完毕，跳转到fin
		MOV		AH,0x0e			; 显示一个文字
		MOV		BX,15			; 指定字符颜色
		INT		0x10			; 调用显卡BIOS（0x10号调用）
		JMP		putloop

fin:
		HLT						; 让CPU停止，等待指令
		JMP		fin				; 无限循环

; 信息显示部分
msg:
		DB		0x0a, 0x0a		; 换行两次
		DB		"load error"
		DB		0x0a			; 换行
		DB		0

		RESB	0x7dfe-$		; 填写0x00，直到0x001fe，注意0x7dfe处未被填充

		DB		0x55, 0xaa		; 设计者规定第一扇区最后两个字节必须为55 AA
								; 不然计算机会认为这张盘上没有所需的启动程序

; 因为启动区只需要最初的512字节，所以启动区以外的部分先注释掉								
; 以下是启动区以外部分的输出
		;DB		0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
		;RESB	4600
		;DB		0xf0, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00
		;RESB	1469432

; PigOS--OS
; Tab = 4
	ORG		0xc200
	
	MOV		AH,0x00
	MOV 	AL,0x13
	INT		0x10
	
fin:
	HLT
	JMP		fin
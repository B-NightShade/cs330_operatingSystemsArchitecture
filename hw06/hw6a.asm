section .data
	prompt db "Enter integer 0-99: "
	lenPrompt equ $-prompt
	errorMsg db "Inavlid input 0-99",0xa
	lenErr equ $-errorMsg

section .bss
	instr resb 1
	retStr	resb 4

section .text
	global _start
_start:

printPrompt:
	mov eax,4
	mov ebx,1
	mov ecx,prompt
	mov edx,lenPrompt
	int 80h

readUser:
	mov eax,3
	mov ebx,2
	mov ecx,instr
	mov edx,4
	int 80h
	
	cmp eax,3
	jg errMsg

	call toInteger
	call testBounds
	call toString
	call printStr
	call exitCall


toInteger:
	cmp eax,2	;if one digit input
	je ifTrue	;if true jump to label ifTrue
	cmp eax,3	;if 2 digit
	je ifTwo	;if true jump to label ifTwo

ifTrue:
	mov byte [ecx+1],0x0	;remove carriage return
	mov esi,[ecx+0]		;put first digit in esi
	sub esi,0x30		;sub 30 to turn to int
	ret

ifTwo:
	mov byte [ecx+2],0x0	;remove carraige return
	mov dl,[ecx+0]		;put first digit in dl
	mov eax,0x10
	sub dl,0x30		;sub 0x30 to get int
	mul dl
	mov bl,[ecx+1]		;put second digit in bl
	sub bl,0x30
	add al,bl		;add two values together
	mov esi,eax		;put val in esi
	ret

testBounds:
	cmp esi,-1
	jle errMsg
	cmp esi,0x99
	jg errMsg
	ret

errMsg:
	mov eax,4
	mov ebx,1
	mov ecx,errorMsg
	mov edx,lenErr
	int 80h
	call exitCall

toString:
	xor eax,eax
	mov edi,0		;used as index counter
	mov ecx,0x30		;will be used to add
	mov eax,0x10		;multiply by 10
	mul esi
	mov ebx,0x100
	div ebx			;divide quotient in al remainder edx
	add eax,ecx
	mov byte[retStr+edi],al	;store quotient
	cmp al,0x30		;check if first digit valid
	jne firstCheck
	secondDigit:
	mov eax,edx		;move to remainder
	xor edx,edx
	mov ebx,0x10
	div ebx			;div 10 quotient in al remainder edx
	add eax,ecx
	mov byte[retStr+edi],al	;store quotient
	cmp al,0x30		;check if second valid
	je secondCheck
	jne incIndex
	comeBack:
	add edx,ecx		;change last remainder to string
	mov byte[retStr+edi],dl	;move into storage array
	inc edi
	mov byte[retStr+edi],0xa	;add carraige return on end of line
	ret

firstCheck:
	inc edi		;move index forward
	jmp secondDigit	

secondCheck:
	cmp edi,0
	jne incIndex
	jmp comeBack

incIndex:
	inc edi		;move index forward
	jmp comeBack

printStr:
	mov eax,4
	mov ebx,1
	mov ecx,retStr
	mov edx,32
	int 80h
	ret

exitCall:
	mov eax,1
	mov ebx,0
	int 80h	

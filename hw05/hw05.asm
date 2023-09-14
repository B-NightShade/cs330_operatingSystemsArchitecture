section .data
	prompt db "Enter your name: "
	lenPrompt equ $-prompt
	helloMsg db "Hello "
	lenHello equ $-helloMsg
	exclamation db "!", 0xa
	lenEx equ $-exclamation

section .bss
	name resb 20

section .text
	global _start
_start:
	mov eax,4
	mov ebx,1
	mov ecx,prompt
	mov edx,lenPrompt
	int 80h

ReadName:
	mov eax,3
	mov ebx,2
	mov ecx,name
	mov edx,20
	int 80h

	mov esi, eax
	dec esi
	mov eax,4
	mov ebx,1
	mov ecx,helloMsg
	mov edx,lenHello
	int 80h

	mov eax,4
	mov ebx,1
	mov ecx,name
	mov edx,esi
	int 80h

	mov eax,4
	mov ebx,1
	mov ecx,exclamation
	mov edx,lenEx
	int 80h

	mov eax,1
	mov ebx,0
	int 80h

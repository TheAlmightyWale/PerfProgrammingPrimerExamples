global Store1x
global Store2x
global Store3x
global Store4x
global Mov1x
global Mov2x
global Mov3x
global Mov4x

section .text

Store1x:
	align 64
.loop:
	mov [rdx], rax
	sub rcx, 1
	jnle .loop
	ret

Store2x:
	align 64
.loop:
	mov [rdx], rax
	mov [rdx], rax
	sub rcx, 2
	jnle .loop
	ret

Store3x:
	align 64
.loop:
	mov [rdx], rax
	mov [rdx], rax
	mov [rdx], rax
	sub rcx, 3
	jnle .loop
	ret

Store4x:
	align 64
.loop:
	mov [rdx], rax
	mov [rdx], rax
	mov [rdx], rax
	mov [rdx], rax
	sub rcx, 4
	jnle .loop
	ret

Mov1x:
	align 64
.loop:
	mov rax, [rdx]
	sub rcx, 1
	jnle .loop
	ret

Mov2x:
	align 64
.loop:
	mov rax, [rdx]
	mov rax, [rdx]
	sub rcx, 2
	jnle .loop
	ret


	Mov3x:
	align 64
.loop:
	mov rax, [rdx]
	mov rax, [rdx]
	mov rax, [rdx]
	sub rcx, 3
	jnle .loop
	ret

	Mov4x:
	align 64
.loop:
	mov rax, [rdx]
	mov rax, [rdx]
	mov rax, [rdx]
	mov rax, [rdx]
	sub rcx, 4
	jnle .loop
	ret
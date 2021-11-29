.section .text
.text
.globl parse
.type parse, @function
parse:
	push %rbx
	movq %rcx, %r9
	movq %rsi, %r10
	movq %rdi, %r11
	movq (%rsi), %rsi
	movq (%rdi), %rdi
	call parsew
	decq %rsi
	movq %rsi, (%r10)
	movq %rdi, (%r11)
	pop %rbx
	ret
parsew:
	movl $-1, %r8d
	leaq .table(%rip), %rbx
	vmovq .LC0(%rip), %xmm1
	vmovq .LC1(%rip), %xmm2
	vmovq .LC2(%rip), %xmm5
	vmovdqa .LC3(%rip), %xmm6
	vmovdqa .LC4(%rip), %xmm7
	vmovdqa .LC5(%rip), %xmm8
	vmovdqa .LC9(%rip), %xmm9
	vmovdqa .LC10(%rip), %xmm10
	vmovdqa .LC11(%rip), %xmm11
	vmovdqa .LC12(%rip), %xmm12
	vpxor %xmm13, %xmm13, %xmm13
	movl $1920, %eax
	vpinsrw $3, %eax, %xmm13, %xmm13
	movl $1, %eax
	vpinsrw $2, %eax, %xmm13, %xmm13
	vpsllw $2, %xmm13, %xmm13
	vmovdqa .LC14(%rip), %xmm14
	movzbl %dl, %ecx
	shr $4, %rdx
	andb $0x0f, %cl
	movsxw (%rbx, %rcx, 2), %rax
	addq %rbx, %rax
	jmp *%rax
.section .rodata
.data
.table:
	.word .P0-.table
	.word .I1-.table
	.word .I2-.table
	.word .P1-.table
	.word .P2-.table
	.word .P4-.table
	.word .I5-.table
	.word .I6-.table
	.word .I7-.table
	.word .I8-.table
.section .text
.text
.I6:
	movl %edx, %eax
	shr $4, %rdx
	andl $0xf, %eax
	pdep .LI0(%rip), %rdx, %rdx
	vmovq (%rsi), %xmm0
	vpcmpistri $20, %xmm0, %xmm1
	vpcmpgtb %xmm2, %xmm0, %xmm3
	vpaddb %xmm2, %xmm0, %xmm4
	vpblendvb %xmm3, %xmm4, %xmm0, %xmm0
	vmovq %xmm0, %rbx
	bswap %rbx
	pext .LI2(%rip), %rbx, %rbx
	addq %rcx, %rsi
	addl %eax, %ecx
	shlb $2, %al
	shrx %rax, %rbx, %rbx
	xorb $0xde, %al
	shrx %rax, %rdx, %rdx
	shlx %rax, %rdx, %rdx
	orq %rbx, %rdx
	vmovq %rdx, %xmm0
	jmp .P7mid
.I5:
	movl $0x0f0f0f0f, %eax
	rorx $4, %edx, %ebx
	andl %eax, %edx
	andl %eax, %ebx
	jmp .P5
.I7:
	pdep .LI1(%rip), %rdx, %rdx
	movl $8, %ecx
	vmovq %rdx, %xmm0
	jmp .P7mid
.I8:
	pdep .LI1(%rip), %rdx, %rdx
	movl $8, %ecx
	vmovq %rdx, %xmm0
	jmp .P7lfcr
.I1:
	decq %rsi
	jmp .P01
.I2:
	leaq -2(%rsi), %rsi
	jmp .P02
.E0:
	incq %rsi
	xorl %eax, %eax
	ret
.E1:
	addq $2, %rsi
	movl $1, %eax
	ret
.E2:
	movl $2, %eax
	ret
.E3:
	movl $3, %eax
	ret
.p2align 2,0
.APPLY6:
	vpinsrb $0, %r8d, %xmm0, %xmm0
	vmovd %xmm0, (%rdx)
	jmp .P0
.APPLY:
	vpshufb %xmm9, %xmm0, %xmm3
	vpinsrd $1, (%rdx), %xmm0, %xmm0
	vpxor %xmm10, %xmm3, %xmm3
	vpmovzxbw %xmm0, %xmm0
	vpmullw %xmm0, %xmm3, %xmm0
	vpmulhuw %xmm11, %xmm0, %xmm0
	vpsllw $1, %xmm0, %xmm0
	vmovhlps %xmm3, %xmm0, %xmm3
	vpaddsw %xmm0, %xmm3, %xmm0
	vpshufb %xmm12, %xmm0, %xmm0
	vmovd %xmm0, (%rdx)
.P0:
	cmpb $0x50, 0(%rsi)
	jne .E0
.P01:
	cmpb $0x58, 1(%rsi)
	jne .E1
.P02:
	cmpb $0x20, 2(%rsi)
	leaq 3(%rsi), %rsi
	jne .E2
.P1:
	movzbl (%rsi), %eax
	incq %rsi
	cmpb $0x20, %al
	je .P1
	xorl $0x30, %eax
	cmpb $9, %al
	ja .E3
	movl %eax, %edx
.P2:
	movzbl (%rsi), %eax
	incq %rsi
	xorl $0x30, %eax
	cmpb $9, %al
	ja .P3
	rorx $24, %edx, %edx
	testb %dl, %dl
	movb %al, %dl
	je .P2
	jmp .ERR1
.P3:
	cmpb $0x10, %al
	jne .E5
.P4:
	movzbl (%rsi), %eax
	incq %rsi
	cmpb $0x20, %al
	je .P4
	xorl $0x30, %eax
	cmpb $9, %al
	ja .E6
	movl %eax, %ebx
.P5:
	movzbl (%rsi), %eax
	incq %rsi
	xorl $0x30, %eax
	cmpb $9, %al
	ja .P5post
	rorx $24, %ebx, %ebx
	testb %bl, %bl
	movb %al, %bl
	je .P5
	jmp .ERR2
	nop
	nop
	nop
.P5post:
	cmpb $0x10, %al
	je .P6
	jmp .E8
	#cmpb $0x3a, %al
	#jne .E8
	vpinsrd $2, %edx, %xmm0, %xmm0
	vpinsrd $3, %ebx, %xmm0, %xmm0
	vpmaddubsw %xmm0, %xmm6, %xmm0
	vpmaddwd %xmm0, %xmm7, %xmm0
	vpshufb %xmm8, %xmm0, %xmm0
	vpmaddwd %xmm0, %xmm13, %xmm3
	vpextrd $1, %xmm3, %eax
	shll $4, %ebx
	orl %edx, %edx
	movl (%rdi, %rax), %eax
	movl %ebx, (%r9)
	movl %eax, 4(%r9)
	addq $8, %r9
	#vmovd (%rdi, %rax), %xmm0
	#vpmovzxbw %xmm0, %xmm0
	#movl $0x2058500a, (%r9)
	#vpsllw $12, %xmm0, %xmm3
	#vpor %xmm0, %xmm3, %xmm0
	#vpsrlw $4, %xmm0, %xmm0
	#vpshufb %xmm0, %xmm14, %xmm0
	#vmovq %xmm0, 4(%r9)
	#addq $12, %r9
	jmp .P0
.P6pre:
	incq %rsi
.P6:
	cmpb $0x20, (%rsi)
	je .P6pre
.P7:
	vmovq (%rsi), %xmm0
	vpcmpistri $20, %xmm0, %xmm1
	vpcmpgtb %xmm2, %xmm0, %xmm3
	vpaddb %xmm0, %xmm2, %xmm4
	vpblendvb %xmm3, %xmm4, %xmm0, %xmm0
	vpand %xmm0, %xmm5, %xmm0
	vpinsrd $2, %edx, %xmm0, %xmm0
	vpinsrd $3, %ebx, %xmm0, %xmm0
	addq %rcx, %rsi
	vpmaddubsw %xmm0, %xmm6, %xmm0
	vpmaddwd %xmm0, %xmm7, %xmm0
	vpshufb %xmm8, %xmm0, %xmm0
.P7mid:
	vpmaddwd %xmm0, %xmm13, %xmm3
	movzbl (%rsi), %eax
	incq %rsi
	cmpb $0x0a, %al
	je .P8
	cmpb $0x0d, %al
	jne .E9
.P7lfcr:
	cmpb $0x0a, (%rsi)
	incq %rsi
	jne .E11
.P8:
	vpextrd $1, %xmm3, %edx
	addq %rdi, %rdx
	cmpb $6, %cl
	je .APPLY6
	cmpb $8, %cl
	je .APPLY
	jmp .ERR3

.ERR1:
	movl $0x1f, %eax
	ret
.E5:
	movl %edx, %eax
	shlq $4, %rax
	orb $4, %al
	ret
.E6:
	movl %edx, %eax
	shlq $4, %rax
	orb $5, %al
	ret
.ERR2:
	movl $0x2f, %eax
	ret
.E8:
	movl %edx, %eax
	shlq $4, %rax
	shlq $8, %rbx
	orq %rbx, %rax
	orb $6, %al
	ret
.E9:
	cmpb $8, %cl
	je .E10
	vmovq %xmm0, %rax
	pext .LI0(%rip), %rax, %rax
	shlb $4, %cl
	shlq $8, %rax
	movb $7, %al
	orb %cl, %al
	ret
.E10:
	vmovq %xmm0, %rax
	pext .LI1(%rip), %rax, %rax
	shlq $4, %rax
	orb $8, %al
	ret
.E116:
	vpinsrb $3, %r8d, %xmm0, %xmm0
.E118:
	vmovq %xmm0, %rax
	pext .LI1(%rip), %rax, %rax
	shlq $4, %rax
	orb $9, %al
	ret
.E11:
	cmpb $6, %cl
	je .E116
	cmpb $8, %cl
	je .E118
.ERR3:
	movl $0x3f, %eax
	ret

.section rodata
.data
.align 8
.LI0:
	.long 0xfffffff0, 0x3fff3fff
.align 8
.LI1:
	.long 0xffffffff, 0x3fff3fff
.align 8
.LI2:
	.long 0x0f0f0f0f, 0x0f0f0f0f
.align 8
.LC0:
	.byte 0x30, 0x39
	.byte 0x41, 0x46
	.byte 0x61, 0x66
	.byte 0, 0
.align 8
.LC1:
	.byte 0x39, 0x39, 0x39, 0x39, 0x39, 0x39, 0x39, 0x39
.align 8
.LC2:
	.byte 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f
.align 16
.LC3:
	.byte 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01
	.byte 0x01, 0x0a, 0x01, 0x0a, 0x01, 0x0a, 0x01, 0x0a
.align 16
.LC4:
	.word 0x0001, 0x0100, 0x0001, 0x0100
	.word 0x0001, 0x0064, 0x0001, 0x0064
.align 16
.LC5:
	.byte 0x05, 0x04, 0x01, 0x00, 0x08, 0x09, 0x0c, 0x0d
	.byte 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff

.align 16
.LC9:
	.byte 0x80, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80
	.byte 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80
.align 16
.LC10:
	.byte 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	.byte 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00
.align 16
.LC11:
	.word 0x8081, 0x8081, 0x8081, 0x8081
	.word 0x8081, 0x8081, 0x8081, 0x8081
.align 16
.LC12:
	.byte 0x01, 0x03, 0x05, 0x07, 0x01, 0x03, 0x05, 0x07
	.byte 0x01, 0x03, 0x05, 0x07, 0x01, 0x03, 0x05, 0x07
.align 16
.LC14:
	.byte 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37
	.byte 0x38, 0x39, 0x61, 0x62, 0x63, 0x64, 0x65, 0x67

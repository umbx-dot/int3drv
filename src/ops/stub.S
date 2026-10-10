	.text
	.code64
	.globl int3_stub
	.globl int3_stub_data
	.globl int3_stub_end

int3_stub:
	xchg %ax, %ax
	leaq int3_stub_data(%rip), %rbx
	movq 0*8(%rbx), %rax
	movq 1*8(%rbx), %rdi
	call *%rax
	movq %rax, 5*8(%rbx)
	movb $1, 2*8(%rbx)
1:
	movl $35, %eax
	leaq 3*8(%rbx), %rdi
	xorl %esi, %esi
	syscall
	jmp 1b

	.p2align 4
int3_stub_data:
	.space 6*8, 0
int3_stub_end:

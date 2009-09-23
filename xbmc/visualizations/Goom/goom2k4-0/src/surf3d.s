	.file	"surf3d.c"
	.version	"01.01"
gcc2_compiled.:
.text
	.align 4
.globl grid3d_new
	.type	 grid3d_new,@function
grid3d_new:
	pushl %ebp
	movl %esp,%ebp
	subl $44,%esp
	pushl %edi
	pushl %esi
	pushl %ebx
	movl 20(%ebp),%eax
	movl 12(%ebp),%esi
	movl %eax,-8(%ebp)
	addl $-12,%esp
	pushl $44
	call malloc
	movl %esi,%edx
	imull -8(%ebp),%edx
	movl %eax,%edi
	movl %edx,-12(%ebp)
	leal (%edx,%edx,2),%ebx
	movl %edx,8(%edi)
	addl $-12,%esp
	sall $2,%ebx
	pushl %ebx
	call malloc
	addl $32,%esp
	movl %eax,(%edi)
	addl $-12,%esp
	pushl %ebx
	call malloc
	movl %eax,4(%edi)
	movl 24(%ebp),%eax
	movl %eax,12(%edi)
	movl 28(%ebp),%eax
	movl %eax,16(%edi)
	movl 32(%ebp),%eax
	movl %eax,20(%edi)
	movl 8(%ebp),%eax
	movl %eax,28(%edi)
	movl %esi,24(%edi)
	movl -8(%ebp),%edx
	movl 16(%ebp),%eax
	movl %edx,32(%edi)
	movl %eax,36(%edi)
	movl $0,40(%edi)
	testl %edx,%edx
	je .L480
	movl %esi,%eax
	movl %esi,-28(%ebp)
	shrl $31,%eax
	addl %eax,%esi
	movl -8(%ebp),%eax
	shrl $31,%eax
	addl -8(%ebp),%eax
	movl -12(%ebp),%edx
	sarl $1,%eax
	movl %edx,-24(%ebp)
	negl -28(%ebp)
	movl %esi,-16(%ebp)
	movl %eax,-20(%ebp)
	.p2align 4,,7
.L481:
	movl -28(%ebp),%eax
	addl %eax,-24(%ebp)
	decl -8(%ebp)
	movl 12(%ebp),%esi
	testl %esi,%esi
	je .L479
	movl -8(%ebp),%eax
	subl -20(%ebp),%eax
	movl %eax,-4(%ebp)
	fildl -4(%ebp)
	movl %esi,-4(%ebp)
	movl -24(%ebp),%edx
	leal (%edx,%esi),%eax
	movl -16(%ebp),%ebx
	fildl 16(%ebp)
	leal (%eax,%eax,2),%eax
	sarl $1,%ebx
	leal 0(,%eax,4),%ecx
	fmulp %st,%st(1)
	fildl 20(%ebp)
	fdivrp %st,%st(1)
	fildl 8(%ebp)
	fildl -4(%ebp)
	jmp .L484
.L487:
	fxch %st(2)
	.p2align 4,,7
.L484:
	decl %esi
	movl %esi,%eax
	movl (%edi),%edx
	subl %ebx,%eax
	movl %eax,-4(%ebp)
	fildl -4(%ebp)
	addl $-12,%ecx
	fmul %st(2),%st
	fdiv %st(1),%st
	fstps (%edx,%ecx)
	fxch %st(2)
	movl (%edi),%eax
	movl $0,4(%eax,%ecx)
	movl (%edi),%eax
	fsts 8(%eax,%ecx)
	testl %esi,%esi
	jne .L487
	fstp %st(0)
	fstp %st(0)
	fstp %st(0)
.L479:
	cmpl $0,-8(%ebp)
	jne .L481
.L480:
	leal -56(%ebp),%esp
	popl %ebx
	movl %edi,%eax
	popl %esi
	popl %edi
	leave
	ret
.Lfe1:
	.size	 grid3d_new,.Lfe1-grid3d_new
.section	.rodata
	.align 8
.LC48:
	.long 0x0,0x3fe00000
	.align 4
.LC49:
	.long 0x3f19999a
	.align 4
.LC50:
	.long 0x3ee3d70a
.text
	.align 4
.globl grid3d_update
	.type	 grid3d_update,@function
grid3d_update:
	pushl %ebp
	movl %esp,%ebp
	subl $32,%esp
	pushl %esi
	pushl %ebx
	flds 12(%ebp)
	movl 8(%ebp),%ebx
	movl 16(%ebp),%ecx
	fld %st(0)
#APP
	fsin
#NO_APP
	fstps -4(%ebp)
	flds -4(%ebp)
	fxch %st(1)
#APP
	fcos
#NO_APP
	fstps -4(%ebp)
	flds -4(%ebp)
	cmpl $0,40(%ebx)
	jne .L519
	testl %ecx,%ecx
	je .L520
	xorl %esi,%esi
	cmpl 24(%ebx),%esi
	jge .L520
	fldl .LC48
	xorl %edx,%edx
	.p2align 4,,7
.L524:
	movl (%ebx),%eax
	fld %st(0)
	fld %st(1)
	fxch %st(1)
	fmuls 4(%eax,%edx)
	fxch %st(1)
	fmuls (%ecx,%esi,4)
	faddp %st,%st(1)
	incl %esi
	fstps 4(%eax,%edx)
	addl $12,%edx
	cmpl 24(%ebx),%esi
	jl .L524
	fstp %st(0)
.L520:
	movl 24(%ebx),%esi
	cmpl 8(%ebx),%esi
	jge .L519
	leal (%esi,%esi,2),%eax
	flds .LC49
	flds .LC50
	leal 0(,%eax,4),%ecx
	.p2align 4,,7
.L529:
	movl (%ebx),%eax
	flds 4(%eax,%ecx)
	fmul %st(2),%st
	fstps 4(%eax,%ecx)
	movl %esi,%eax
	subl 24(%ebx),%eax
	movl (%ebx),%edx
	leal (%eax,%eax,2),%eax
	flds 4(%edx,%eax,4)
	fmul %st(1),%st
	fadds 4(%edx,%ecx)
	incl %esi
	fstps 4(%edx,%ecx)
	addl $12,%ecx
	cmpl 8(%ebx),%esi
	jl .L529
	fstp %st(0)
	fstp %st(0)
.L519:
	xorl %esi,%esi
	cmpl 8(%ebx),%esi
	jge .L536
	xorl %ecx,%ecx
	.p2align 4,,7
.L534:
	movl (%ebx),%eax
	flds (%eax,%ecx)
	flds 8(%eax,%ecx)
	fmul %st(2),%st
	fxch %st(1)
	fmul %st(3),%st
	fsubp %st,%st(1)
	movl 4(%ebx),%edx
	incl %esi
	fstps (%edx,%ecx)
	movl (%ebx),%eax
	flds (%eax,%ecx)
	flds 8(%eax,%ecx)
	fxch %st(1)
	fmul %st(2),%st
	fxch %st(1)
	fmul %st(3),%st
	faddp %st,%st(1)
	movl 4(%ebx),%edx
	fstps 8(%edx,%ecx)
	movl (%ebx),%eax
	flds 4(%eax,%ecx)
	movl 4(%ebx),%edx
	fstps 4(%edx,%ecx)
	movl 4(%ebx),%eax
	flds (%eax,%ecx)
	fadds 12(%ebx)
	fstps (%eax,%ecx)
	movl 4(%ebx),%eax
	flds 4(%eax,%ecx)
	fadds 16(%ebx)
	fstps 4(%eax,%ecx)
	movl 4(%ebx),%eax
	flds 8(%eax,%ecx)
	fadds 20(%ebx)
	fstps 8(%eax,%ecx)
	addl $12,%ecx
	cmpl 8(%ebx),%esi
	jl .L534
.L536:
	fstp %st(0)
	fstp %st(0)
	popl %ebx
	popl %esi
	leave
	ret
.Lfe2:
	.size	 grid3d_update,.Lfe2-grid3d_update
.section	.rodata
	.align 4
.LC51:
	.long 0x40000000
	.align 8
.LC52:
	.long 0x0,0x42380000
.text
	.align 4
.globl surf3d_draw
	.type	 surf3d_draw,@function
surf3d_draw:
	pushl %ebp
	movl %esp,%ebp
	subl $60,%esp
	pushl %edi
	pushl %esi
	pushl %ebx
	movl $0,-20(%ebp)
	movl -20(%ebp),%edx
	movl 8(%ebp),%eax
	cmpl 8(%eax),%edx
	jge .L493
	fldl .LC52
	flds .LC51
	xorl %edi,%edi
	.p2align 4,,7
.L495:
	movl 8(%ebp),%eax
	movl 4(%eax),%eax
	movl %eax,-36(%ebp)
	fcoms 8(%eax,%edi)
	fnstsw %ax
	andb $69,%ah
	cmpb $1,%ah
	jne .L496
	fildl 16(%ebp)
	movl -36(%ebp),%edx
	fld %st(0)
	fmuls (%edx,%edi)
	fdivs 8(%edx,%edi)
	fld %st(3)
	faddp %st,%st(1)
	fstpl -32(%ebp)
	movl -32(%ebp),%eax
	movl -28(%ebp),%edx
	movl %eax,-40(%ebp)
	sarl $16,-40(%ebp)
	movl -36(%ebp),%edx
	fmuls 4(%edx,%edi)
	fdivs 8(%edx,%edi)
	movl -40(%ebp),%ecx
	fld %st(2)
	faddp %st,%st(1)
	fstpl -32(%ebp)
	movl -32(%ebp),%eax
	movl -28(%ebp),%edx
	movl %eax,-44(%ebp)
	movl 28(%ebp),%eax
	sarl $1,%eax
	addl %eax,%ecx
	movl 32(%ebp),%eax
	sarl $16,-44(%ebp)
	sarl $1,%eax
	movl %ecx,%ebx
	subl -44(%ebp),%eax
	movl %eax,%esi
	cmpl 28(%ebp),%ebx
	jge .L496
	testl %ecx,%ecx
	jl .L496
	cmpl 32(%ebp),%esi
	jge .L496
	testl %eax,%eax
	jge .L499
.L496:
	xorl %esi,%esi
	xorl %ebx,%ebx
.L499:
	movl 20(%ebp),%eax
	movl %ebx,%edx
	leal (%eax,%edx,4),%edx
	movl 28(%ebp),%eax
	imull %esi,%eax
	leal (%edx,%eax,4),%eax
	testl %ebx,%ebx
	je .L494
	testl %esi,%esi
	je .L494
#APP
	movd (%eax), %mm0
	paddusb 12(%ebp), %mm0
	movd %mm0, (%eax)
#NO_APP
.L494:
	incl -20(%ebp)
	addl $12,%edi
	movl -20(%ebp),%eax
	movl 8(%ebp),%edx
	cmpl 8(%edx),%eax
	jl .L495
	fstp %st(0)
	fstp %st(0)
.L493:
	popl %ebx
	popl %esi
	popl %edi
	leave
	ret
.Lfe3:
	.size	 surf3d_draw,.Lfe3-surf3d_draw
	.align 4
.globl surf3d_rotate
	.type	 surf3d_rotate,@function
surf3d_rotate:
	pushl %ebp
	movl %esp,%ebp
	subl $32,%esp
	pushl %esi
	pushl %ebx
	flds 12(%ebp)
	movl 8(%ebp),%ebx
	fld %st(0)
#APP
	fsin
#NO_APP
	fstps -4(%ebp)
	flds -4(%ebp)
	fxch %st(1)
#APP
	fcos
#NO_APP
	fstps -4(%ebp)
	xorl %esi,%esi
	flds -4(%ebp)
	cmpl 8(%ebx),%esi
	jge .L537
	xorl %ecx,%ecx
	.p2align 4,,7
.L508:
	movl (%ebx),%eax
	flds (%eax,%ecx)
	flds 8(%eax,%ecx)
	fmul %st(2),%st
	fxch %st(1)
	fmul %st(3),%st
	fsubp %st,%st(1)
	movl 4(%ebx),%edx
	incl %esi
	fstps (%edx,%ecx)
	movl (%ebx),%eax
	flds (%eax,%ecx)
	flds 8(%eax,%ecx)
	fxch %st(1)
	fmul %st(2),%st
	fxch %st(1)
	fmul %st(3),%st
	faddp %st,%st(1)
	movl 4(%ebx),%edx
	fstps 8(%edx,%ecx)
	movl (%ebx),%eax
	flds 4(%eax,%ecx)
	movl 4(%ebx),%edx
	fstps 4(%edx,%ecx)
	addl $12,%ecx
	cmpl 8(%ebx),%esi
	jl .L508
.L537:
	fstp %st(0)
	fstp %st(0)
	popl %ebx
	popl %esi
	leave
	ret
.Lfe4:
	.size	 surf3d_rotate,.Lfe4-surf3d_rotate
	.align 4
.globl surf3d_translate
	.type	 surf3d_translate,@function
surf3d_translate:
	pushl %ebp
	movl %esp,%ebp
	pushl %ebx
	movl 8(%ebp),%ecx
	xorl %ebx,%ebx
	cmpl 8(%ecx),%ebx
	jge .L512
	xorl %edx,%edx
	.p2align 4,,7
.L514:
	movl 4(%ecx),%eax
	flds (%eax,%edx)
	fadds 12(%ecx)
	incl %ebx
	fstps (%eax,%edx)
	movl 4(%ecx),%eax
	flds 4(%eax,%edx)
	fadds 16(%ecx)
	fstps 4(%eax,%edx)
	movl 4(%ecx),%eax
	flds 8(%eax,%edx)
	fadds 20(%ecx)
	fstps 8(%eax,%edx)
	addl $12,%edx
	cmpl 8(%ecx),%ebx
	jl .L514
.L512:
	popl %ebx
	leave
	ret
.Lfe5:
	.size	 surf3d_translate,.Lfe5-surf3d_translate
	.ident	"GCC: (GNU) 2.95.3 19991030 (prerelease)"

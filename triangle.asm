;=====================================================================
; ECOAR - Intel x86-64 project - Gouraud shading
;
; Author:      Michal Leszczynski
; Date:        2016-12-20
; Description: Function takes description of 3 vertices and the pixel
;              array and performs Gouraud shading.
;
;=====================================================================

section	.text
global  fillTriangleSSE

%define img_width r11
%define pixels r12

%define v1x r9
%define v1y r9+4
%define v1r r9+8
%define v1g r9+12
%define v1b r9+16

%define v2x r10
%define v2y r10+4
%define v2r r10+8
%define v2g r10+12
%define v2b r10+16

%define v3x r8
%define v3y r8+4
%define v3r r8+8
%define v3g r8+12
%define v3b r8+16

%define d0  rbp-4
%define d1  rbp-8
%define y   rbp-12
%define curx1  rbp-16
%define curx2  rbp-20
%define row_height  rbp-24

%define coeffIa1    rbp-32
%define coeffIa2    rbp-36
%define coeffIb1    rbp-40
%define coeffIb2    rbp-44
%define	x1	rbp-48

%define Ib0	rbp-52
%define	Ibb	rbp-56
%define	Ibg	rbp-60
%define	Ibr	rbp-64
%define vecIb	rbp-64

%define Ia0	rbp-68
%define	Iab	rbp-72
%define	Iag	rbp-76
%define	Iar	rbp-80
%define vecIa	rbp-80

%define x2	rbp-84
%define	x	rbp-88

%define tmp     rbp-112 ; 16 bytes

%macro  subdiv  5
	fild	DWORD %1
	fisub	DWORD %2
	fild	DWORD %3
	fisub	DWORD %4
	fdivp	st1, st0
	fstp	DWORD %5
%endmacro

%macro  addmul  5
	fld	DWORD %1
	fld	DWORD %2
	fmulp
	fld	DWORD %3
	fld	DWORD %4
	fmulp
	faddp
	fstp	DWORD %5
%endmacro

%macro  swapm   2
	mov	eax, %1
	mov	edx, %2
	mov	%1, edx
	mov	%2, eax
%endmacro

%macro	xpack    4
	mov	eax, %1
	mov	DWORD [tmp], eax
	mov	eax, %2
	mov	DWORD [tmp+4], eax
	mov	eax, %3
	mov	DWORD [tmp+8], eax
	mov	eax, %4
	mov	DWORD [tmp+12], eax
%endmacro

fillTriangleSSE:
; prologue
	push	rbp
	mov	rbp, rsp
	sub	rsp, 128	; allocate space for local variables

; push callee-saved registers
	push	rbx
	push	r9
	push	r10
	push	r11
	push	r12

; function body
bptest:
	mov	r9, rdx
	mov	r10, rcx
	mov	r11, rdi
	mov	r12, rsi

	mov	[Ia0], DWORD 0
	mov	[Ib0], DWORD 0

	; calculate d0
	subdiv	[v2x], [v1x], [v2y], [v1y], [d0]
	subdiv	[v3x], [v1x], [v3y], [v1y], [d1]

	; initialize y var
	mov	edx, [v1y]
	mov	ecx, edx   ; loop counter y
	mov	[y], ecx

	; initialize curx1, curx2 vars
	fild	DWORD [v1x]
	fild	DWORD [v1x]
	fstp	DWORD [curx1]
	fstp	DWORD [curx2]

	; initialize row_height
	mov	rax, img_width
	add	rax, rax
	add	rax, img_width
	mov	[row_height], eax

loop:
	subdiv	[y], [v3y], [v1y], [v3y], [coeffIa1]
	subdiv	[v1y], [y], [v1y], [v3y], [coeffIa2]

	mov	ecx, [y]
	cmp	ecx, [v2y]	; if y <= v2.y
	jg	y_greater
	subdiv [y], [v2y], [v1y], [v2y], [coeffIb1]
	subdiv [v1y], [y], [v1y], [v2y], [coeffIb2]

	addmul [coeffIb1], [v1r], [coeffIb2], [v2r], [Ibr]
	addmul [coeffIb1], [v1g], [coeffIb2], [v2g], [Ibg]
	addmul [coeffIb1], [v1b], [coeffIb2], [v2b], [Ibb]
	jmp	if_end
y_greater:
	subdiv [y], [v3y], [v2y], [v3y], [coeffIb1]
	subdiv [v2y], [y], [v2y], [v3y], [coeffIb2]

	addmul [coeffIb1], [v2r], [coeffIb2], [v3r], [Ibr]
	addmul [coeffIb1], [v2g], [coeffIb2], [v3g], [Ibg]
	addmul [coeffIb1], [v2b], [coeffIb2], [v3b], [Ibb]
if_end:
	addmul [coeffIa1], [v1r], [coeffIa2], [v3r], [Iar]
	addmul [coeffIa1], [v1g], [coeffIa2], [v3g], [Iag]
	addmul [coeffIa1], [v1b], [coeffIa2], [v3b], [Iab]

	fld	DWORD [curx1]
	fistp	DWORD [x1]
	fld	DWORD [curx2]
	fistp	DWORD [x2]

	mov	eax, [x1]
	cmp	eax, [x2]
	jle	no_swap

	swapm	DWORD [x1], DWORD [x2]
	swapm	DWORD [Ibr], DWORD [Iar]
	swapm	DWORD [Ibg], DWORD [Iag]
	swapm	DWORD [Ibb], DWORD [Iab]

no_swap:
	; prepare for inner loop
	xor	rcx, rcx
	mov	ecx, [x1]
	mov	[x], ecx

	; calculate "row" address
	xor	rax, rax
	xor	rdx, rdx

	mov	eax, [y]
	mov	edx, [row_height]

	mul	edx			; row += y * row_height

	add	rax, pixels		; row += pixels (base address)
	add	rax, rcx		; row += x1 * 3
	add	rax, rcx
	add	rax, rcx
	mov	rsi, rax		; store row

	mov	rbx, [x2]		; calculate difference x2-x1
	sub	ecx, ebx
	neg	ecx

	; prepare xmm stuff
	mov	DWORD [tmp], ecx
	mov	DWORD [tmp+4], ecx
	mov	DWORD [tmp+8], ecx
	mov	DWORD [tmp+12], ecx

	movaps	xmm0, [tmp]
	cvtdq2ps xmm0, xmm0		; convert to float
	rcpps	xmm0, xmm0		; vector of 1/(x2-x1)

	xpack	1, 1, 1, 1
	movaps	xmm1, [tmp]		; initial vector coeff1
	cvtdq2ps xmm1, xmm1		; convert to float

	xpack	0, 0, 0, 0
	movaps	xmm2, [tmp]		; initial vector coeff2
	cvtdq2ps xmm2, xmm2		; convert to float

	movaps	xmm3, [vecIb]		; [ Ibr, Ibg, Ibb, 0 ]
	movaps	xmm4, [vecIa]		; [ Iar, Iag, Iab, 0 ]

	cmp	ecx, ebx
	jg	inner_loop_end

inner_loop:
	movaps	xmm5, xmm1		; copy coeff1
	movaps	xmm6, xmm2		; copy coeff2

	mulps	xmm5, xmm3		; coeff1 * Ib
	mulps	xmm6, xmm4		; coeff2 * Ia
	addps	xmm5, xmm6		; coeff1 * Ib + coeff2 * Ia

	subps	xmm1, xmm0		; coeff1 -= 1/(x2-x1)
	addps	xmm2, xmm0		; coeff2 += 1/(x2-x1)

	cvtps2dq xmm5, xmm5		; convert floats to integers
	movups	[tmp], xmm5

	mov	eax, [tmp]
	mov	[rsi+2], al		; store B
	mov	eax, [tmp+4]
	mov	[rsi+1], al		; store G
	mov	eax, [tmp+8]
	mov	[rsi], al		; store R

	add	rsi, 3
	dec	ecx			; advance y
	jg	inner_loop

inner_loop_end:
	mov	ecx, ebx
	inc	ecx
	mov	[x], ecx
	mov	ecx, [y]
	cmp	ecx, [v3y]
	je	loop_end

	cmp	ecx, [v2y]
	jne	no_d0_change
	subdiv	[v3x], [v2x], [v3y], [v2y], [d0]

no_d0_change:
	inc	DWORD [y]

	fld	DWORD [curx1]
	fadd	DWORD [d0]
	fstp	DWORD [curx1]

	fld	DWORD [curx2]
	fadd	DWORD [d1]
	fstp	DWORD [curx2]

	jmp	loop

loop_end:
	mov	rax, 0			; return 0

; pop callee-saved registers
	pop	r12
	pop	r11
	pop	r10
	pop	r9
	pop	rbx

; epilogue
	mov	rsp, rbp
	pop	rbp
	ret


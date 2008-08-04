;// file : mmx_zoom.s
;// author : JC Hoelt <jeko@free.fr>
;//
;// history
;// 07/01/2001 : Changing FEMMS to EMMS : slower... but run on intel machines
;//	03/01/2001 : WIDTH and HEIGHT are now variable
;//	28/12/2000 : adding comments to the code, suppress some useless lines
;//	27/12/2000 : reducing memory access... improving performance by 20%
;//		coefficients are now on 1 byte
;//	22/12/2000 : Changing data structure
;//	16/12/2000 : AT&T version
;//	14/12/2000 : unrolling loop
;//	12/12/2000 : 64 bits memory access


.data

chaine:
	.string	"pos = %d\n\0"
	.long 0x0
		
thezero:
	.long 0x00000000
	.long 0x00000000

.text

.globl mmx_zoom		;// name of the function to call by C program
/* .extern coeffs		;// the transformation buffer */
.extern expix1,expix2 ;// the source and destination buffer
.extern mmx_zoom_size, zoom_width ;// size of the buffers

.extern brutS,brutD,buffratio,precalCoef,prevX,prevY

#define PERTEMASK 15
/* faire : a / sqrtperte <=> a >> PERTEDEC*/
#define PERTEDEC 4

.align 16
mmx_zoom:

		pushl %ebp
		movl %esp,%ebp
		subl $12,%esp

		movl prevX,%eax
		decl %eax
		sarl $4,%eax
		movl %eax,-4(%ebp)

		movl prevY,%eax
		decl %eax
		sarl $4,%eax
		movl %eax,-8(%ebp)

;// initialisation du mm7 à zero
		movq (thezero), %mm7

movl mmx_zoom_size, %ecx
decl %ecx

.while:
	;// esi <- nouvelle position
	movl brutS, %eax
	leal (%eax, %ecx, 8),%eax

	movl (%eax),%edx /* = brutS.px (brutSmypos) */
	movl 4(%eax),%eax /* = brutS.py */

	movl brutD,%ebx
	leal (%ebx, %ecx, 8),%ebx
	movl (%ebx),%esi
	subl %edx, %esi
	imull buffratio,%esi
	sarl $16,%esi
	addl %edx,%esi /* esi = px */

	/* eax contient deja brutS.py = le nouveau brutSmypos*/
	/* ebx pointe sur brutD[myPos] */
	movl 4(%ebx),%edi
	subl %eax,%edi
	imull buffratio,%edi
	sarl $16,%edi
	addl %eax,%edi /* edi = py */

/*		pushl %eax
		pushl %ebx*/
/*		popl %ebx
		popl %eax*/
		
	movl %esi,%eax
	andl $15,%eax /* eax = coefh */
	movl %edi,%ebx
	andl $15,%ebx /* ebx = coefv */

	leal 0(,%ebx,4),%ebx
	sall $6,%eax
	addl %ebx,%eax
	movl $precalCoef,%ebx
/*	movd (%eax,%ebx),%mm6*/ /* mm6 = coeffs */

	cmpl -8(%ebp),%edi
	jge .then1
	cmpl -4(%ebp),%esi
	jge .then1

	sarl $4,%esi
	sarl $4,%edi
	imull zoom_width,%edi
	leal (%esi,%edi),%esi
	jmp .finsi1

.then1:
	movl $0,%esi
.finsi1:

	/** apres ce calcul, %esi = pos, %mm6 = coeffs **/
/*	pushl %esi
	pushl $chaine
	call printf
	addl $8,%esp*/

	movl expix1,%eax

	;// recuperation des deux premiers pixels dans mm0 et mm1
/*	movq (%eax,%esi,4), %mm0		/* b1-v1-r1-a1-b2-v2-r2-a2 */
	movq %mm0, %mm1				/* b1-v1-r1-a1-b2-v2-r2-a2 */

	;// depackage du premier pixel
	punpcklbw %mm7, %mm0	/* 00-b2-00-v2-00-r2-00-a2 */

	movq %mm6, %mm5			/* ??-??-??-??-c4-c3-c2-c1 */
	;// depackage du 2ieme pixel
	punpckhbw %mm7, %mm1	/* 00-b1-00-v1-00-r1-00-a1 */

	;// extraction des coefficients...
	punpcklbw %mm5, %mm6	/* c4-c4-c3-c3-c2-c2-c1-c1 */
	movq %mm6, %mm4			/* c4-c4-c3-c3-c2-c2-c1-c1 */
	movq %mm6, %mm5			/* c4-c4-c3-c3-c2-c2-c1-c1 */

	punpcklbw %mm5, %mm6	/* c2-c2-c2-c2-c1-c1-c1-c1 */
	punpckhbw %mm5, %mm4	/* c4-c4-c4-c4-c3-c3-c3-c3 */

	movq %mm6, %mm3			/* c2-c2-c2-c2-c1-c1-c1-c1 */
	punpcklbw %mm7, %mm6	/* 00-c1-00-c1-00-c1-00-c1 */
	punpckhbw %mm7, %mm3	/* 00-c2-00-c2-00-c2-00-c2 */
	
	;// multiplication des pixels par les coefficients
	pmullw %mm6, %mm0		/* c1*b2-c1*v2-c1*r2-c1*a2 */
	pmullw %mm3, %mm1		/* c2*b1-c2*v1-c2*r1-c2*a1 */
	paddw %mm1, %mm0
	
	;// ...extraction des 2 derniers coefficients
	movq %mm4, %mm5			/* c4-c4-c4-c4-c3-c3-c3-c3 */
	punpcklbw %mm7, %mm4	/* 00-c3-00-c3-00-c3-00-c3 */
	punpckhbw %mm7, %mm5	/* 00-c4-00-c4-00-c4-00-c4 */

	/* ajouter la longueur de ligne a esi */
	addl prevX,%esi
		
	;// recuperation des 2 derniers pixels
/*	movq (%eax,%esi,4), %mm1*/
	movq %mm1, %mm2
	
	;// depackage des pixels
	punpcklbw %mm7, %mm1
	punpckhbw %mm7, %mm2
	
	;// multiplication pas les coeffs
	pmullw %mm4, %mm1
	pmullw %mm5, %mm2
	
	;// ajout des valeurs obtenues à la valeur finale
	paddw %mm1, %mm0
	paddw %mm2, %mm0

	;// division par 256 = 16+16+16+16, puis repackage du pixel final
	psrlw $8, %mm0
	packuswb %mm7, %mm0
	
	;// passage au suivant

	;// enregistrement du resultat
	movl expix2,%eax
/*	movd %mm0,(%eax,%ecx,4)*/

	decl %ecx
	;// test de fin du tantque
	cmpl $0, %ecx				;// 400x300

	jz .fin_while
	jmp .while

.fin_while:
	emms

	movl %ebp,%esp
	popl %ebp

	ret                  ;//The End

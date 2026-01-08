; SQPSHOW.CMD - AZffiche une image
; Syntaxe : SQPSHOW,fichier.sqp

	ORG     $7600   ; Zone des utilitaires


LINBUF  EQU     $C080
SYSDAT  EQU     $CC0E
LINBFP  EQU     $CC14
MEMEND  EQU     $CC2B
PRTCHK  EQU     $CCD8
PRTOUT  EQU     $CCE4
WARMS	EQU     $CD03
INCH    EQU     $CD09
GETCHR  EQU     $CD15
PUTCHR  EQU     $CD18
PSTRING EQU     $CD1E
GETFIL  EQU     $CD2D
SETEXT  EQU     $CD33
RPTERR  EQU     $CD3F
DOCMND  EQU     $CD4B
CINCHNE EQU     $D3E5
CSTAT   EQU	$D3F7
COUTCH  EQU	$D3F9
FMSCLS  EQU	$D403
FMS	EQU	$D406

START   LBSR    CLRFCB
	
* parse filename
        JSR     GETFIL
        LBCS    BAD_SQP
	 
* Fixer extension par défaut (tout accepté)
        BSR     CHKEXT
        
* Ouvre le fichier en lecture
        BSR     OPEN
        BEQ     CHKSIG
        LBRA	CANNOT_OPEN_SQP

CHKEXT  LDA     12,X
        BNE     EXTDONE
        LDA     #'S
        STA     12,X
        LDD     #'Q*256+'P
        STD     13,X
EXTDONE RTS   

PSET2   BSR     *+2         
        LDA     #16
        MUL
* AFFICHE le pixel en Y et l'avance  de  1        
PSET    PSHS    D,X
        SWI     
        FCB     $24
        LEAX    <COORD,PCR
        TFR     Y,D
        COMA
        STA     3,X
        STB     1,X
        SWI
        FCB     $2A
        LEAY    1,Y
        PULS    D,X,PC
COORD   FDB     0,0     
        FDB     0,0

* 2 pixels par octets
SQP_0   LDA     #12
        JSR     PUTCHR
        LDY     #0
LOOP    SET     *
        BSR     GETC
        LBNE    READ_ERROR
        BSR     PSET2
        BNE     LOOP
        
        BSR     WAIT
        
EXIT    BSR     CLOSE
        JMP     WARMS
        
WAIT    LDA     #7
        JSR     PUTCHR
LOOP    SET     *
        JSR     GETCHR
        TSTA
        BNE     LOOP
LOOP    SET     *
        JSR     GETCHR
        TSTA
        BEQ     LOOP
        RTS

* EXOMIZER compression
SQP_1   LDY     #0

CHKSIG  BSR     GETC
        CMPB    #'S
        BNE     BAD_SQP
        BSR     GETC
        CMPB    #'Q
        BNE     BAD_SQP
        BSR     GETC
        CMPB    #'P
        BNE     BAD_SQP
        BSR     GETC
        DECB    
        BEQ     SQP_0
        DECB
        BEQ     SQP_1
*       BRA     BAD_SQP        

* ouvre le  fichier
OPEN    STX     <GETC+3,PCR
        STX     <CLOSE+1,PCR
        LDD	#511    ; OPEN for READ
        STA	,X
        BSR     CALLFMS
        BNE     OPENEND
        STB     59,X    ; BINARY
        COMB
        STB     ,X
OPENEND RTS

* ouvre le  fichier
CLOSE   LDX     #0
	LDA	#4      ; CLOSE
        STA	,X
CALLFMS JMP     FMS
        
* Lit un octet
GETC    PSHS    A,X
        LDX     #0
        BSR     CALLFMS
        TFR     A,B
        PULS    A,X,PC

CLRFCB  LEAX    <SQPFCB,PCR
        LDD     #31
LOOP    SET     *
        STA     B,X
        DECB
        BPL     LOOP
        RTS

; ———— Gestion des Erreurs ————
BAD_SQP	LEAX	<BAD_SQP_MSG,PCR
ERROR	JSR	PSTRING
	BRA	EXIT
BAD_SQP_MSG
        FCC     "BAD SPQ FILE"
        FCB     4
	 
CANNOT_OPEN_SQP
	LEAX	<CANNOT_OPEN_SQP_MSG,PCR
	BRA     ERROR
CANNOT_OPEN_SQP_MSG
        FCC "CANNOT OPEN SPQ FILE"
	FCB $04
 
READ_ERROR:
	LEAX	<READ_ERROR_MSG,PCR
	BRA     ERROR	 
READ_ERROR_MSG:
	 FCC "READ ERROR"
	 FCB $04

; ———— Variables Locales ————
SQPFCB  RMB     320     ; FCB fichier

        END     START
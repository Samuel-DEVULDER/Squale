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
INCH2   EQU     $CD0C
GETCHR  EQU     $CD15
PUTCHR  EQU     $CD18
PSTRING EQU     $CD1E
PCRLF   EQU     $CD24
GETFIL  EQU     $CD2D
SETEXT  EQU     $CD33
RPTERR  EQU     $CD3F
INDEC   EQU     $CD48
DOCMD   EQU     $CD4B
CINCHNE EQU     $D3E5
CSTAT   EQU	$D3F7
COUTCH  EQU	$D3F9
FMSCLS  EQU	$D403
FMS	EQU	$D406

START   LBSR    CLRFCB
	
* parse filename
        JSR     GETFIL
        LBCS    DOS_ERR
	 
* Fixer extension par défaut (tout accepté)
        BSR     CHKEXT

* cas spécial pour les fichiers TXT      
        BSR     CHKTXT
        BEQ     SHOWTXT
        
* Ouvre le fichier en lecture
        LBSR    OPEN
        LBEQ    SHOWSQP
        LBRA	DOS_ERR

CHKEXT  LDA     12,X
        BNE     CHKEXT1
        LDA     #'S
        STA     12,X
        LDD     #'Q*256+'P
        STD     13,X
CHKEXT1 RTS   

CHKTXT  LDD     #'T*256+'X
        CMPD    12,X
        BNE     CHKTXT1
        CMPA    14,X
CHKTXT1 RTS   

SHOWTXT LBSR    OPEN
        LBNE    DOS_ERR
        JSR     PCRLF
        LDA     #'U-64
SHWTXT1 CMPA    #10
        BNE     SHWTXT2
        BSR     PUTC
        LDA     #13
SHWTXT2 BSR     PUTC
        BSR     READ
        BNE     SHWTXT1
        JSR     PCRLF
        BRA     EXIT
        
WAIT    BSR     BEEP
WAIT1   BSR     GETC
        TSTA
        BEQ     WAIT1
        RTS

CURSON  LDA     #'T-64
        FCB     $8C
CURSOFF LDA     #'U-64
        FCB     $8C
CLS     LDA     #12
        FCB     $8C
BEEP    LDA     #7
PUTC    JMP     $F10F   ; fast monitor
GETC    JMP     $F106   ; fast monitor
KTST    JMP     $F10C   ; fast monitor

PSET2   LEAY    -2,Y
        PSHS    CC,D,X
        LEAX    <COORD,PCR

        LSRA
        LSRA
        LSRA
        LSRA
        SWI     
        FCB     $24

        TFR     Y,D
        STA     3,X
        STB     1,X
        SWI
        FCB     $2A

        LDA     1,S
        ANDA    #15
        SWI     
        FCB     $24

        INC     1,X
        SWI
        FCB     $2A
        PULS    CC,D,X,PC

* 2 pixels par octets
SQP1    BSR     READ
        BSR     PSET2
        BNE     SQP1

        BSR     WAIT
        BSR     CLS        
EXIT    BSR     CLOSE
        BSR     CURSON
        JMP     WARMS

* EXOMIZER compression
SQP2

SHOWSQP BSR     READ
        CMPA    #'S
        BNE     BADSQP
        BSR     READ
        CMPA    #'Q
        BNE     BADSQP
        BSR     READ
        CMPA    #'P
        BNE     BADSQP
        LDY     #0
        BSR     CURSOFF
        BSR     READ
        DECA    
        BEQ     SQP1
        DECA
        BEQ     SQP2
BADSQP  BRA     BAD_SQP        

* Lit un octet
READ    PSHS    X
        LDX     #0
        BSR     CALLFMS
        BEQ     READ_DONE
        LDA     1,X
        SUBA    #8
        BNE     READ_ERR
READ_DONE
        TSTA
        PULS    X,PC
READ_ERR
        LEAS    4,S
        BRA     DOS_ERR

PSET    PSHS    D,X
        SWI     
        FCB     $24
        LEAX    <COORD,PCR
        TFR     Y,D
        STA     3,X
        STB     1,X
        SWI
        FCB     $2A
        PULS    D,X,PC
COORD   FDB     0,0     
        FDB     0,0

* ouvre le  fichier
CLOSE   LDX     #0
	LDA	#4      ; CLOSE
        STA	,X
CALLFMS JMP     FMS

* ouvre le  fichier
OPEN    LDD	#511    ; OPEN for READ
        STA	,X
        BSR     CALLFMS
        BNE     OPENEND
        STX     <READ+3,PCR
        STX     <CLOSE+1,PCR
        STB     59,X    ; BINARY
        COMB
        STB     ,X      ; sets Z=1
OPENEND RTS

CLRFCB  LEAX    <SQPFCB,PCR
        LDD     #31
LOOP    SET     *
        STA     B,X
        DECB
        BPL     LOOP
        RTS

; ———— Gestion des Erreurs ————
DOS_ERR JSR     RPTERR
        LBSR    BEEP
        LBRA    EXIT
        
BAD_SQP	LEAX	<BAD_SQP_MSG,PCR
PUTS    JMP     PSTRING
	LBRA    EXIT
BAD_SQP_MSG
        FCC     "BAD SPQ FILE"
        FCB     4

; ———— Variables Locales ————
SQPFCB  RMB     320     ; FCB fichier

        END     START
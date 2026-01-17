; SQPSHOW.CMD - Affiche une image
; Syntaxe : SQPSHOW,fichier.sqp

        ORG     $7600   ; Zone des utilitaires

LINBUF  EQU     $C080
SYSDAT  EQU     $CC0E
LINBFP  EQU     $CC14
MEMEND  EQU     $CC2B
PRTCHK  EQU     $CCD8
PRTOUT  EQU     $CCE4
WARMS   EQU     $CD03
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
CSTAT   EQU     $D3F7
COUTCH  EQU     $D3F9
FMSCLS  EQU     $D403
FMS     EQU     $D406

ZZZ     EQU     0

START   LBSR    INIT
        
* parse filename
        JSR     GETFIL
        LBCS    DOS_ERR
         
* Fixer extension par défaut (tout accepté)
        BSR     CHKEXT

* cas spécial pour les fichiers TXT      
        BSR     CHKTXT
        LBEQ    SHOWTXT
        
* Ouvre le fichier en lecture
        BRA    SHOWSQP

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

CHKBYT  PULS    X
        BSR     READ
        CMPA    ,X+
        LBNE    BAD_SQP
        JMP     ,X

SHOWSQP LBSR    OPEN
        LBNE    DOS_ERR
        BSR     CHKBYT
        FCB     'S
        BSR     CHKBYT
        FCB     'Q
        BSR     CHKBYT
        FCB     'P
        BSR     CURSOFF
        BSR     READ
        DECA    
        BEQ     SQP1
        DECA
        LBEQ    SQP2
        LBRA    BAD_SQP

DONE    BSR     WAIT
        BSR     CLS        
EXIT    BSR     CLOSE
        BSR     CURSON
        JMP     WARMS

SHOWTXT BSR     OPEN
        LBNE    DOS_ERR
        JSR     PCRLF
        LDA     #'U-64
SHWTXT1 CMPA    #10
        BNE     SHWTXT2
        BSR     PUTC
        LDA     #13
SHWTXT2 BSR     PUTC
        BSR     READ
        TSTA
        BNE     SHWTXT1
        JSR     PCRLF
        BRA     EXIT

* ouvre le  fichier
CLOSE   LDX     #ZZZ
        LDA     #4      ; CLOSE
        STA     ,X
CALLFMS JMP     FMS

* Lit un octet
READ    PSHS    X
        LDX     #ZZZ
        BSR     CALLFMS
        BEQ     READEND
        LDA     1,X
        SUBA    #8
        BNE     READERR
READEND PULS    X,PC
READERR LEAS    4,S
        LBRA    DOS_ERR

* ouvre le  fichier
OPEN    LDD     #511    ; OPEN for READ
        STA     ,X
        BSR     CALLFMS
        BNE     OPENEND
        STX     <READ+3,PCR
        STX     <CLOSE+1,PCR
        STB     59,X    ; BINARY
        COMB
        STB     ,X      ; sets Z=1
OPENEND RTS

DPSET   LEAY    -1,Y
        LBRA    PSET

* 2 pixels par octets
SQP1    LDY     #0
SQP1LP  BSR     READ
        BSR     DPSET
        LSRA
        LSRA
        LSRA
        LSRA
        BSR     DPSET
        LEAY    ,Y
        BNE     SQP1LP
        BRA     DONE
        
EXOCOOK LDX     #ZZZ
        ABX
        ASLB
        ABX
        LDB     ,X
        BSR     EXOBITS
        ADDD    1,X
        RTS

EXOBIT  LDB     #1
EXOBITS PSHS    U               ; U=0
EXOBUF  LDA     #1
        BRA     EXOBIT4
EXOBIT2 LSRA
        BNE     EXOBIT3
        BSR     READ
        LSRA
        ORA     #128
EXOBIT3 ROL     1,S
        ROL     ,S
EXOBIT4 DECB
        BPL     EXOBIT2
        STA     <EXOBUF+1,PCR
        PULS    D,PC

EXOIDX  LDB     #ZZZ/256
        CMPB    #16
        BNE     EXOIDX1
        LBRA    DONE
EXOIDX1 BLO     EXOCOFF
        DECB
        BSR     EXOBITS

EXOCPY  TFR     D,X
EXOCPY1 BSR     READ
        BSR     DPSET
        LEAX    -1,X
        BNE     EXOCPY1
        BRA     EXOLOOP

EXOTAB  FCB     4,2,4,16,48,32

EXOCOFF BSR     EXOCOOK
        PSHS    D
        LEAX    <EXOTAB,PCR
        CMPD    #3
*        CMPB    #3 <=bad optim
        BHS     EXOSCOF
        ABX
EXOSCOF LDB     ,X
        BSR     EXOBITS
        ADDB    3,X
        BSR     EXOCOOK
        STD     <EXOOFFS+1,PCR
        PULS    X

EXOCPY2 LEAY    -1,Y
        TFR     Y,D
EXOOFFS ADDD    #ZZZ    ; auto modifiable
        BSR     PGET
        BSR     PSET    ; inline ?
        LEAX    -1,X
        BNE     EXOCPY2
*        BRA     EXOLOOP

EXOLOOP BSR     EXOBIT
        STB     <EXOIDX+1,PCR
        BNE     EXOCPY
     
EXORBL  BSR     EXOBIT
        TSTB    
        BNE     EXOIDX
        INC     <EXOIDX+1,PCR
        BRA     EXORBL

* EXOMIZER decompression
SQP2    LBSR    READ
        STA     EXOBUF+1,PCR        
        LEAY    EXOBIBA,PCR
        STY     EXOCOOK+1,PCR
        LDU     #0
        CLRB
EXONXT  CLRA
        PSHS    D
        BITB    #15
        BNE     EXOSKP
        LDX     #1
EXOSKP  LDB     #4
        LBSR    EXOBITS
        STB     ,Y+
        COMB
EXOROLL ROL     ,S
        ROLA
        INCB
        BMI     EXOROLL
        LDB     ,S
        STX     ,Y++
        LEAX    D,X
        PULS    D
        INCB
        CMPB    #52
        BNE     EXONXT
        LEAY    ,U                      
        BRA     EXOLOOP

PYXOFF  EQU     -10     ; PYCACHE-PXCACHE
COOROFF EQU     -8      ; COORD-PXCACHE

PSET    PSHS    A,X
        TFR     Y,D
PSET1   LDX     #ZZZ
        INCA
        STD     -10,X
        ABX
        LDA     #15
        ANDA    ,S
        STA     ,X
        
        TSTB    
        BEQ     PFLUSH
        PULS    A,X,PC

PGET    PSHS    X
PGET1   LDX     #ZZZ
        CMPD    -10,X
        BHS     PGET3
PGET2   ABX
        LDA     ,X      ; get color from cache
        PULS    X,PC
   
PGET3   STA     -5,X
        LDA     -10,X
        BEQ     PGET2
        STB     -7,X
        LEAX    -8,X
        SWI
        FCB     $2F     ; get color from monitor
        PULS    X,PC

PFLUSH  LEAU    ,X
        LEAX    -8,X
        TFR     Y,D
        STA     3,X
        STA     7,X
        
PFLUSH1 STB     1,X
        LDA     ,U+
        SWI
        FCB     $24     ; set color
        CMPA    ,U+
        BNE     PFLUSH2
        INCB
        BNE     PFLUSH4
        DECB
PFLUSH2 SWI
        FCB     $2A     ; draw point             
PFLUSH3 LEAU    -1,U
        INCB
        BNE     PFLUSH1
        LDU     #0
        PULS    A,X,PC
        
PFLUSH4 CMPA    ,U+
        BNE     PFLUSH5
        INCB    
        BNE     PFLUSH4
        DECB
PFLUSH5 STB     5,X
        SWI
        FCB     $2B     ; draw line
        BRA     PFLUSH3

INIT    LEAX    PXCACHE,PCR
        STX     <PSET1+1,PCR
        STX     <PGET1+1,PCR
        CLRA
        CLRB
        STD     -2,X
        STD     -4,X
        STD     -6,X
        STD     -8,X
        
        STD     -10,X

        LEAX    SQPFCB,PCR
        LDB     #31
LOOP    SET     *
        STA     B,X
        DECB
        BPL     LOOP
        
        RTS

; ———— Gestion des Erreurs ————
DOS_ERR JSR     RPTERR
        LBSR    BEEP
        LBRA    EXIT
              
BAD_SQP LEAX    <BAD_SQP_MSG,PCR
        JMP     PSTRING
        LBRA    EXIT
BAD_SQP_MSG
        FCC     "BAD SPQ FILE"

        FCB     4
                
; ———— Variables Locales ————
EXOBIBA RMB     156
PYCACHE FDB     0        
COORD   FDB     0,0     
        FDB     0,0
PXCACHE RMB     256
SQPFCB  RMB     320     ; FCB fichier

ENDATA  set     *

        END     START

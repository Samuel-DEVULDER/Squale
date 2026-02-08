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

GFX_CMD EQU     $00
GFX_DX  EQU     $05
GFX_DY  EQU     $07
GFX_XH  EQU     $08
GFX_XL  EQU     $09
GFX_YH  EQU     $0A
GFX_YL  EQU     $0B
GFX_COL EQU     $10
BAK_COL EQU     $E787


ZZZ     EQU     $5555

START   LBSR    INIT

* parse filename
        JSR     GETFIL
        LBCS    DOS_ERR

* Fixer extension par défaut (tout accepté)
        BSR     CHKEXT

* cas spécial pour les fichiers TXT
        BSR     CHKTXT
        BEQ     SHOWTXT

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

SHOWSQP BSR     OPEN
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
        DECA
        LBEQ    SQP3
        LBRA    BAD_SQP

DONE    BSR     WAIT
        BSR     CLS
EXIT    BSR     CLOSE
        BSR     CURSON
        JMP     WARMS

SHOWTXT BSR     OPEN
        LBNE    DOS_ERR
        JSR     PCRLF
        LDA     #'U-64  ; HIDE CURSOR
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

BITBUF  FCB     0

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

EXOBIT  LDB     #1
EXOBITS PSHS    U               ; U=0
EXOBUF  LDA     #1
        DECB
        BMI     EXOBIT5
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
EXOBIT5 PULS    D,PC

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

EXOCOOK LDX     #ZZZ
        ABX
        ASLB
        ABX
        LDB     ,X
        BSR     EXOBITS
        ADDD    1,X
        RTS

DPSET   LEAY    -1,Y
        BRA     PSET

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
        BSR     PGETSET
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

PGETSET BSR     PGET

PSET    PSHS    A,X
        TFR     Y,D
PSET1   LDX     #ZZZ    ; PXCACHE /!\ modified
        INCA
        STD     -2,X    ; PYCACHE
        ABX
        LDA     #15
        ANDA    ,S
        STA     ,X

        TSTB
        BEQ     PFLUSH
        PULS    A,X,PC

PGET    PSHS    X
PGET1   LDX     #ZZZ    ; PXCACHE /!\ modified
        CMPD    -2,X    ; PYCACHE
        BHS     HWPGET
PGET2   ABX
        LDA     ,X      ; get color from cache
        PULS    X,PC

PFLUSH  STU     <PFLUSH3-2,PCR
        LDU     #$F000
        TFR     Y,D
        STB     GFX_YH,U
        STA     GFX_YL,U
        STB     GFX_XH,U
        COMB
        STB     GFX_XL,U
PFLUSH1 LDD     #$F0FF
        ANDA    BAK_COL
        ORA     ,X+
        STA     BAK_COL
        ANDA    #15
PFLUSH2 INCB
        CMPA    ,X+
        BEQ     PFLUSH2
        BSR     GFXWAIT
        STA     GFX_COL,U
        STB     GFX_DX,U
        INC     GFX_XL,U
        LDA     #$10            ; ignore DY, DX>0
        STA     ,U
        LDA     ,-X             ; sentinel ?
        BPL     PFLUSH1
        BSR     GFXWAIT
        LDU     #0
PFLUSH3 PULS    A,X,PC

GFXWAIT PSHS    B
        LDB     #4
GFXWAI2 BITB    ,U
        BEQ     GFXWAI2
        PULS    B,PC

INIT    LEAX    PXCACHE,PCR
        STX     <PSET1+1,PCR
        STX     <PGET1+1,PCR
        LDD     #-31*256
        STB     -2,X
        STB     -1,X
        LEAX    SQPFCB,PCR
        STA     -1,X    ; sentinel
        NEGA
INIT2   STB     A,X
        DECA
        BPL     INIT2
        RTS

HWPGET  STA     >$F00B
        LDA     -2,X
        BEQ     PGET2
        LDX     #$F000
        STB     9,X
        LDD     #$0F04
        STA     ,X
HWPGET1 BITB    ,X
        BEQ     HWPGET1
        LDD     #$2003
        ANDB    9,X
        LSRB
        BEQ     HWPGET2
        ORA     #16
HWPGET2 LDA     A,X
        BCS     HWPGET3
        LSRA
        LSRA
        LSRA
        LSRA
HWPGET3 ANDA    #$0F
        PULS    X,PC

* ZX0 decompression
SQP3    LEAU    BITBUF,PCR      ; 1,U = READ
        LDD     #$8001          ; beware : offset = +1 on backward
        STA     ,U
        SEX
        STD     <ZX0OFFS+1,PCR  ; offset =-1 (should be + 1 when backward)
        LEAX    DPSET,PCR
        STX     <ZX0LIT0+3,PCR  ; update DPSET in case of relocation
        LEAX    PGETSET-DPSET,X
        STX     <ZX0OFFS+4,PCR  ; update PGETSET in case of relocation

        LDY     #0
ZX0LITS BSR     ZX0ELIA
        TFR     D,X
ZX0LIT0 JSR     1,U
        JSR     ZZZ     ; /!\ modifdied to DPSET
        LEAX    -1,X
        BNE     ZX0LIT0

        LSL     ,U
        BCS     ZX0NEWO

        BSR     ZX0ELIA
ZX0COPY TFR     D,X
ZX0COP1 LEAY    -1,Y
        TFR     Y,D
ZX0OFFS ADDD    #ZZZ    ; /!\ auto modifiable
        JSR     ZZZ     ; /!\ modified to PGETSET
        LEAX    -1,X
        BNE     ZX0COP1
        LSL     ,U
        BCC     ZX0LITS

ZX0NEWO BSR     ZX0ELIA
        TSTB
        BNE     ZX0NEW1
        LBRA    DONE
ZX0NEW1 JSR     1,U     ; READ

        EORA    #$FE    ; B*128-(A>>1)
        ADDA    #2      ; 
        ADCB    #$FF
        LSRB            ; NOTE B and A are swapped
        RORA            ; BIT0 will be passed to ZX0ELIB
        
        STA     <ZX0OFFS+2,PCR ; faster than EXG + STD
        STB     <ZX0OFFS+1,PCR

        LDD     #1
        BSR     ZX0ELIB
        ADDD    #1
        BRA     ZX0COPY

ZX0ELIA LDD     #1
        BRA     ZX0ELI1
ZX0ELI0 LSL     ,U
        ROLB
        ROLA
ZX0ELI1 LSL     ,U
        BNE     ZX0ELIB
        STA     <ZX0ELIB-1,PCR
        JSR     1,U
        LSLA
        INCA
        STA     ,U
        LDA     #0
ZX0ELIB BCC     ZX0ELI0
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
PXCACHE RMB     256+1
SQPFCB  RMB     320     ; FCB fichier

ENDATA  set     *

        END     START

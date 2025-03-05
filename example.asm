; Comentário
; x = a + b

.DATA

A DB 50
B DB 10
X DB ?

.CODE
.ORG 0
LDA A        ; a
ADD B        ; b
STA X        ; x
HLT


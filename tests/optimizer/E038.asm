; in
add $0,5
mov $2,8
mul $2,$0
mov $1,5
lpb $0
  sub $0,1
  mul $1,$2
lpe
mov $3,7
add $3,$1
mov $0,$3
; out
add $0,5
mov $2,8
mul $2,$0
mov $4,$0
max $4,0
mov $5,$2
pow $5,$4
mov $1,5
mul $1,$5
mov $3,7
add $3,$1
min $0,0
mov $0,$3
; in
mov $4,$0
lpb $0
  mov $2,$1
  add $2,7
  pow $2,$0
  mov $3,$4
  bin $3,$0
  sub $0,1
  mod $3,2
  mul $3,$2
  add $5,$3
lpe
mov $0,$5
add $0,1
; out
mov $4,$0
lpb $0
  mov $2,7
  pow $2,$0
  mov $3,$4
  bin $3,$0
  sub $0,1
  mod $3,2
  mul $3,$2
  add $5,$3
lpe
mov $0,$5
add $0,1

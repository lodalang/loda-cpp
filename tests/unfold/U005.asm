; in
mov $1,$0
mul $1,5
seq $1,30
add $1,$0
seq $1,34
div $1,4
; out
mov $1,$0
mul $1,5
mov $2,$1
lpb $2
  div $1,10
  sub $2,$1
lpe
add $1,$0
mod $1,2
add $1,1
div $1,4

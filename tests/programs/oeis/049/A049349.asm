; A049349: Row sums of triangle A049325.
; 1,7,29,103,405,1599,6141,23863,92773,359791,1396493,5421415,21041397,81670431,317005341,1230432919,4775854213,18537264079,71951401517,279275580103,1083993881877,4207466012031,16331061009213

add $0,4
lpb $0
  sub $0,1
  max $0,2
  add $1,$5
  mul $4,8
  sub $3,$4
  mov $4,$2
  mov $2,$3
  add $2,$1
  add $2,1
  mul $2,2
  add $3,$4
  add $5,$4
  mov $1,$3
  mov $3,$5
  sub $4,$5
lpe
mov $0,$2
div $0,2
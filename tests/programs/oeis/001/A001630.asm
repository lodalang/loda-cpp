; A001630: Tetranacci numbers: a(n) = a(n-1) + a(n-2) + a(n-3) + a(n-4), with a(0)=a(1)=0, a(2)=1, a(3)=2.
; 0,0,1,2,3,6,12,23,44,85,164,316,609,1174,2263,4362,8408,16207,31240,60217,116072,223736,431265,831290,1602363,3088654,5953572,11475879,22120468,42638573,82188492,158423412,305370945,588621422,1134604271,2187020050,4215616688,8125862431,15663103440,30191602609,58196185168,112176753648,216227644865,416792186290,803392769971,1548589354774,2985001955900,5753776266935,11090760347580,21378127925189,41207666495604,79430331035308,153106885803681,295123011259782,568867894594375,1096528122693146

mov $5,1
lpb $0
  sub $0,1
  mov $4,$2
  add $4,$1
  mov $1,$3
  add $1,$2
  mov $3,$2
  mov $2,$5
  add $5,$4
lpe
mov $0,$1
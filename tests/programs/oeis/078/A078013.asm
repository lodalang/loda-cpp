; A078013: Expansion of (1-x)/(1-x+x^3).
; 1,0,0,-1,-1,-1,0,1,2,2,1,-1,-3,-4,-3,0,4,7,7,3,-4,-11,-14,-10,1,15,25,24,9,-16,-40,-49,-33,7,56,89,82,26,-63,-145,-171,-108,37,208,316,279,71,-245,-524,-595,-350,174,769,1119,945,176,-943,-1888,-2064,-1121,767,2831,3952,3185,354,-3598,-6783,-7137,-3539,3244,10381,13920,10676,295,-13625,-24301,-24596,-10971,13330,37926

mov $2,1
lpb $0
  sub $0,1
  mov $4,$1
  mov $1,$3
  sub $3,$2
  mov $2,$4
lpe
mov $0,$2
; A130487: a(n) = Sum_{k=0..n} (k mod 9) (Partial sums of A010878).
; 0,1,3,6,10,15,21,28,36,36,37,39,42,46,51,57,64,72,72,73,75,78,82,87,93,100,108,108,109,111,114,118,123,129,136,144,144,145,147,150,154,159,165,172,180,180,181,183,186,190,195,201,208,216,216,217,219,222,226,231,237,244,252,252,253,255,258,262,267,273,280,288,288,289,291,294,298,303,309,316,324,324,325,327,330,334,339,345,352,360,360,361,363,366,370,375,381,388,396,396

lpb $0
  mov $2,$0
  sub $0,1
  mod $2,9
  add $1,$2
lpe
mov $0,$1



import sys
import math

# x^2 - d*y^2 =1 求佩尔方程特解 https://blog.csdn.net/cj1064789374/article/details/84996346
def sqrtd(d):
    c = int(math.sqrt(d))
    n=0
    fz=0
    fm=1
    ra = [0]*10000
    p = 0
    q = 1
    even=0
    if c*c == d:
        print("pingfangshu")
        return
    while True:
        r = int((c+fz)/fm)
        k = int((d - (r*fm-fz)*(r*fm-fz))/fm)
        ra[n]=r
        n = n +1
        if fm==1 and fz !=0:
            break
        fz = r*fm-fz
        fm = k
        print(r)
        
    if n %2 == 0:
        even=1   
    
    while n>0:
        old_p = p
        p = ra[n-1]*p+q
        q = old_p;
        n = n - 1
    if even:
        q = 2*p*q
        p = 2*p*p+1
        
    print(p,q,p*p-d*q*q)


sqrtd(int(sys.argv[1]))

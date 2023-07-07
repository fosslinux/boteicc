#!/bin/bash
cat > tmp2.M1 <<EOF
:FUNCTION_ret3
mov_eax, %3
ret
:FUNCTION_ret5
mov_eax, %5
ret
:FUNCTION_add
lea_eax,[esp+DWORD] %4
mov_eax,[eax]
lea_ebx,[esp+DWORD] %8
mov_ebx,[ebx]
add_eax,ebx
ret
:FUNCTION_sub
lea_ebx,[esp+DWORD] %4
mov_ebx,[ebx]
lea_eax,[esp+DWORD] %8
mov_eax,[eax]
sub_ebx,eax
mov_eax,ebx
ret
:FUNCTION_add6
lea_eax,[esp+DWORD] %4
mov_eax,[eax]
lea_ebx,[esp+DWORD] %8
mov_ebx,[ebx]
add_eax,ebx
lea_ebx,[esp+DWORD] %12
mov_ebx,[ebx]
add_eax,ebx
lea_ebx,[esp+DWORD] %16
mov_ebx,[ebx]
add_eax,ebx
lea_ebx,[esp+DWORD] %20
mov_ebx,[ebx]
add_eax,ebx
lea_ebx,[esp+DWORD] %24
mov_ebx,[ebx]
add_eax,ebx
ret
EOF

assert() {
  expected="$1"
  input="$2"

  ./chibicc "$input" > tmp.M1 || exit
  blood-elf --little-endian --file tmp.M1 --output tmp-elf.M1 && M1 --file x86_defs.M1 --file /tmp/early/M2libc/x86/libc-core.M1 --file tmp-elf.M1 --little-endian --architecture x86 --file tmp2.M1 --file tmp.M1 --output tmp.hex2 && hex2 --file /tmp/early/M2libc/x86/ELF-x86-debug.hex2 --file tmp.hex2 --output tmp --architecture x86 --base-address 0x8048000 --little-endian
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 '{ return 0; }'
assert 42 '{ return 42; }'
assert 21 '{ return 5+20-4; }'
assert 41 '{ return  12 + 34 - 5 ; }'
assert 47 '{ return 5+6*7; }'
assert 15 '{ return 5*(9-6); }'
assert 4 '{ return (3+5)/2; }'
assert 10 '{ return -10+20; }'
assert 10 '{ return - -10; }'
assert 10 '{ return - - +10; }'

assert 0 '{ return 0==1; }'
assert 1 '{ return 42==42; }'
assert 1 '{ return 0!=1; }'
assert 0 '{ return 42!=42; }'

assert 1 '{ return 0<1; }'
assert 0 '{ return 1<1; }'
assert 0 '{ return 2<1; }'
assert 1 '{ return 0<=1; }'
assert 1 '{ return 1<=1; }'
assert 0 '{ return 2<=1; }'

assert 1 '{ return 1>0; }'
assert 0 '{ return 1>1; }'
assert 0 '{ return 1>2; }'
assert 1 '{ return 1>=0; }'
assert 1 '{ return 1>=1; }'
assert 0 '{ return 1>=2; }'

assert 3 '{ int a; a=3; return a; }'
assert 3 '{ int a=3; return a; }'
assert 8 '{ int a=3; int z=5; return a+z; }'

assert 3 '{ int a=3; return a; }'
assert 8 '{ int a=3; int z=5; return a+z; }'
assert 6 '{ int a; int b; a=b=3; return a+b; }'
assert 3 '{ int foo=3; return foo; }'
assert 8 '{ int foo123=3; int bar=5; return foo123+bar; }'

assert 1 '{ return 1; 2; 3; }'
assert 2 '{ 1; return 2; 3; }'
assert 3 '{ 1; 2; return 3; }'

assert 3 '{ {1; {2;} return 3;} }'
assert 5 '{ ;;; return 5; }'

assert 3 '{ if (0) return 2; return 3; }'
assert 3 '{ if (1-1) return 2; return 3; }'
assert 2 '{ if (1) return 2; return 3; }'
assert 2 '{ if (2-1) return 2; return 3; }'
assert 4 '{ if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 '{ if (1) { 1; 2; return 3; } else { return 4; } }'

assert 55 '{ int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 3 '{ for (;;) return 3; return 5; }'

assert 10 '{ int i=0; while(i<10) i=i+1; return i; }'

assert 3 '{ {1; {2;} return 3;} }'

assert 10 '{ int i=0; while(i<10) i=i+1; return i; }'
assert 55 '{ int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} return j; }'

assert 3 '{ int x=3; return *&x; }'
assert 3 '{ int x=3; int *y=&x; int **z=&y; return **z; }'
assert 5 '{ int x=3; int y=5; return *(&x+1); }'
assert 3 '{ int x=3; int y=5; return *(&y-1); }'
assert 5 '{ int x=3; int y=5; return *(&x-(-1)); }'
assert 5 '{ int x=3; int *y=&x; *y=5; return x; }'
assert 7 '{ int x=3; int y=5; *(&x+1)=7; return y; }'
assert 7 '{ int x=3; int y=5; *(&y-2+1)=7; return x; }'
assert 5 '{ int x=3; return (&x+2)-&x+3; }'
assert 8 '{ int x, y; x=3; y=5; return x+y; }'
assert 8 '{ int x=3, y=5; return x+y; }'

assert 3 '{ return ret3(); }'
assert 5 '{ return ret5(); }'
assert 8 '{ return add(3, 5); }'
assert 2 '{ return sub(5, 3); }'
assert 21 '{ return add6(1,2,3,4,5,6); }'
assert 66 '{ return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
assert 136 '{ return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'

echo OK

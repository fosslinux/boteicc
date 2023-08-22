int main() {
  _TEST_ASSERT(0, 0);
  _TEST_ASSERT(42, 42);
  _TEST_ASSERT(21, 5+20-4);
  _TEST_ASSERT(41,  12 + 34 - 5 );
  _TEST_ASSERT(47, 5+6*7);
  _TEST_ASSERT(15, 5*(9-6));
  _TEST_ASSERT(4, (3+5)/2);
  _TEST_ASSERT(10, -10+20);
  _TEST_ASSERT(10, - -10);
  _TEST_ASSERT(10, - - +10);

  _TEST_ASSERT(0, 0==1);
  _TEST_ASSERT(1, 42==42);
  _TEST_ASSERT(1, 0!=1);
  _TEST_ASSERT(0, 42!=42);

  _TEST_ASSERT(1, 0<1);
  _TEST_ASSERT(0, 1<1);
  _TEST_ASSERT(0, 2<1);
  _TEST_ASSERT(1, 0<=1);
  _TEST_ASSERT(1, 1<=1);
  _TEST_ASSERT(0, 2<=1);

  _TEST_ASSERT(1, 1>0);
  _TEST_ASSERT(0, 1>1);
  _TEST_ASSERT(0, 1>2);
  _TEST_ASSERT(1, 1>=0);
  _TEST_ASSERT(1, 1>=1);
  _TEST_ASSERT(0, 1>=2);

  _TEST_ASSERT(0, 1073741824 * 100 / 100);

  _TEST_ASSERT(7, ({ int i=2; i+=5; i; }));
  _TEST_ASSERT(7, ({ int i=2; i+=5; }));
  _TEST_ASSERT(3, ({ int i=5; i-=2; i; }));
  _TEST_ASSERT(3, ({ int i=5; i-=2; }));
  _TEST_ASSERT(6, ({ int i=3; i*=2; i; }));
  _TEST_ASSERT(6, ({ int i=3; i*=2; }));
  _TEST_ASSERT(3, ({ int i=6; i/=2; i; }));
  _TEST_ASSERT(3, ({ int i=6; i/=2; }));

  _TEST_ASSERT(3, ({ int i=2; ++i; }));
  _TEST_ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; ++*p; }));
  _TEST_ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; --*p; }));

  _TEST_ASSERT(2, ({ int i=2; i++; }));
  _TEST_ASSERT(2, ({ int i=2; i--; }));
  _TEST_ASSERT(3, ({ int i=2; i++; i; }));
  _TEST_ASSERT(1, ({ int i=2; i--; i; }));
  _TEST_ASSERT(1, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; *p++; }));
  _TEST_ASSERT(1, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; *p--; }));

  _TEST_ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[0]; }));
  _TEST_ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*(p--))--; a[1]; }));
  _TEST_ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p)--; a[2]; }));
  _TEST_ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p)--; p++; *p; }));

  _TEST_ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[0]; }));
  _TEST_ASSERT(0, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[1]; }));
  _TEST_ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; a[2]; }));
  _TEST_ASSERT(2, ({ int a[3]; a[0]=0; a[1]=1; a[2]=2; int *p=a+1; (*p++)--; *p; }));

  _TEST_ASSERT(0, !1);
  _TEST_ASSERT(0, !2);
  _TEST_ASSERT(1, !0);
  _TEST_ASSERT(1, !(char)0);
  _TEST_ASSERT(0, !(long)3);
  _TEST_ASSERT(4, sizeof(!(char)0));
  _TEST_ASSERT(4, sizeof(!(long)0));

  _TEST_ASSERT(-1, ~0);
  _TEST_ASSERT(0, ~-1);

  _TEST_ASSERT(5, 17%6);
  _TEST_ASSERT(5, ((long)17)%6);
  _TEST_ASSERT(2, ({ int i=10; i%=4; i; }));
  _TEST_ASSERT(2, ({ long i=10; i%=4; i; }));

  _TEST_ASSERT(0, 0&1);
  _TEST_ASSERT(1, 3&1);
  _TEST_ASSERT(3, 7&3);
  _TEST_ASSERT(10, -1&10);

  _TEST_ASSERT(1, 0|1);
  _TEST_ASSERT(0b10011, 0b10000|0b00011);

  _TEST_ASSERT(0, 0^0);
  _TEST_ASSERT(0, 0b1111^0b1111);
  _TEST_ASSERT(0b110100, 0b111000^0b001100);

  _TEST_ASSERT(2, ({ int i=6; i&=3; i; }));
  _TEST_ASSERT(7, ({ int i=6; i|=3; i; }));
  _TEST_ASSERT(10, ({ int i=15; i^=5; i; }));

  _TEST_ASSERT(1, 1<<0);
  _TEST_ASSERT(8, 1<<3);
  _TEST_ASSERT(10, 5<<1);
  _TEST_ASSERT(2, 5>>1);
  _TEST_ASSERT(-1, -1>>1);
  _TEST_ASSERT(1, ({ int i=1; i<<=0; i; }));
  _TEST_ASSERT(8, ({ int i=1; i<<=3; i; }));
  _TEST_ASSERT(10, ({ int i=5; i<<=1; i; }));
  _TEST_ASSERT(2, ({ int i=5; i>>=1; i; }));
  _TEST_ASSERT(-1, -1);
  _TEST_ASSERT(-1, ({ int i=-1; i; }));
  _TEST_ASSERT(-1, ({ int i=-1; i>>=1; i; }));

  _TEST_ASSERT(2, 0?1:2);
  _TEST_ASSERT(1, 1?1:2);
  _TEST_ASSERT(-1, 0?-2:-1);
  _TEST_ASSERT(-2, 1?-2:-1);
  _TEST_ASSERT(4, sizeof(0?1:2));
  _TEST_ASSERT(4, sizeof(0?(short)1:(short)2));
  _TEST_ASSERT(-1, 0?(long)-2:-1);
  _TEST_ASSERT(-1, 0?-2:(long)-1);
  _TEST_ASSERT(-2, 1?(long)-2:-1);
  _TEST_ASSERT(-2, 1?-2:(long)-1);

  1 ? -2 : (void)-1;

  _TEST_ASSERT(20, ({ int x; int *p=&x; p+20-p; }));
  _TEST_ASSERT(1, ({ int x; int *p=&x; p+20-p>0; }));
  _TEST_ASSERT(-20, ({ int x; int *p=&x; p-20-p; }));
  _TEST_ASSERT(1, ({ int x; int *p=&x; p-20-p<0; }));

  _TEST_ASSERT(15, (char *)0xffffffffffffffff - (char *)0xfffffffffffffff0);
  _TEST_ASSERT(-15, (char *)0xfffffffffffffff0 - (char *)0xffffffffffffffff);

  return 0;
}

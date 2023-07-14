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

  return 0;
}

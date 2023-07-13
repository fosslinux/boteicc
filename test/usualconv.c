int main() {
  _TEST_ASSERT((long)-5, -10 + (long)5);
  _TEST_ASSERT((long)-15, -10 - (long)5);
  _TEST_ASSERT((long)-50, -10 * (long)5);
  _TEST_ASSERT((long)-2, -10 / (long)5);

  _TEST_ASSERT(1, -2 < (long)-1);
  _TEST_ASSERT(1, -2 <= (long)-1);
  _TEST_ASSERT(0, -2 > (long)-1);
  _TEST_ASSERT(0, -2 >= (long)-1);

  _TEST_ASSERT(1, (long)-2 < -1);
  _TEST_ASSERT(1, (long)-2 <= -1);
  _TEST_ASSERT(0, (long)-2 > -1);
  _TEST_ASSERT(0, (long)-2 >= -1);

  _TEST_ASSERT(0, 2147483647 + 2147483647 + 2);
  _TEST_ASSERT((long)-1, ({ long x; x=-1; x; }));

  _TEST_ASSERT(1, ({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[0]; }));
  _TEST_ASSERT(0, ({ char x[3]; x[0]=0; x[1]=1; x[2]=2; char *y=x+1; y[-1]; }));
  _TEST_ASSERT(5, ({ struct t {char a;} x, y; x.a=5; y=x; y.a; }));

  return 0;
}

int main() {
  _TEST_ASSERT(8, ({ union { int a; char b[6]; } x; sizeof(x); }));
  _TEST_ASSERT(3, ({ union { int a; char b[4]; } x; x.a = 515; x.b[0]; }));
  _TEST_ASSERT(2, ({ union { int a; char b[4]; } x; x.a = 515; x.b[1]; }));
  _TEST_ASSERT(0, ({ union { int a; char b[4]; } x; x.a = 515; x.b[2]; }));
  _TEST_ASSERT(0, ({ union { int a; char b[4]; } x; x.a = 515; x.b[3]; }));

  _TEST_ASSERT(3, ({ union {int a,b;} x,y; x.a=3; y.a=5; y=x; y.a; }));
  _TEST_ASSERT(3, ({ union {struct {int a,b;} c;} x,y; x.c.b=3; y.c.b=5; y=x; y.c.b; }));

  return 0;
}

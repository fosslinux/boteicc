int g1, g2[4];

int main() {
  _TEST_ASSERT(3, ({ int a; a=3; a; }));
  _TEST_ASSERT(3, ({ int a=3; a; }));
  _TEST_ASSERT(8, ({ int a=3; int z=5; a+z; }));

  _TEST_ASSERT(3, ({ int a=3; a; }));
  _TEST_ASSERT(8, ({ int a=3; int z=5; a+z; }));
  _TEST_ASSERT(6, ({ int a; int b; a=b=3; a+b; }));
  _TEST_ASSERT(3, ({ int foo=3; foo; }));
  _TEST_ASSERT(8, ({ int foo123=3; int bar=5; foo123+bar; }));

  _TEST_ASSERT(4, ({ int x; sizeof(x); }));
  _TEST_ASSERT(4, ({ int x; sizeof x; }));
  _TEST_ASSERT(4, ({ int *x; sizeof(x); }));
  _TEST_ASSERT(16, ({ int x[4]; sizeof(x); }));
  _TEST_ASSERT(48, ({ int x[3][4]; sizeof(x); }));
  _TEST_ASSERT(16, ({ int x[3][4]; sizeof(*x); }));
  _TEST_ASSERT(4, ({ int x[3][4]; sizeof(**x); }));
  _TEST_ASSERT(5, ({ int x[3][4]; sizeof(**x) + 1; }));
  _TEST_ASSERT(5, ({ int x[3][4]; sizeof **x + 1; }));
  _TEST_ASSERT(4, ({ int x[3][4]; sizeof(**x + 1); }));
  _TEST_ASSERT(4, ({ int x=1; sizeof(x=2); }));
  _TEST_ASSERT(1, ({ int x=1; sizeof(x=2); x; }));

  _TEST_ASSERT(0, g1);
  _TEST_ASSERT(3, ({ g1=3; g1; }));
  _TEST_ASSERT(0, ({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[0]; }));
  _TEST_ASSERT(1, ({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[1]; }));
  _TEST_ASSERT(2, ({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[2]; }));
  _TEST_ASSERT(3, ({ g2[0]=0; g2[1]=1; g2[2]=2; g2[3]=3; g2[3]; }));

  _TEST_ASSERT(4, sizeof(g1));
  _TEST_ASSERT(16, sizeof(g2));

  _TEST_ASSERT(1, ({ char x=1; x; }));
  _TEST_ASSERT(1, ({ char x=1; char y=2; x; }));
  _TEST_ASSERT(2, ({ char x=1; char y=2; y; }));

  _TEST_ASSERT(1, ({ char x; sizeof(x); }));
  _TEST_ASSERT(10, ({ char x[10]; sizeof(x); }));

  _TEST_ASSERT(2, ({ int x=2; { int x=3; } x; }));
  _TEST_ASSERT(2, ({ int x=2; { int x=3; } int y=4; x; }));
  _TEST_ASSERT(3, ({ int x=2; { x=3; } x; }));

  _TEST_ASSERT(7, ({ int x; int y; char z; char *a=&y; char *b=&z; b-a; }));
  _TEST_ASSERT(1, ({ int x; char y; int z; char *a=&y; char *b=&z; b-a; }));

  _TEST_ASSERT(4, ({ long x; sizeof(x); }));
  _TEST_ASSERT(2, ({ short x; sizeof(x); }));

  _TEST_ASSERT(12, ({ char *x[3]; sizeof(x); }));
  _TEST_ASSERT(4, ({ char (*x)[3]; sizeof(x); }));
  _TEST_ASSERT(1, ({ char (x); sizeof(x); }));
  _TEST_ASSERT(3, ({ char (x)[3]; sizeof(x); }));
  _TEST_ASSERT(12, ({ char (x[3])[4]; sizeof(x); }));
  _TEST_ASSERT(4, ({ char (x[3])[4]; sizeof(x[0]); }));
  _TEST_ASSERT(3, ({ char *x[3]; char y; x[0]=&y; y=3; x[0][0]; }));
  _TEST_ASSERT(4, ({ char x[3]; char (*y)[3]=x; y[0][0]=4; y[0][0]; }));

  return 0;
}

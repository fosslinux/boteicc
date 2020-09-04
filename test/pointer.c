int main() {
  _TEST_ASSERT(3, ({ int x=3; *&x; }));
  _TEST_ASSERT(3, ({ int x=3; int *y=&x; int **z=&y; **z; }));
  _TEST_ASSERT(5, ({ int x=3; int y=5; *(&x+1); }));
  _TEST_ASSERT(3, ({ int x=3; int y=5; *(&y-1); }));
  _TEST_ASSERT(5, ({ int x=3; int y=5; *(&x-(-1)); }));
  _TEST_ASSERT(5, ({ int x=3; int *y=&x; *y=5; x; }));
  _TEST_ASSERT(7, ({ int x=3; int y=5; *(&x+1)=7; y; }));
  _TEST_ASSERT(7, ({ int x=3; int y=5; *(&y-2+1)=7; x; }));
  _TEST_ASSERT(5, ({ int x=3; (&x+2)-&x+3; }));
  _TEST_ASSERT(8, ({ int x, y; x=3; y=5; x+y; }));
  _TEST_ASSERT(8, ({ int x=3, y=5; x+y; }));

  _TEST_ASSERT(3, ({ int x[2]; int *y=&x; *y=3; *x; }));

  _TEST_ASSERT(3, ({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *x; }));
  _TEST_ASSERT(4, ({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+1); }));
  _TEST_ASSERT(5, ({ int x[3]; *x=3; *(x+1)=4; *(x+2)=5; *(x+2); }));

  _TEST_ASSERT(0, ({ int x[2][3]; int *y=x; *y=0; **x; }));
  _TEST_ASSERT(1, ({ int x[2][3]; int *y=x; *(y+1)=1; *(*x+1); }));
  _TEST_ASSERT(2, ({ int x[2][3]; int *y=x; *(y+2)=2; *(*x+2); }));
  _TEST_ASSERT(3, ({ int x[2][3]; int *y=x; *(y+3)=3; **(x+1); }));
  _TEST_ASSERT(4, ({ int x[2][3]; int *y=x; *(y+4)=4; *(*(x+1)+1); }));
  _TEST_ASSERT(5, ({ int x[2][3]; int *y=x; *(y+5)=5; *(*(x+1)+2); }));

  _TEST_ASSERT(3, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *x; }));
  _TEST_ASSERT(4, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+1); }));
  _TEST_ASSERT(5, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); }));
  _TEST_ASSERT(5, ({ int x[3]; *x=3; x[1]=4; x[2]=5; *(x+2); }));
  _TEST_ASSERT(5, ({ int x[3]; *x=3; x[1]=4; 2[x]=5; *(x+2); }));

  _TEST_ASSERT(0, ({ int x[2][3]; int *y=x; y[0]=0; x[0][0]; }));
  _TEST_ASSERT(1, ({ int x[2][3]; int *y=x; y[1]=1; x[0][1]; }));
  _TEST_ASSERT(2, ({ int x[2][3]; int *y=x; y[2]=2; x[0][2]; }));
  _TEST_ASSERT(3, ({ int x[2][3]; int *y=x; y[3]=3; x[1][0]; }));
  _TEST_ASSERT(4, ({ int x[2][3]; int *y=x; y[4]=4; x[1][1]; }));
  _TEST_ASSERT(5, ({ int x[2][3]; int *y=x; y[5]=5; x[1][2]; }));

  return 0;
}

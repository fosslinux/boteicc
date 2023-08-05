int main() {
  _TEST_ASSERT(1, ({ int x[3]={1,2,3}; x[0]; }));
  _TEST_ASSERT(2, ({ int x[3]={1,2,3}; x[1]; }));
  _TEST_ASSERT(3, ({ int x[3]={1,2,3}; x[2]; }));
  _TEST_ASSERT(3, ({ int x[3]={1,2,3}; x[2]; }));

  _TEST_ASSERT(2, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[0][1]; }));
  _TEST_ASSERT(4, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][0]; }));
  _TEST_ASSERT(6, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][2]; }));

  _TEST_ASSERT(0, ({ int x[3]={}; x[0]; }));
  _TEST_ASSERT(0, ({ int x[3]={}; x[1]; }));
  _TEST_ASSERT(0, ({ int x[3]={}; x[2]; }));

  _TEST_ASSERT(2, ({ int x[2][3]={{1,2}}; x[0][1]; }));
  _TEST_ASSERT(0, ({ int x[2][3]={{1,2}}; x[1][0]; }));
  _TEST_ASSERT(0, ({ int x[2][3]={{1,2}}; x[1][2]; }));

  _TEST_ASSERT('a', ({ char x[4]="abc"; x[0]; }));
  _TEST_ASSERT('c', ({ char x[4]="abc"; x[2]; }));
  _TEST_ASSERT(0, ({ char x[4]="abc"; x[3]; }));
  _TEST_ASSERT('a', ({ char x[2][4]={"abc","def"}; x[0][0]; }));
  _TEST_ASSERT(0, ({ char x[2][4]={"abc","def"}; x[0][3]; }));
  _TEST_ASSERT('d', ({ char x[2][4]={"abc","def"}; x[1][0]; }));
  _TEST_ASSERT('f', ({ char x[2][4]={"abc","def"}; x[1][2]; }));

  _TEST_ASSERT(4, ({ int x[]={1,2,3,4}; x[3]; }));
  _TEST_ASSERT(16, ({ int x[]={1,2,3,4}; sizeof(x); }));
  _TEST_ASSERT(4, ({ char x[]="foo"; sizeof(x); }));

  _TEST_ASSERT(4, ({ typedef char T[]; T x="foo"; T y="x"; sizeof(x); }));
  _TEST_ASSERT(2, ({ typedef char T[]; T x="foo"; T y="x"; sizeof(y); }));
  _TEST_ASSERT(2, ({ typedef char T[]; T x="x"; T y="foo"; sizeof(x); }));
  _TEST_ASSERT(4, ({ typedef char T[]; T x="x"; T y="foo"; sizeof(y); }));

  return 0;
}

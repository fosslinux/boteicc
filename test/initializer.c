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

  return 0;
}

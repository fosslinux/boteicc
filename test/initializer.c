int main() {
  _TEST_ASSERT(1, ({ int x[3]={1,2,3}; x[0]; }));
  _TEST_ASSERT(2, ({ int x[3]={1,2,3}; x[1]; }));
  _TEST_ASSERT(3, ({ int x[3]={1,2,3}; x[2]; }));
  _TEST_ASSERT(3, ({ int x[3]={1,2,3}; x[2]; }));

  _TEST_ASSERT(2, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[0][1]; }));
  _TEST_ASSERT(4, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][0]; }));
  _TEST_ASSERT(6, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][2]; }));

  return 0;
}

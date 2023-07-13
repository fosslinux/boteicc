int main() {
  _TEST_ASSERT(1, ({ char x; sizeof(x); }));
  _TEST_ASSERT(2, ({ short int x; sizeof(x); }));
  _TEST_ASSERT(2, ({ int short x; sizeof(x); }));
  _TEST_ASSERT(4, ({ int x; sizeof(x); }));
  _TEST_ASSERT(4, ({ long int x; sizeof(x); }));
  _TEST_ASSERT(4, ({ int long x; sizeof(x); }));

  _TEST_ASSERT(0, ({ _Bool x=0; x; }));
  _TEST_ASSERT(1, ({ _Bool x=1; x; }));
  _TEST_ASSERT(1, ({ _Bool x=2; x; }));
  _TEST_ASSERT(1, (_Bool)1);
  _TEST_ASSERT(1, (_Bool)2);
  _TEST_ASSERT(0, (_Bool)(char)256);

  return 0;
}

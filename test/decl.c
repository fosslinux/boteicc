int main() {
  _TEST_ASSERT(1, ({ char x; sizeof(x); }));
  _TEST_ASSERT(2, ({ short int x; sizeof(x); }));
  _TEST_ASSERT(2, ({ int short x; sizeof(x); }));
  _TEST_ASSERT(4, ({ int x; sizeof(x); }));
  _TEST_ASSERT(4, ({ long int x; sizeof(x); }));
  _TEST_ASSERT(4, ({ int long x; sizeof(x); }));

  return 0;
}

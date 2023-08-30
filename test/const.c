int main() {
  { const x; }
  { int const x; }
  { const int x; }
  { const int const const x; }
  _TEST_ASSERT(5, ({ const x = 5; x; }));
  _TEST_ASSERT(8, ({ const x = 8; int *const y=&x; *y; }));
  _TEST_ASSERT(6, ({ const x = 6; *(const * const)&x; }));

  return 0;
}

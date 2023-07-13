int main() {
  _TEST_ASSERT(0, ({ enum { zero, one, two }; zero; }));
  _TEST_ASSERT(1, ({ enum { zero, one, two }; one; }));
  _TEST_ASSERT(2, ({ enum { zero, one, two }; two; }));
  _TEST_ASSERT(5, ({ enum { five=5, six, seven }; five; }));
  _TEST_ASSERT(6, ({ enum { five=5, six, seven }; six; }));
  _TEST_ASSERT(0, ({ enum { zero, five=5, three=3, four }; zero; }));
  _TEST_ASSERT(5, ({ enum { zero, five=5, three=3, four }; five; }));
  _TEST_ASSERT(3, ({ enum { zero, five=5, three=3, four }; three; }));
  _TEST_ASSERT(4, ({ enum { zero, five=5, three=3, four }; four; }));
  _TEST_ASSERT(4, ({ enum { zero, one, two } x; sizeof(x); }));
  _TEST_ASSERT(4, ({ enum t { zero, one, two }; enum t y; sizeof(y); }));

  return 0;
}

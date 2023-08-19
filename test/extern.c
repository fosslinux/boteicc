// TODO can be expanded once chibicc has multi-file support
extern int ext1;

int main() {
  _TEST_ASSERT(5, ext1);

  extern int ext3;
  _TEST_ASSERT(7, ext3);

  int ext_fn1(int x);
  _TEST_ASSERT(5, ext_fn1(5));

  extern int ext_fn2(int x);
  _TEST_ASSERT(8, ext_fn2(8));

  return 0;
}

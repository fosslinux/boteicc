int _assert_failed(int expected, int actual, char *code);

int ret3();
int addx();

int ret3() {
  return 3;
  return 5;
}

int add2(int x, int y) {
  return x + y;
}

int sub2(int x, int y) {
  return x - y;
}

int add6(int a, int b, int c, int d, int e, int f) {
  return a + b + c + d + e + f;
}

int addx(int *x, int y) {
  return *x + y;
}

int sub_char(char a, char b, char c) {
  return a - b - c;
}

int fib(int x) {
  if (x<=1)
    return 1;
  return fib(x-1) + fib(x-2);
}

int sub_long(long a, long b, long c) {
  return a - b - c;
}

int sub_short(short a, short b, short c) {
  return a - b - c;
}

int g1;

int *g1_ptr() { return &g1; }
char int_to_char(int x) { return x; }

int main() {
  _TEST_ASSERT(3, ret3());
  _TEST_ASSERT(8, add2(3, 5));
  _TEST_ASSERT(2, sub2(5, 3));
  _TEST_ASSERT(21, add6(1,2,3,4,5,6));
  _TEST_ASSERT(66, add6(1,2,add6(3,4,5,6,7,8),9,10,11));
  _TEST_ASSERT(136, add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16));

  _TEST_ASSERT(7, add2(3,4));
  _TEST_ASSERT(1, sub2(4,3));
  _TEST_ASSERT(55, fib(9));

  _TEST_ASSERT(1, ({ sub_char(7, 3, 3); }));

  _TEST_ASSERT(1, sub_long(7, 3, 3));
  _TEST_ASSERT(1, sub_short(7, 3, 3));

  g1 = 3;

  _TEST_ASSERT(3, *g1_ptr());
  _TEST_ASSERT(5, int_to_char(261));

  return 0;
}

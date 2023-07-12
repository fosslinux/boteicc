typedef int MyInt, MyInt2[4];
typedef int;

int main() {
  _TEST_ASSERT(1, ({ typedef int t; t x=1; x; }));
  _TEST_ASSERT(1, ({ typedef struct {int a;} t; t x; x.a=1; x.a; }));
  _TEST_ASSERT(1, ({ typedef int t; t t=1; t; }));
  _TEST_ASSERT(2, ({ typedef struct {int a;} t; { typedef int t; } t x; x.a=2; x.a; }));
  _TEST_ASSERT(4, ({ typedef t; t x; sizeof(x); }));
  _TEST_ASSERT(3, ({ MyInt x=3; x; }));
  _TEST_ASSERT(16, ({ MyInt2 x; sizeof(x); }));

  return 0;
}

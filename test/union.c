int main() {
  _TEST_ASSERT(8, ({ union { int a; char b[6]; } x; sizeof(x); }));
  _TEST_ASSERT(3, ({ union { int a; char b[4]; } x; x.a = 515; x.b[0]; }));
  _TEST_ASSERT(2, ({ union { int a; char b[4]; } x; x.a = 515; x.b[1]; }));
  _TEST_ASSERT(0, ({ union { int a; char b[4]; } x; x.a = 515; x.b[2]; }));
  _TEST_ASSERT(0, ({ union { int a; char b[4]; } x; x.a = 515; x.b[3]; }));

  return 0;
}

int main() {
  _TEST_ASSERT(131585, (int)8590066177);
  _TEST_ASSERT(513, (short)8590066177);
  _TEST_ASSERT(1, (char)8590066177);
  _TEST_ASSERT(1, (long)1);
  _TEST_ASSERT(0, (long)&*(int *)0);
  _TEST_ASSERT(513, ({ int x=512; *(char *)&x=1; x; }));
  _TEST_ASSERT(5, ({ int x=5; long y=(long)&x; *(int*)y; }));

  (void)1;

  _TEST_ASSERT(-1, (char)255);
  _TEST_ASSERT(-1, (signed char)255);
  _TEST_ASSERT(255, (unsigned char)255);
  _TEST_ASSERT(-1, (short)65535);
  _TEST_ASSERT(65535, (unsigned short)65535);
  _TEST_ASSERT(-1, (int)0xffffffff);
  _TEST_ASSERT(0xffffffff, (unsigned)0xffffffff);

  _TEST_ASSERT(1, -1<1);
  _TEST_ASSERT(0, -1<(unsigned)1);
  _TEST_ASSERT(254, (char)127+(char)127);
  _TEST_ASSERT(65534, (short)32767+(short)32767);
  _TEST_ASSERT(-1, -1>>1);
  _TEST_ASSERT(-1, (unsigned long)-1);
  _TEST_ASSERT(2147483647, ((unsigned)-1)>>1);
  _TEST_ASSERT(-50, (-100)/2);
  _TEST_ASSERT(2147483598, ((unsigned)-100)/2);
  _TEST_ASSERT(0, ((long)-1)/(unsigned)100);
  _TEST_ASSERT(-2, (-100)%7);
  _TEST_ASSERT(2, ((unsigned)-100)%7);
  _TEST_ASSERT(3, ((unsigned long)-100)%9);

  _TEST_ASSERT(65535, (int)(unsigned short)65535);
  _TEST_ASSERT(65535, ({ unsigned short x = 65535; x; }));
  _TEST_ASSERT(65535, ({ unsigned short x = 65535; (int)x; }));

  _TEST_ASSERT(-1, ({ typedef short T; T x = 65535; (int)x; }));
  _TEST_ASSERT(65535, ({ typedef unsigned short T; T x = 65535; (int)x; }));

  return 0;
}

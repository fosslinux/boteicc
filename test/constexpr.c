int main() {
  _TEST_ASSERT(10, ({ enum { ten=1+2+3+4 }; ten; }));
  _TEST_ASSERT(1, ({ int i=0; switch(3) { case 5-2+0*3: i++; } i; }));
  _TEST_ASSERT(8, ({ int x[1+1]; sizeof(x); }));
  _TEST_ASSERT(6, ({ char x[8-2]; sizeof(x); }));
  _TEST_ASSERT(6, ({ char x[2*3]; sizeof(x); }));
  _TEST_ASSERT(3, ({ char x[12/4]; sizeof(x); }));
  _TEST_ASSERT(2, ({ char x[12%10]; sizeof(x); }));
  _TEST_ASSERT(0b100, ({ char x[0b110&0b101]; sizeof(x); }));
  _TEST_ASSERT(0b111, ({ char x[0b110|0b101]; sizeof(x); }));
  _TEST_ASSERT(0b110, ({ char x[0b111^0b001]; sizeof(x); }));
  _TEST_ASSERT(4, ({ char x[1<<2]; sizeof(x); }));
  _TEST_ASSERT(2, ({ char x[4>>1]; sizeof(x); }));
  _TEST_ASSERT(2, ({ char x[(1==1)+1]; sizeof(x); }));
  _TEST_ASSERT(1, ({ char x[(1!=1)+1]; sizeof(x); }));
  _TEST_ASSERT(1, ({ char x[(1<1)+1]; sizeof(x); }));
  _TEST_ASSERT(2, ({ char x[(1<=1)+1]; sizeof(x); }));
  _TEST_ASSERT(2, ({ char x[1?2:3]; sizeof(x); }));
  _TEST_ASSERT(3, ({ char x[0?2:3]; sizeof(x); }));
  _TEST_ASSERT(3, ({ char x[(1,3)]; sizeof(x); }));
  _TEST_ASSERT(2, ({ char x[!0+1]; sizeof(x); }));
  _TEST_ASSERT(1, ({ char x[!1+1]; sizeof(x); }));
  _TEST_ASSERT(2, ({ char x[~-3]; sizeof(x); }));
  _TEST_ASSERT(2, ({ char x[(5||6)+1]; sizeof(x); }));
  _TEST_ASSERT(1, ({ char x[(0||0)+1]; sizeof(x); }));
  _TEST_ASSERT(2, ({ char x[(1&&1)+1]; sizeof(x); }));
  _TEST_ASSERT(1, ({ char x[(1&&0)+1]; sizeof(x); }));
  _TEST_ASSERT(3, ({ char x[(int)3]; sizeof(x); }));
  _TEST_ASSERT(15, ({ char x[(char)0xffffff0f]; sizeof(x); }));
  _TEST_ASSERT(0x10f, ({ char x[(short)0xffff010f]; sizeof(x); }));
  _TEST_ASSERT(4, ({ char x[(int)0xfffffffffff+5]; sizeof(x); }));
  _TEST_ASSERT(8, ({ char x[(int*)0+2]; sizeof(x); }));
  _TEST_ASSERT(12, ({ char x[(int*)16-1]; sizeof(x); }));
  _TEST_ASSERT(3, ({ char x[(int*)16-(int*)4]; sizeof(x); }));

  _TEST_ASSERT(255, ({ char x[(unsigned char)0xffffffff]; sizeof(x); }));
  _TEST_ASSERT(0x800f, ({ char x[(unsigned short)0xffff800f]; sizeof(x); }));
  _TEST_ASSERT(1, ({ char x[(unsigned int)0xfffffffffff>>31]; sizeof(x); }));
  _TEST_ASSERT(1, ({ char x[(unsigned)1<-1]; sizeof(x); }));
  _TEST_ASSERT(1, ({ char x[(unsigned)1<=-1]; sizeof(x); }));
  _TEST_ASSERT(0xff, ({char x[(unsigned char)(-1)]; sizeof(x); }));

  return 0;
}

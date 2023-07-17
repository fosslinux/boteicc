/*
 * This is a block comment.
 */

int main() {
  _TEST_ASSERT(3, ({ int x; if (0) x=2; else x=3; x; }));
  _TEST_ASSERT(3, ({ int x; if (1-1) x=2; else x=3; x; }));
  _TEST_ASSERT(2, ({ int x; if (1) x=2; else x=3; x; }));
  _TEST_ASSERT(2, ({ int x; if (2-1) x=2; else x=3; x; }));

  _TEST_ASSERT(55, ({ int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; j; }));

  _TEST_ASSERT(10, ({ int i=0; while(i<10) i=i+1; i; }));

  _TEST_ASSERT(3, ({ 1; {2;} 3; }));
  _TEST_ASSERT(5, ({ ;;; 5; }));

  _TEST_ASSERT(10, ({ int i=0; while(i<10) i=i+1; i; }));
  _TEST_ASSERT(55, ({ int i=0; int j=0; while(i<=10) {j=i+j; i=i+1;} j; }));

  _TEST_ASSERT(3, (1,2,3));
  _TEST_ASSERT(5, ({ int i=2, j=3; (i=5,j)=6; i; }));
  _TEST_ASSERT(6, ({ int i=2, j=3; (i=5,j)=6; j; }));

  _TEST_ASSERT(55, ({ int j=0; for (int i=0; i<=10; i=i+1) j=j+i; j; }));
  _TEST_ASSERT(3, ({ int i=3; int j=0; for (int i=0; i<=10; i=i+1) j=j+i; i; }));

  _TEST_ASSERT(1, 0||1);
  _TEST_ASSERT(1, 0||(2-2)||5);
  _TEST_ASSERT(0, 0||0);
  _TEST_ASSERT(0, 0||(2-2));

  _TEST_ASSERT(0, 0&&1);
  _TEST_ASSERT(0, (2-2)&&5);
  _TEST_ASSERT(1, 1&&5);

  _TEST_ASSERT(3, ({ int i=0; goto a; a: i++; b: i++; c: i++; i; }));
  _TEST_ASSERT(2, ({ int i=0; goto e; d: i++; e: i++; f: i++; i; }));
  _TEST_ASSERT(1, ({ int i=0; goto i; g: i++; h: i++; i: i++; i; }));

  _TEST_ASSERT(1, ({ typedef int foo; goto foo; foo:; 1; }));

  _TEST_ASSERT(3, ({ int i=0; for(;i<10;i++) { if (i == 3) break; } i; }));
  _TEST_ASSERT(4, ({ int i=0; while (1) { if (i++ == 3) break; } i; }));
  _TEST_ASSERT(3, ({ int i=0; for(;i<10;i++) { for (;;) break; if (i == 3) break; } i; }));
  _TEST_ASSERT(4, ({ int i=0; while (1) { while(1) break; if (i++ == 3) break; } i; }));

  _TEST_ASSERT(10, ({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++; } i; }));
  _TEST_ASSERT(6, ({ int i=0; int j=0; for (;i<10;i++) { if (i>5) continue; j++; } j; }));
  _TEST_ASSERT(10, ({ int i=0; int j=0; for(;!i;) { for (;j!=10;j++) continue; break; } j; }));
  _TEST_ASSERT(11, ({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++; } i; }));
  _TEST_ASSERT(5, ({ int i=0; int j=0; while (i++<10) { if (i>5) continue; j++; } j; }));
  _TEST_ASSERT(11, ({ int i=0; int j=0; while(!i) { while (j++!=10) continue; break; } j; }));

  return 0;
}

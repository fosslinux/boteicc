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

  return 0;
}

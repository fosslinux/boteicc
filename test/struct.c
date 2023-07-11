int main() {
  _TEST_ASSERT(1, ({ struct {int a; int b;} x; x.a=1; x.b=2; x.a; }));
  _TEST_ASSERT(2, ({ struct {int a; int b;} x; x.a=1; x.b=2; x.b; }));
  _TEST_ASSERT(1, ({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.a; }));
  _TEST_ASSERT(2, ({ struct {char a; int b; char c;} x; x.b=1; x.b=2; x.c=3; x.b; }));
  _TEST_ASSERT(3, ({ struct {char a; int b; char c;} x; x.a=1; x.b=2; x.c=3; x.c; }));

  _TEST_ASSERT(0, ({ struct {char a; char b;} x[3]; char *p=x; p[0]=0; x[0].a; }));
  _TEST_ASSERT(1, ({ struct {char a; char b;} x[3]; char *p=x; p[1]=1; x[0].b; }));
  _TEST_ASSERT(2, ({ struct {char a; char b;} x[3]; char *p=x; p[2]=2; x[1].a; }));
  _TEST_ASSERT(3, ({ struct {char a; char b;} x[3]; char *p=x; p[3]=3; x[1].b; }));

  _TEST_ASSERT(6, ({ struct {char a[3]; char b[5];} x; char *p=&x; x.a[0]=6; p[0]; }));
  _TEST_ASSERT(7, ({ struct {char a[3]; char b[5];} x; char *p=&x; x.b[0]=7; p[3]; }));

  _TEST_ASSERT(6, ({ struct { struct { char b; } a; } x; x.a.b=6; x.a.b; }));

  _TEST_ASSERT(8, ({ struct {int a;} x; sizeof(x); }));
  _TEST_ASSERT(16, ({ struct {int a; int b;} x; sizeof(x); }));
  _TEST_ASSERT(16, ({ struct {int a, b;} x; sizeof(x); }));
  _TEST_ASSERT(24, ({ struct {int a[3];} x; sizeof(x); }));
  _TEST_ASSERT(32, ({ struct {int a;} x[4]; sizeof(x); }));
  _TEST_ASSERT(48, ({ struct {int a[3];} x[2]; sizeof(x); }));
  _TEST_ASSERT(2, ({ struct {char a; char b;} x; sizeof(x); }));
  _TEST_ASSERT(0, ({ struct {} x; sizeof(x); }));
  _TEST_ASSERT(16, ({ struct {char a; int b;} x; sizeof(x); }));
  _TEST_ASSERT(16, ({ struct {int a; char b;} x; sizeof(x); }));

  _TEST_ASSERT(16, ({ struct t {int a; int b;} x; struct t y; sizeof(y); }));
  _TEST_ASSERT(16, ({ struct t {int a; int b;}; struct t y; sizeof(y); }));
  _TEST_ASSERT(2, ({ struct t {char a[2];}; { struct t {char a[4];}; } struct t y; sizeof(y); }));
  _TEST_ASSERT(3, ({ struct t {int x;}; int t=1; struct t y; y.x=2; t+y.x; }));

  return 0;
}

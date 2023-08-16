char g3 = 3;
short g4 = 4;
int g5 = 5;
long g6 = 6;

int main() {
  _TEST_ASSERT(1, ({ int x[3]={1,2,3}; x[0]; }));
  _TEST_ASSERT(2, ({ int x[3]={1,2,3}; x[1]; }));
  _TEST_ASSERT(3, ({ int x[3]={1,2,3}; x[2]; }));
  _TEST_ASSERT(3, ({ int x[3]={1,2,3}; x[2]; }));

  _TEST_ASSERT(2, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[0][1]; }));
  _TEST_ASSERT(4, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][0]; }));
  _TEST_ASSERT(6, ({ int x[2][3]={{1,2,3},{4,5,6}}; x[1][2]; }));

  _TEST_ASSERT(0, ({ int x[3]={}; x[0]; }));
  _TEST_ASSERT(0, ({ int x[3]={}; x[1]; }));
  _TEST_ASSERT(0, ({ int x[3]={}; x[2]; }));

  _TEST_ASSERT(2, ({ int x[2][3]={{1,2}}; x[0][1]; }));
  _TEST_ASSERT(0, ({ int x[2][3]={{1,2}}; x[1][0]; }));
  _TEST_ASSERT(0, ({ int x[2][3]={{1,2}}; x[1][2]; }));

  _TEST_ASSERT('a', ({ char x[4]="abc"; x[0]; }));
  _TEST_ASSERT('c', ({ char x[4]="abc"; x[2]; }));
  _TEST_ASSERT(0, ({ char x[4]="abc"; x[3]; }));
  _TEST_ASSERT('a', ({ char x[2][4]={"abc","def"}; x[0][0]; }));
  _TEST_ASSERT(0, ({ char x[2][4]={"abc","def"}; x[0][3]; }));
  _TEST_ASSERT('d', ({ char x[2][4]={"abc","def"}; x[1][0]; }));
  _TEST_ASSERT('f', ({ char x[2][4]={"abc","def"}; x[1][2]; }));

  _TEST_ASSERT(4, ({ int x[]={1,2,3,4}; x[3]; }));
  _TEST_ASSERT(16, ({ int x[]={1,2,3,4}; sizeof(x); }));
  _TEST_ASSERT(4, ({ char x[]="foo"; sizeof(x); }));

  _TEST_ASSERT(4, ({ typedef char T[]; T x="foo"; T y="x"; sizeof(x); }));
  _TEST_ASSERT(2, ({ typedef char T[]; T x="foo"; T y="x"; sizeof(y); }));
  _TEST_ASSERT(2, ({ typedef char T[]; T x="x"; T y="foo"; sizeof(x); }));
  _TEST_ASSERT(4, ({ typedef char T[]; T x="x"; T y="foo"; sizeof(y); }));

  _TEST_ASSERT(1, ({ struct {int a; int b; int c;} x={1,2,3}; x.a; }));
  _TEST_ASSERT(2, ({ struct {int a; int b; int c;} x={1,2,3}; x.b; }));
  _TEST_ASSERT(3, ({ struct {int a; int b; int c;} x={1,2,3}; x.c; }));
  _TEST_ASSERT(1, ({ struct {int a; int b; int c;} x={1}; x.a; }));
  _TEST_ASSERT(0, ({ struct {int a; int b; int c;} x={1}; x.b; }));
  _TEST_ASSERT(0, ({ struct {int a; int b; int c;} x={1}; x.c; }));

  _TEST_ASSERT(1, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].a; }));
  _TEST_ASSERT(2, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[0].b; }));
  _TEST_ASSERT(3, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].a; }));
  _TEST_ASSERT(4, ({ struct {int a; int b;} x[2]={{1,2},{3,4}}; x[1].b; }));

  _TEST_ASSERT(0, ({ struct {int a; int b;} x[2]={{1,2}}; x[1].b; }));

  _TEST_ASSERT(0, ({ struct {int a; int b;} x={}; x.a; }));
  _TEST_ASSERT(0, ({ struct {int a; int b;} x={}; x.b; }));

  _TEST_ASSERT(5, ({ typedef struct {int a,b,c,d,e,f;} T; T x={1,2,3,4,5,6}; T y; y=x; y.e; }));
  _TEST_ASSERT(2, ({ typedef struct {int a,b;} T; T x={1,2}; T y, z; z=y=x; z.b; }));

  _TEST_ASSERT(1, ({ typedef struct {int a,b;} T; T x={1,2}; T y=x; y.a; }));

  _TEST_ASSERT(4, ({ union { int a; char b[4]; } x={0x01020304}; x.b[0]; }));
  _TEST_ASSERT(3, ({ union { int a; char b[4]; } x={0x01020304}; x.b[1]; }));

  _TEST_ASSERT(0x01020304, ({ union { struct { char a,b,c,d; } e; int f; } x={{4,3,2,1}}; x.f; }));

  _TEST_ASSERT(3, g3);
  _TEST_ASSERT(4, g4);
  _TEST_ASSERT(5, g5);
  _TEST_ASSERT(6, g6);

  return 0;
}

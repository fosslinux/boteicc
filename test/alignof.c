char g3;
int g4;
long g5;
char g6;

int main() {
  _TEST_ASSERT(1, _Alignof(char));
  _TEST_ASSERT(2, _Alignof(short));
  _TEST_ASSERT(4, _Alignof(int));
  _TEST_ASSERT(4, _Alignof(long));
  _TEST_ASSERT(1, _Alignof(char[3]));
  _TEST_ASSERT(4, _Alignof(int[3]));
  _TEST_ASSERT(1, _Alignof(struct {char a; char b;}[2]));
  _TEST_ASSERT(4, _Alignof(struct {char a; long b;}[2]));

  return 0;
}

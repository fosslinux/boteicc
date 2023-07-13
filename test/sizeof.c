int main() {
  _TEST_ASSERT(1, sizeof(char));
  _TEST_ASSERT(2, sizeof(short));
  _TEST_ASSERT(2, sizeof(short int));
  _TEST_ASSERT(2, sizeof(int short));
  _TEST_ASSERT(4, sizeof(int));
  _TEST_ASSERT(4, sizeof(long));
  _TEST_ASSERT(4, sizeof(long int));
  _TEST_ASSERT(4, sizeof(long int));
  _TEST_ASSERT(4, sizeof(char *));
  _TEST_ASSERT(4, sizeof(int *));
  _TEST_ASSERT(4, sizeof(long *));
  _TEST_ASSERT(4, sizeof(int **));
  _TEST_ASSERT(4, sizeof(int(*)[4]));
  _TEST_ASSERT(16, sizeof(int*[4]));
  _TEST_ASSERT(16, sizeof(int[4]));
  _TEST_ASSERT(48, sizeof(int[3][4]));
  _TEST_ASSERT(8, sizeof(struct {int a; int b;}));

  _TEST_ASSERT(4, sizeof(-10 + (short)5));
  _TEST_ASSERT(4, sizeof(-10 - (short)5));
  _TEST_ASSERT(4, sizeof(-10 * (short)5));
  _TEST_ASSERT(4, sizeof(-10 / (short)5));
  _TEST_ASSERT(4, sizeof((short)-10 + 5));
  _TEST_ASSERT(4, sizeof((short)-10 - 5));
  _TEST_ASSERT(4, sizeof((short)-10 * 5));
  _TEST_ASSERT(4, sizeof((short)-10 / 5));

  return 0;
}

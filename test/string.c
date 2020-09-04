int main() {
  _TEST_ASSERT(0, ""[0]);
  _TEST_ASSERT(1, sizeof(""));

  _TEST_ASSERT(97, "abc"[0]);
  _TEST_ASSERT(98, "abc"[1]);
  _TEST_ASSERT(99, "abc"[2]);
  _TEST_ASSERT(0, "abc"[3]);
  _TEST_ASSERT(4, sizeof("abc"));

  _TEST_ASSERT(7, "\a"[0]);
  _TEST_ASSERT(8, "\b"[0]);
  _TEST_ASSERT(9, "\t"[0]);
  _TEST_ASSERT(10, "\n"[0]);
  _TEST_ASSERT(11, "\v"[0]);
  _TEST_ASSERT(12, "\f"[0]);
  _TEST_ASSERT(13, "\r"[0]);
  _TEST_ASSERT(27, "\e"[0]);

  _TEST_ASSERT(106, "\j"[0]);
  _TEST_ASSERT(107, "\k"[0]);
  _TEST_ASSERT(108, "\l"[0]);

  _TEST_ASSERT(7, "\ax\ny"[0]);
  _TEST_ASSERT(120, "\ax\ny"[1]);
  _TEST_ASSERT(10, "\ax\ny"[2]);
  _TEST_ASSERT(121, "\ax\ny"[3]);

  _TEST_ASSERT(0, "\0"[0]);
  _TEST_ASSERT(16, "\20"[0]);
  _TEST_ASSERT(65, "\101"[0]);
  _TEST_ASSERT(104, "\1500"[0]);
  _TEST_ASSERT(0, "\x00"[0]);
  _TEST_ASSERT(119, "\x77"[0]);

  return 0;
}

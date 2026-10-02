fun outer() {
  fun a() { return b(); }
  fun b() { return 1; }
  return a();
}

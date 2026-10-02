// 호출 규약을 자주 지나는 벤치마크. 재귀 중심이다.
fun fib(n) {
  if (n < 2) return n;
  return fib(n - 1) + fib(n - 2);
}
print fib(22);

// 함수 선언은 본문을 처리하기 전에 이름을 묶으므로 재귀가 된다.
fun fib(n) {
  if (n < 2) return n;
  return fib(n - 1) + fib(n - 2);
}
print fib(10);

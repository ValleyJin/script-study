// 두 단계 이상 떨어진 변수를 잡는 길. 2단계에서 1 을 찍어야 한다.
fun outer() {
  var x = 1;
  fun mid() {
    fun inner() { return x; }
    return inner();
  }
  return mid();
}
print outer();

// 1부의 기준 테스트. 2단계에서 1, 2 를 찍어야 한다.
fun counter() {
  var n = 0;
  return fun() { n = n + 1; return n; };
}
var c = counter();
print c();
print c();

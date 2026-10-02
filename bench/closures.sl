// 클로저를 많이 만드는 벤치마크. 할당 차이를 본다.
fun makeAdder(n) {
  return fun(x) { return x + n; };
}
var i = 0;
var total = 0;
while (i < 20000) {
  var add = makeAdder(i);
  total = total + add(1);
  i = i + 1;
}
print total;

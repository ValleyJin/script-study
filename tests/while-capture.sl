// 블록 끝에서 upvalue 를 닫지 않으면 갈리는 반례. 2단계에서 0, 1 을 찍어야 한다.
var f = nil;
var g = nil;
var i = 0;
while (i < 2) {
  var x = i;
  if (i == 0) f = fun() { return x; };
  if (i == 1) g = fun() { return x; };
  i = i + 1;
}
print f();
print g();

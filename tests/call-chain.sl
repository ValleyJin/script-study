fun k(v) { return fun(ignored) { return v; }; }
print k(1)(2);
print k(k(3)(4))(5);

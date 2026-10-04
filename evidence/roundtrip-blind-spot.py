# 왕복 검사(트리를 글로 찍고 다시 파싱해 또 찍기)가 파서의 우선순위 오류를
# 잡지 못한다는 것을 보인다. study/02 의 1단계 조건이 이 결과를 근거로 삼는다.
def parse_wrong(toks):
    """+ - * / 를 모두 같은 우선순위, 우결합으로 잘못 짠 파서"""
    def expr(i):
        left, i = toks[i], i + 1
        if i < len(toks) and toks[i] in '+-*/':
            op = toks[i]
            right, i = expr(i + 1)
            return (op, left, right), i
        return left, i
    return expr(0)[0]

def show(t):
    if isinstance(t, tuple):
        return f"({show(t[1])} {t[0]} {show(t[2])})"
    return str(t)

def toklist(s):
    return [x for x in s.replace('(', ' ').replace(')', ' ').split() if x]

src = "1 + 2 * 3 - 4"
p1 = show(parse_wrong(toklist(src)))
p2 = show(parse_wrong(toklist(p1)))
print(f"원문                : {src}")
print(f"틀린 파서가 만든 트리 : {p1}")
print(f"올바른 트리          : ((1 + (2 * 3)) - 4)")
print(f"다시 파싱해 찍은 것   : {p2}")
print(f"왕복 일치            : {p1 == p2}")

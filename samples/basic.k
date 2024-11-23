def foo(a, b) a*a + 2*a*b - b*b

def bar(one, two) foo(one, two) < 7+1

def opttest(x) (1+2+x) * (x+(1+2))

foo(5, 2)
bar(6, 3)
opttest(7)

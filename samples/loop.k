def add(a, b) a + b

def loopAround(x)
    for i = 1, i < x, 1.0 in
        add(i, i*2);

loopAround(10)

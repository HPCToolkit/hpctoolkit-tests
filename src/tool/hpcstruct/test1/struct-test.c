volatile int ctr = 0;

#define N 10000000

#define DECLARE(name) void name() { int i; for(i = 0; i < N; i++) { ctr++; } } 
#define DECLARE_INLINE(name) __attribute__((always_inline)) inline void name() { int i; for(i = 0; i < N; i++) { ctr++; } } 
#define CALL(name) name();

#define ALPHABET(F) \
F(a) \
F(b) \
F(c) \
F(d) \
F(e) \
F(f) \
F(g) \
F(h) \
F(i) \
F(j) \
F(k) \
F(l) \
F(m) \
F(n) \
F(o) \
F(p) \
F(q) \
F(r) \
F(s) \
F(t) \
F(u) \
F(v) \
F(w) \
F(x) \
F(y) \
F(z)

ALPHABET(MACRO)



int main(int argc, char **argv)
{
	ALPHABET(CALL)
	return 0;
}

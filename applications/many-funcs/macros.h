#define apply(x) x

#define CF(f) @f ();
// #define DF(f) @void __attribute__((noinline)) f () { return; }
// #define DF(f) @void __attribute__((section("s" #f))) f () { return; }
#define DF(f) @void f() __attribute__((section(("s" # f)))); void f () { return; }

#define F2(F, f) apply(F(f ## 0)) apply(F(f ##1))
#define F8(F, f) F2(F, f ## 00) F2(F, f ## 01) F2(F, f ## 10) F2(F, f ## 11)
#define F32(F, f) F8(F, f ## 00) F8(F, f ## 01) F8(F, f ## 10) F8(F, f ## 11)
#define F128(F, f) F32(F, f ## 00) F32(F, f ## 01) F32(F, f ## 10) F32(F, f ## 11)
#define F512(F, f) F128(F, f ## 00) F128(F, f ## 01) F128(F, f ## 10) F128(F, f ## 11)
#define F2K(F, f) F512(F, f ## 00) F512(F, f ## 01) F512(F, f ## 10) F512(F, f ## 11)
#define F8K(F, f) F2K(F, f ## 00) F2K(F, f ## 01) F2K(F, f ## 10) F2K(F, f ## 11)
#define F32K(F, f) F8K(F, f ## 00) F8K(F, f ## 01) F8K(F, f ## 10) F8K(F, f ## 11)
#define F64K(F, f) F32K(F, f ## 0) F32K(F, f ## 1) 
#define ALL(F, f) F64K(F, f ## 0) F8(F, f ## 1) 

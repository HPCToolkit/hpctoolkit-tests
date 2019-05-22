#include <omp.h>

void
loop(){
    int j;
    for(j=0;j<1000000000;j+=2) j--;
}

void a() { loop(); }
void b() { loop(); }
void c() { loop(); }
void d() { loop(); }
void e() { loop(); }
void f() { loop(); }
void g() { loop(); }
void h() { loop(); }
void i() { loop(); }

int main(int argc, char **argv)
{
#pragma omp parallel num_threads(8)
   {
	switch(omp_get_thread_num()){
	case 0:
		a();
		break;
	case 1:
		b();
		break;
	case 2:
		c();
		break;
	case 3:
		d();
		break;
	case 4:
		e();
		break;
	case 5:
		f();
		break;
	case 6:
		g();
		break;
	case 7:
		i();
		break;
	}
   }
   return 0;
}

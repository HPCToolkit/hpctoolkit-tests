#include <stdio.h>
#include <stdint.h>
int
main(int argc, char **argv)
{
	void *frame = __builtin_frame_address(0);
	printf("frame = %p ra = %p\n", frame, *(uint64_t *)(((char *)frame + sizeof(void *))));
	return 0;
}

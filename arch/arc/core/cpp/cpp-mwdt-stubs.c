#include <toolchain.h>

int __cxa_atexit(void (*destructor)(void *), void *objptr, void *dso)
{
	ARG_UNUSED(destructor);
	ARG_UNUSED(objptr);
	ARG_UNUSED(dso);
	return 0;
}

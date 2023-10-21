
#if defined(__linux__) || defined(__APPLE__)
#include "./fs_posix.c"
#elif defined(_WIN32)
#error "TODO"
#endif

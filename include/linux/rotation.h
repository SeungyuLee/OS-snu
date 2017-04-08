#ifndef __ROTATION_H__
#define __ROTATION_H__

#include <asm/atomic.h>

extern atomic_t rotation;

void set_rotation(int degree);

int get_rotation(void);

void exit_rotlock(void);
#endif

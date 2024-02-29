
// kernel module for raspi to enable 32-bit raspi timestamp counter

// https://stackoverflow.com/questions/31620375/arm-cortex-a7-returning-pmccntr-0-in-kernel-mode-and-ille

#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");

static void enableccntread (void *param)
{
    asm volatile ("mcr p15,0,%0,c9,c14,0" : : "r" (1));
    asm volatile ("mcr p15,0,%0,c9,c12,0" : : "r" (1));
    asm volatile ("mcr p15,0,%0,c9,c12,1" : : "r" (0x80000000));
}

int init_module ()
{
    on_each_cpu (enableccntread, NULL, 1);
    return 0;
}

void cleanup_module ()
{ }

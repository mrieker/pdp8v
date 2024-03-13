
// g++ -Wall -O2 -shared -o libGUIRasPiCtl.so -fPIC -Ibin/javahome/include -Ibin/javahome/include/linux GUIRasPiCtl.cpp

#include "GUIRasPiCtl.h"

#include <alloca.h>
#include <pthread.h>
#include <string.h>

#include "disassemble.h"
#include "gpiolib.h"
#include "memext.h"
#include "memory.h"
#include "miscdefs.h"
#include "shadow.h"

#define EXCKR(x) x; if (env->ExceptionCheck ()) return 0

extern GpioLib *gpio;

int main (int argc, char **argv);

JNIEXPORT int JNICALL Java_GUIRasPiCtl_main (JNIEnv *env, jclass klass, jobjectArray args)
{
    // see how many strings passed in args
    int argc = EXCKR (env->GetArrayLength (args));

    // allocate an array of pointers (one extra for NULL terminator)
    char **argv = (char **) alloca ((argc + 1) * sizeof *argv);

    // copy strings into array
    for (int i = 0; i < argc; i ++) {

        // get string from arg array element
        jstring argstr = (jstring) EXCKR (env->GetObjectArrayElement (args, i));

        // convert to UTF-8 byte array
        char const *argutf = EXCKR (env->GetStringUTFChars (argstr, NULL));

        // copy to stack
        int arglen = strlen (argutf);
        char *arg = (char *) alloca (arglen + 1);
        memcpy (arg, argutf, arglen);
        arg[arglen] = 0;
        argv[i] = arg;

        // release java string
        env->ReleaseStringUTFChars (argstr, argutf);
    }

    // NULL terminate argument list and call main
    argv[argc] = NULL;
    return main (argc, argv);
}

JNIEXPORT jint JNICALL Java_GUIRasPiCtl_getacl (JNIEnv *env, jclass klass)
{
    return (shadow.r.link ? 010000 : 0) | shadow.r.ac;
}

JNIEXPORT jint JNICALL Java_GUIRasPiCtl_getma (JNIEnv *env, jclass klass)
{
    return shadow.r.ma | memext.dframe;
}

JNIEXPORT jint JNICALL Java_GUIRasPiCtl_getpc (JNIEnv *env, jclass klass)
{
    return shadow.r.pc | memext.iframe;
}

JNIEXPORT jint JNICALL Java_GUIRasPiCtl_getir (JNIEnv *env, jclass klass)
{
    return shadow.r.ir;
}

JNIEXPORT jint JNICALL Java_GUIRasPiCtl_getst (JNIEnv *env, jclass klass)
{
    return (jint) shadow.r.state;
}

JNIEXPORT jint JNICALL Java_GUIRasPiCtl_getgpio (JNIEnv *env, jclass klass)
{
    GpioLib *g = gpio;
    if (g == NULL) return -1;
    return g->readgpio ();
}

JNIEXPORT jlong JNICALL Java_GUIRasPiCtl_getcycs (JNIEnv *env, jclass klass)
{
    return shadow.getcycles ();
}

JNIEXPORT void JNICALL Java_GUIRasPiCtl_setsr (JNIEnv *env, jclass klass, jint sr)
{
    char stringbuffer[8];
    sprintf (stringbuffer, "%o", sr & 4095);
    setenv ("switchregister", stringbuffer, 1);
}

JNIEXPORT void JNICALL Java_GUIRasPiCtl_sethalt (JNIEnv *env, jclass klass, jboolean halt)
{
    pthread_mutex_lock (&haltmutex);
    if (halt) {

        // tell raspictl to halt at end of current cycle
        haltflags |= HF_HALTIT;

        // wait for raspictl to halt at end of current cycle
        while (! (haltflags & HF_HALTED)) {
            pthread_cond_wait (&haltcond2, &haltmutex);
        }
    } else {

        // tell raspictl to resume and keep going
        haltflags &= ~ (HF_HALTIT | HF_HALTED);
        pthread_cond_broadcast (&haltcond);
    }
    pthread_mutex_unlock (&haltmutex);
}

JNIEXPORT void JNICALL Java_GUIRasPiCtl_stepcyc (JNIEnv *env, jclass klass)
{
    pthread_mutex_lock (&haltmutex);

    // make sure raspictl will halt at end of cycle
    haltflags |= HF_HALTIT;

    // tell raspictl to resume processing
    haltflags &= ~ HF_HALTED;
    pthread_cond_broadcast (&haltcond);

    // wait for raspictl to halt again
    while (! (haltflags & HF_HALTED)) {
        pthread_cond_wait (&haltcond2, &haltmutex);
    }

    pthread_mutex_unlock (&haltmutex);
}

JNIEXPORT jstring JNICALL Java_GUIRasPiCtl_disassemble (JNIEnv *env, jclass klass, jint ir, jint pc)
{
    std::string str = disassemble (ir, pc);
    return env->NewStringUTF (str.c_str ());
}

JNIEXPORT jint JNICALL Java_GUIRasPiCtl_rdmem (JNIEnv *env, jclass klass, jint addr)
{
    if (addr >= MEMSIZE) return -1;
    return memarray[addr];
}

JNIEXPORT jint JNICALL Java_GUIRasPiCtl_wrmem (JNIEnv *env, jclass klass, jint addr, jint data)
{
    if (addr >= MEMSIZE) return -1;
    data &= 07777;
    memarray[addr] = data;
    return data;
}

JNIEXPORT void JNICALL Java_GUIRasPiCtl_reset (JNIEnv *env, jclass klass, jint addr)
{
    uint32_t sample;

    pthread_mutex_lock (&haltmutex);

    // processor must already be halted
    ASSERT (haltflags & HF_HALTED);

    // tell it to reset but then halt immediately thereafter
    haltflags = HF_RESETIT | HF_HALTIT;
    pthread_cond_broadcast (&haltcond);

    // wait for it to halt again
    while (! (haltflags & HF_HALTED)) {
        pthread_cond_wait (&haltcond2, &haltmutex);
    }

    pthread_mutex_unlock (&haltmutex);

    // raspictl has reset the processor and the tubes are waiting at the end of FETCH1 with the clock still low
    // when resumed, raspictl will sample the GPIO lines, call shadow.clock() and resume processing from there

    // if 15-bit address given, load DF and IF with the frame and JMP to the address
    if (addr >= 0) {

        // set DF and IF registers to given frame
        memext.setdfif ((addr >> 12) & 7);

        // clock has been low for half cycle, at end of FETCH1 as a result of the reset()
        // G_DENA has been asserted for half cycle so we can read MDL,MD coming from cpu
        // processor should be asking us to read from memory
        sample = gpio->readgpio ();
        if ((sample & (G_CLOCK|G_RESET|G_IOS|G_QENA|G_IRQ|G_DENA|G_IOIN|G_DFRM|G_READ|G_WRITE|G_IAK)) !=
                (G_DENA|G_READ)) ABORT ();

        // drive clock high to transition to FETCH2
        shadow.clock (sample);
        gpio->writegpio (false, G_CLOCK);
        gpio->halfcycle (shadow.aluadd ());

        // half way through FETCH2 with clock still high
        // drop clock and start sending it a JMP @0 opcode
        gpio->writegpio (true, 05400 * G_DATA0);
        gpio->halfcycle (shadow.aluadd ());

        // end of FETCH2 - processor should be accepting the opcode we sent it
        sample = gpio->readgpio ();
        uint32_t masked_sample = sample & (G_CLOCK|G_RESET|G_DATA|G_IOS|G_QENA|G_IRQ|G_DENA|G_JUMP|G_IOIN|G_DFRM|G_READ|G_WRITE|G_IAK);
        uint32_t should_be = 05400 * G_DATA0 | G_QENA;
        if (masked_sample != should_be) ABORT ();

        // drive clock high to transition to DEFER1
        // keep sending opcode so tubes will clock it in
        shadow.clock (sample);
        gpio->writegpio (true, 05400 * G_DATA0 | G_CLOCK);
        gpio->halfcycle (shadow.aluadd ());

        // half way through DEFER1 with clock still high
        // drop clock and step to end of cycle
        gpio->writegpio (false, 0);
        gpio->halfcycle (shadow.aluadd ());

        // end of DEFER1 - processor should be asking us to read from memory location 0
        sample = gpio->readgpio ();
        if ((sample & (G_CLOCK|G_RESET|G_DATA|G_IOS|G_QENA|G_IRQ|G_DENA|G_JUMP|G_IOIN|G_DFRM|G_READ|G_WRITE|G_IAK)) !=
                (G_DENA|G_READ)) ABORT ();

        // drive clock high to transition to FETCH2
        shadow.clock (sample);
        gpio->writegpio (false, G_CLOCK);
        gpio->halfcycle (shadow.aluadd ());

        // half way through DEFER2 with clock still high
        // drop clock and start sending it the PC contents
        gpio->writegpio (true, (addr & 07777) * G_DATA0);
        gpio->halfcycle (shadow.aluadd ());

        // end of DEFER2 - processor should be accepting the address we sent it
        sample = gpio->readgpio ();
        masked_sample = sample & (G_CLOCK|G_RESET|G_DATA|G_IOS|G_QENA|G_IRQ|G_DENA|G_JUMP|G_IOIN|G_DFRM|G_READ|G_WRITE|G_IAK);
        should_be = (addr & 07777) * G_DATA0 | G_QENA;
        if (masked_sample != should_be) ABORT ();

        // drive clock high to transition to EXEC1/JMP
        // keep sending the PC contents so it gets clocked into PC
        shadow.clock (sample);
        gpio->writegpio (true, (addr & 07777) * G_DATA0 | G_CLOCK);
        gpio->halfcycle (shadow.aluadd ());

        // drop clock and run to end of EXEC1/JUMP
        gpio->writegpio (false, 0);
        gpio->halfcycle (shadow.aluadd ());

        // end of EXEC1/JUMP - processor should be reading the opcode at the new address
        sample = gpio->readgpio ();
        masked_sample = sample & (G_CLOCK|G_RESET|G_DATA|G_IOS|G_QENA|G_IRQ|G_DENA|G_JUMP|G_IOIN|G_DFRM|G_READ|G_WRITE|G_IAK);
        should_be = (addr & 07777) * G_DATA0 | G_DENA | G_JUMP | G_READ;
        if (masked_sample != should_be) ABORT ();

        // set shadow PC so it shows up in GUI's LEDs - it should already be in tube's LEDs
        // shadow PC will get redundantly set as soon as raspictl's loop calls shadow.clock() when run button is clicked
        shadow.r.pc = addr & 07777;
    }
}

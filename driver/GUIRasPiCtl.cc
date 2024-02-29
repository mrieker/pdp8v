
// g++ -Wall -O2 -shared -o libGUIRasPiCtl.so -fPIC -Ibin/javahome/include -Ibin/javahome/include/linux GUIRasPiCtl.cpp

#include "GUIRasPiCtl.h"

#include <alloca.h>
#include <pthread.h>
#include <string.h>

#include "disassemble.h"
#include "gpiolib.h"
#include "memext.h"
#include "memory.h"
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
    pthread_mutex_lock (&shadow.haltmutex);
    if (halt) {

        // tell shadow.cc to halt at end of current cycle
        shadow.haltflags |= HF_HALTIT;

        // wait for shadow.cc to halt at end of current cycle
        while (! (shadow.haltflags & HF_HALTED)) {
            pthread_cond_wait (&shadow.haltcond2, &shadow.haltmutex);
        }
    } else {

        // tell shadow.cc to resume and keep going
        shadow.haltflags &= ~ (HF_HALTIT | HF_HALTED);
        pthread_cond_broadcast (&shadow.haltcond);
    }
    pthread_mutex_unlock (&shadow.haltmutex);
}

JNIEXPORT void JNICALL Java_GUIRasPiCtl_stepcyc (JNIEnv *env, jclass klass)
{
    pthread_mutex_lock (&shadow.haltmutex);

    // make sure shadow.cc will halt at end of cycle
    shadow.haltflags |=   HF_HALTIT;

    // tell shadow.cc to resume processing
    shadow.haltflags &= ~ HF_HALTED;
    pthread_cond_broadcast (&shadow.haltcond);

    // wait for shadow.cc to halt again
    while (! (shadow.haltflags & HF_HALTED)) {
        pthread_cond_wait (&shadow.haltcond2, &shadow.haltmutex);
    }

    pthread_mutex_unlock (&shadow.haltmutex);
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

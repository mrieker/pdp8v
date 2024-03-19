//    Copyright (C) Mike Rieker, Beverly, MA USA
//    www.outerworldapps.com
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; version 2 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    http://www.gnu.org/licenses/gpl-2.0.html

// inteface GUI java program to raspictl

#include "GUIRasPiCtl.h"

#include <alloca.h>
#include <pthread.h>
#include <string.h>

#include "controls.h"
#include "disassemble.h"
#include "gpiolib.h"
#include "iodevs.h"
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
    if (halt) {
        ctl_halt ();
    } else {
        ctl_run ();
    }
}

JNIEXPORT void JNICALL Java_GUIRasPiCtl_stepcyc (JNIEnv *env, jclass klass)
{
    // processor assumed to already be halted
    if (! ctl_stepcyc ()) ABORT ();
}

JNIEXPORT jstring JNICALL Java_GUIRasPiCtl_disassemble (JNIEnv *env, jclass klass, jint ir, jint pc)
{
    char const *io = iodisas (ir);
    if (io != NULL) {
        char buf[20];
        snprintf (buf, sizeof buf, "%04o/%s", ir, io);
        buf[19] = 0;
        return env->NewStringUTF (buf);
    }
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
    // processor assumed to already be halted
    if (! ctl_reset (addr)) ABORT ();
}

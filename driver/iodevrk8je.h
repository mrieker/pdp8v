#ifndef _IODEVRK8JE_H
#define _IODEVRK8JE_H

// ref: minicomputer handbook 1976 pp 9-115(224)..120(229)

#include "iodevs.h"
#include "miscdefs.h"

struct IODevRK8JE : IODev {
    IODevRK8JE ();
    void ioreset ();
    uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    bool debug;
    bool resetting;
    bool ros[4];
    int fds[4];
    uint32_t nsperus;

    pthread_cond_t cond;
    pthread_cond_t cond2;
    pthread_mutex_t lock;
    pthread_t threadid;

    uint16_t command;       // <11:09> : func code: 0=read data; 1=read all; 2=set write prot; 3=seek only; 4=write data; 5=write all; 6/7=unused
                            //    <08> : interrupt on done
                            //    <07> : set done on seek done
                            //    <06> : block length: 0=256 words; 1=128 words
                            // <05:03> : extended memory address <14:12>
                            // <02:01> : drive select
                            //    <00> : cylinder number<07>

    uint16_t diskaddr;      // <11:05> : cylinder number<06:00>
                            //    <04> : surface number
                            // <03:00> : sector number<03:00>

    uint16_t memaddr;       // memory address <11:00>

    uint16_t status;        // <11> : status: 0=done; 1=busy
                            // <10> : motion: 0=stationary; 1=head in motion
                            // <09> : xfr capacity exceeded
                            // <08> : seek fail
                            // <07> : file not ready
                            // <06> : control busy
                            // <05> : timing error
                            // <04> : write lock error
                            // <03> : crc error
                            // <02> : data request late
                            // <01> : drive status error
                            // <00> : cylinder address error

    uint16_t lastdas[4];

    void startio ();

    static void *threadwrap (void *zhis);
    void thread ();
    void updateirq ();
};

extern IODevRK8JE iodevrk8je;

#endif

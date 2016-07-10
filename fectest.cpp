/* FEClib - simple forward error correction library.
 *
 * fec.c -- forward error correction based on Vandermonde matrices
 * 980624
 * (C) 1997-98 Luigi Rizzo (luigi@iet.unipi.it)
 * (C) 2001 Alain Knaff (alain@knaff.lu)
 * (C) 2016 Edouard Griffiths, F4EXB
 *
 * Portions derived from code by Phil Karn (karn@ka9q.ampr.org),
 * Robert Morelos-Zaragoza (robert@spectra.eng.hawaii.edu) and Hari
 * Thirumoorthy (harit@spectra.eng.hawaii.edu), Aug 1995
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */

#include <iostream>
#include <sys/time.h>
#include <stdint.h>
#include <string.h>

#include "fec.h"

static long long getUSecs()
{
    struct timeval tp;
    gettimeofday(&tp, 0);
    return (long long) tp.tv_sec * 1000000L + tp.tv_usec;
}

bool example1()
{
#pragma pack(push, 1)
    struct Sample
    {
        int16_t i;
        int16_t q;
    };
    struct Header
    {
        uint16_t frameIndex;
        uint8_t  blockIndex;
        uint8_t  filler;
    };

    static const int samplesPerBlock = (512 - sizeof(Header)) / sizeof(Sample);

    struct ProtectedBlock
    {
        Sample samples[samplesPerBlock];
    };
    struct SuperBlock
    {
        Header         header;
        ProtectedBlock protectedBlock;
    };
#pragma pack(pop)

    std::cerr << "Initialize feclib..." << std::endl;
    FEClib::fec_init();

    SuperBlock txBuffer[256];
    ProtectedBlock txRecovery[256];
    unsigned char *dataBlocks[256];
    unsigned char *fecBlocks[256];
    int frameCount = 0;
    const int OriginalCount = 128;
    const int RecoveryCount = 32;
    const int BlockBytes = sizeof(ProtectedBlock);

    // Fill original data
    std::cerr << "Fill original data..." << std::endl;

    for (int i = 0; i < OriginalCount + RecoveryCount; ++i)
    {
        txBuffer[i].header.frameIndex = frameCount;
        txBuffer[i].header.blockIndex = i;

        if (i < OriginalCount)
        {
            txBuffer[i].protectedBlock.samples[0].i = i; // marker
            dataBlocks[i] = (unsigned char *) &txBuffer[i].protectedBlock;
        }
        else
        {
            memset((void *) &txBuffer[i].protectedBlock, 0, sizeof(SuperBlock));
            fecBlocks[i - OriginalCount] = (unsigned char *) &txRecovery[i - OriginalCount];
        }
    }


    // Generate recovery data
    std::cerr << "Generate recovery data..." << std::endl;

    long long ts = getUSecs();

    FEClib::fec_encode(BlockBytes,
            dataBlocks,
            OriginalCount,
            fecBlocks,
            RecoveryCount);

    long long usecs = getUSecs() - ts;

    std::cerr << "Encoded in " << usecs << " microseconds" << std::endl;

    return true;
}

int main()
{
    if (!example1())
    {
        std::cerr << "example1 failed" << std::endl;
        return 1;
    }

    std::cerr << "example1 successful" << std::endl;
    return 0;
}



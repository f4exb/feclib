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
    unsigned char *txDataBlocks[256];
    unsigned char *txFecBlocks[256];
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
            txDataBlocks[i] = (unsigned char *) &txBuffer[i].protectedBlock;
        }
        else
        {
            memset((void *) &txBuffer[i].protectedBlock, 0, sizeof(SuperBlock));
            txFecBlocks[i - OriginalCount] = (unsigned char *) &txRecovery[i - OriginalCount];
        }
    }


    // Generate recovery data
    std::cerr << "Generate recovery data..." << std::endl;

    long long ts = getUSecs();

    FEClib::fec_encode(BlockBytes,
            txDataBlocks,
            OriginalCount,
            txFecBlocks,
            RecoveryCount);

    long long usecs = getUSecs() - ts;

    std::cerr << "Encoded in " << usecs << " microseconds" << std::endl;

    // insert recovery data in sent data

    for (int ir = 0; ir < RecoveryCount; ir++)
    {
        txBuffer[OriginalCount+ir].protectedBlock = txRecovery[ir];
    }

    SuperBlock* rxBuffer = new SuperBlock[256]; // received blocks
    int nbRxBlocks = 0;

    for (int i = 0; i < OriginalCount + RecoveryCount; i++)
    {
        if (i % 6 != 4)
        {
            rxBuffer[nbRxBlocks] = txBuffer[i];
            nbRxBlocks++;
        }
    }

    // simulate reception

    ProtectedBlock  blockZero;
    Sample *samplesBuffer = new Sample[samplesPerBlock * OriginalCount];
    ProtectedBlock* rxData = (ProtectedBlock *) samplesBuffer;
    ProtectedBlock rxRecovery[256];
    unsigned char *rxDataBlocks[256];
    unsigned char *rxFecBlocks[256];
    unsigned int rxFecIndices[256];
    int rxDataIndices[256];
    unsigned int erasedIndices[256];
    int nbRxDataBlocks = 0;
    int nbRxFecBlocks = 0;
    int nbBlocks = 0;

    memset(rxDataIndices, 0, sizeof(int) * 256);

    for (int i = 0; i < nbRxBlocks; i++)
    {
        if (nbBlocks < OriginalCount)
        {
            int blockIndex = rxBuffer[i].header.blockIndex;

            if (blockIndex == 0) // special data
            {
                blockZero = rxBuffer[i].protectedBlock;
                rxDataIndices[blockIndex] = 1;
                nbRxDataBlocks++;
            }
            else if (blockIndex < OriginalCount) // data
            {
                rxData[blockIndex] = rxBuffer[i].protectedBlock;
                rxDataIndices[blockIndex] = 1;
                nbRxDataBlocks++;
            }
            else // FEC
            {
                rxRecovery[nbRxFecBlocks] = rxBuffer[i].protectedBlock;
                rxFecBlocks[nbRxFecBlocks] = (unsigned char *) &rxRecovery[nbRxFecBlocks];
                rxFecIndices[nbRxFecBlocks] = blockIndex - OriginalCount;
                nbRxFecBlocks++;
            }
        }

        nbBlocks++;
    }

    int nbErasedBlocks = 0;

    for (int i = 0; i < OriginalCount; i++)
    {
        if (rxDataIndices[i] == 0)
        {
            erasedIndices[nbErasedBlocks] = i;
            nbErasedBlocks++;
        }

        rxDataBlocks[i] = (unsigned char *) &rxData[i];
    }

    if ((nbRxDataBlocks < OriginalCount) && (nbRxDataBlocks + nbRxFecBlocks >= OriginalCount)) // decoding necessary and feasible
    {
        ts = getUSecs();

        FEClib::fec_decode(BlockBytes,
                rxDataBlocks,
                OriginalCount,
                rxFecBlocks,
                rxFecIndices,
                erasedIndices,
                nbRxFecBlocks);

        usecs = getUSecs() - ts;
        std::cerr << "Decoded in " << usecs << " microseconds" << std::endl;
    }

    std::cerr << "final" << std::endl;

    std::cerr << "zero:"
        << (unsigned int) blockZero.samples[0].i << std::endl;

    for (int i = 1; i < OriginalCount; i++)
    {
        std::cerr << i << ":"
                << (unsigned int) rxData[i].samples[0].i << std::endl;
    }

    delete[] samplesBuffer;
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



# Simple forward erasure correction library
Erasure correction over GF(256) for frames of 128 fixed sized blocks and a variable number of FEC blocks up to 127
It is able to reconstruct original blocks provided 128 blocks are available at the decoder (original + FEC blocks)

This work is derived from the code in `fec.h` and `fec.c` in the [udp-senderx project](https://github.com/simulacre/udp-senderx.git).

<h2>Encoding</h2>

Prototype:

<pre><code>
void fec_encode(int blockSize,
        unsigned char **data_blocks,
        int nrDataBlocks,
        unsigned char **fec_blocks,
        int nrFecBlocks);
</code></pre>

  - **blockSize**: this is the size of a data or FEC block
  - **data_blocks**: this is the array of pointers to data blocks. Its size it at least the number of data blocks (extra slots are ignored)
  - **nrDataBlocks**: this is the number of data blocks and must be equal to 128
  - **fec_blocks**: this is the array of pointers to FEC blocks that you would normally append to data blocks to compose the erasure protected frame. Its size it at least the number of FEC blocks (extra slots are ignored)
  - **nrFecBlocks**: this is the number of FEC blocks (maximum: 127)
  
  
<h2>Decoding</h2>

Prototype:

<pre><code>
void fec_decode(int blockSize,
        unsigned char **data_blocks,
        int nr_data_blocks,
        unsigned char **fec_blocks,
        unsigned int *fec_block_nos,
        unsigned int *erased_blocks,
        short nr_fec_blocks);
</code></pre>

  - **blockSize**: this is the size of a data or FEC block
  - **data_blocks**: this is the array of pointers to data blocks. Its size it at least the number of data blocks (extra slots are ignored). Space must be provided for missing (erased) blocks as they will be restored in place. 
  - **nrDataBlocks**: this is the number of data blocks and must be equal to 128
  - **fec_blocks**: this is the array of pointers to FEC blocks that are received. This must be a contiguous array. There is no need of space for missing blocks.  
  - **fec_block_nos**: this is the array of indices of received FEC blocks starting at 0 for the first FEC block after data.
  - **erased_blocks**: this is the array of indices of edrased data blocks
  - **nr_fec_blocks**: this is the number of received FEC blocks up to the point that the number of original blocks are received
  
Note that as soon as the number of original blocks are received the decoding can take place. Any extra blocks must not be included in the parameters.

<h2>Example</h2>

Program `fectest.cpp` is provided as an example of encoding and decoding blocks with loss of some blocks. 

It simulates the sending of complex samples over UDP where data loss may occur. It uses data frames of 128 blocks. The data blocks are encapsulated in "super" blocks having extra data to be able to identify the blocks by a frame number and a block index in the frame. As FEC blocks are sent after data blocks the FEC block indices simply follow the data block indices i.e. the first FEC block has an index of 128. It uses 32 FEC blocks thus the last index is 159 (128 + 32 = 160 blocks sent in total).

Data loss is simulated by creating the received frame removing one block every 6 blocks at index 4 modulo 6 i.e blocks with indices 4, 10, 16, ... are removed thus 21 FEC blocks are necessary to reconstruct the frame.

The first block (index 0) is considered special and in real life it could contain some frame related data. Thus it receives a special treatment and is not stored with the rest of data blocks. This shows it is not necessary that all data blocks need to be stored in the same structure of contiguous blocks.   
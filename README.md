# AES comparison test

So, due to a recent project relating to the BBC micro:bit, I've had to consider the possibility of implementing the AES algorithm for secure encryption in pure software, since the micro:bit's cortex M4 doesn't come packaged with AES-NI instructions; so before I started working on figuring out the mess that is the micropython C API, I decided to see how quickly I could calculate AES on the three languages that the micro:bit could support.

## What is AES?

The Advanced Encryption Standard (also called Rijndael) is an encryption algorithm first published in 1998 by Belgian cryptographers John Daemen and Vincent Rijmen, as a submission to the AES process run by the NIST between 1997 and 2000, one which aimed to transparently find a secure algorithm to be specified as the new standard for data encryption, just as its predecessor, the DES, was becoming cryptographically insecure. In 2001, and after rigorous testing and examination of all submissions, Rijndael was chosen to be the new standard, and its implementation was published in FIPS-197 (which was the main source for this post). It is a symmetric encryption algorithm, which means that, for its outputs to be decrypted, both parties must be in possession of the same key (which is either 128, 192 or 256 bits), and it relies on bit operations and special mathematical operations to scramble data beyond computable reversability. AES is a block cipher, which means that it encrypts blocks of data rather than a direct stream of bytes (this is done by "stream" ciphers). Each encrypted block is of size equal to that of the key. For this benchmark, we will be using the 128-bit version of AES (called AES-128), as it plenty secure and the simplest to implement.

Now, a quick note about AES:

 - All operations are done in the Rijndael Galois Field Gf(2^8), which is a field of polynomials with coefficients 0 or 1, where addition is analogous to the XOR bitwise operation, and multiplication is done modulo x^8 + x^4 + x^3 + x^1 + 1. This is an irreducible polynomial in this field, which means it can't be represented as a product of any two other polynomials in Gf(2^8). It would be easier if every entry however, is interpreted as its bitwise representation, where 0 in the nth bit means that the coefficient of the nth power in that represented polynomial is 0, and 1 means that the nth power is 1 (for example, x^7 + x^5 + x^3 + x^1 is represented as 10101010). These two operations fit the criteria for forming a field as XOR is a reversible operation ((a XOR b) XOR b = a), multiplication modulo x^8 + x^4 + x^3 + x^1 + 1 is also reversible, and these two operations are distributive due to the way galois multiplication uses XOR (the same way that normal multiplication uses addition). It is thus important to note that every time any operation regarding addition, multiplication, subtraction or division is done, it is using these defined operations and their inverses.

- It is also important to note that the state is stored as a matrix in column-major order, which means the entries 0, 1, 2, 3 appear on the same column top to bottom, then 4, 5, 6, 7... and so on. You can think of the state as a 4x4 matrix, where the entries 0, 1, 2, 3 make the first column vector, entries 4, 5, 6, 7 make the second column vector, and so forth.

The AES algorithm is composed of two steps:

#### The AES key scheduler

This first step consists of expanding the key into a multi-round schedule: in our 128-bit example, there are 11 rounds, so the key goes from 16 bytes to 16*11 = 176 bytes. The key is broken down into 4 chunks of 32-bits (called words), and a recursive formula is applied to get the next words in the sequence, with the first four chunks being the raw contents of the key (note that this formula contains additional conditions in AES-192 and AES-256).

<img src="https://latex.codecogs.com/svg.image?\large&space;\bg{white}&space;W_n=W_{n-4}\;\text{XOR}\;\text{S}(\text{ROT}(W_{n-1},1))\;\text{XOR}\;\text{RConst}_{i/n}\;\text{(}n\equiv&space;0\;\text{mod&space;4)}"/>
<img src="https://latex.codecogs.com/svg.image?\large&space;\bg{white}&space;W_n=W_{n-4}\;\text{XOR}\;W_{n-1}\;\text{(otherwise)}"/>

Here, SUB(W) means substituting all 4 bytes in the word W with their respective entry in the AES S-Box (more on this later), ROT(W, N) means doing a right rotation of the word W by N bytes, and the ith Rconst is given by (0x01 << i) mod (0x11B) in Gf(2^8).

Once the key schedule has been calculated, the Nth block of 16 bytes in the expanded result is called the Nth round key.

#### The AES rounds

All three variants of AES have a set amount of rounds for which they run; in AES-128, there are 11 rounds:

- The initial round only consists of XORing the first round key with the raw block of 16 bytes. This block is now the state.

- This state is then fed through nine other rounds that consist of the following operations (note that all modifications to the state are done in-place):

    1. Sub_Bytes: each of the bytes of the state are replaced with their respective entry in the S-box. An entry in the S-box is computed by an affine transformation where an element is first mapped to its multiplicative inverse, transformed by a matrix and then added to 0x63. Due to how computationally expensive this may be, most implementations keep a lookup table called the S-Box, which is a precomputation of all possible needed results.
    2.  Shift_bytes: Each row of the state matrix is rotated right by its index in the matrix: the first row is left unchanged, the second row is rotated one byte to the right, the third row is rotated two bytes to the right, and the last row is rotated three bytes to the right (or one to the left).
    3.  Mix_Columns: The state is handled like a 4x4 square matrix and transformed by another known, invertible matrix. Dot product is applied following the operators of the Rijndael field.
    4.  Add_round_key: The round key corresponding to the current round is XORed to the state.
 
- The final round consists of all the steps of the normal round excluding Mix_Columns (Sub_bytes, Shift_bytes, and Add_round_key) in that order.

The result is a 16-byte block of encrypted data that is (as of 2025) virtually indecipherable with current computational power. 

## Implementations

### Python

After implementing python on AES and testing it on my machine (Core I5-8365U, 2x8GB 2667mhz RAM on Ubuntu), I got a speed of 37000 blocks/s. If you're curious about the speed of this on micro:bit's Cortex M4 running micropython v2.1.2, with some platform-dependent optimizations, it becomes 56 blocks/s. For the PC implementation, this may seem pretty fast, but, compared to the other entries here, it's actually the slowest result. 

This is because, even though it's supposed to only use bitwise operations, python has a lot of overhead associated with the way it handles data: whenever you pass a literal to python, that data is actually initialized as a PyObject, which is a struct that contains extra information on top of your data. Python is dynamically typed, so it can't make the variables and objects have a fixed data representation: at any moment, it is expected that the type of data will change, and so will the methods. 

This seems convenient to people who don't want to deal with the problems related to static typing, but it causes a lot of overhead because every time you want to, let's say, add two integers together, you will have a memory lookup to get the integer object associated to the first operand, another one to get the integer type, another one associated with calling the addition method of the integer object, where the second operand is dereferenced again to get the underlying data, and then a new PyObject is created to store the result... Needless to say, tons of memory operations happen just for a single trivial operation.

The optimizations I've had to make for python are simple: lists happened to be the fastest iterable I could use to denote blocks of data, and unrolling loops helped reduce the overhead related to for loops in python. Furthermore, directly computing galois multiplication for the state table is quite expensive, so I precalculated the multiplication tables for each byte (tables of 2 and 3), which also reduced the overhead related to manually simulating galois multiplication. This helped speed up the code, that was initially computing only 1000 blocks/s (around 5 blocks/s on micro:bit). This however, still isn't enough to beat other statically typed languages.

### C 

C fares much better on our CPU: We get a speed of roughly 560000 blocks/s, which is a close to a 15x speedup from python. Implementing this as a micropython module for the micro:bit gives close to 4300 blocks/s, which is a 75x speedup. 

The improvements that C makes over python are noticeable, and is mainly related to the overhead we discussed earlier about python: the first constraint, which was types, was easily solved because C doesn't need to keep type information beyond compilation, so all data can be stored as is directly in memory. To add two integers in C, you just need the two operands, which can be directly imported into the CPU and written directly, as C maps very closely to raw CPU instructions, and this coupled with the optimizations that the GCC compiler is able to find can make for large speedups between the two languages.

There was, initially, a pitfall relating to the memory used by the CPU: calling malloc to get memory halved the time (down to about 270000 blocks/s), due to the overhead related to retrieving memory from the heap, and the solution was to use stack memory instead and include a parameter to pass an output address in the function. 

### ASM

This is the only implementation that actually doesn't pay for itself: for about 3x the work, we got only marginally better results than C, with an output of 680000 blocks/s on x86 assembly. I haven't implemented this on micro:bit yet, but I imagine it would result in a similar speedup.

The improvement that was made here is that the assembly uses registers a lot more, which makes for less reads/writes, so less waiting around for memory. Unless we use SIMD instructions, there really isn't a way to get much faster than this...

### AES-NI and SIMD

Except if, somewhere in the middle of looking for SIMD instructions in a x86 ASM instruction reference website, you find that there is an instruction called ``aesenc``, which utilises the AES-NI hardware built into every modern CPU to calculate AES much faster than your software implementation. This incentive is also available on C, but I thought it would be quicker to write directly in assembly. The result: A massive speedup going up to 4000000 (million!) blocks per second. There are no CPU incentives relating to AES on micro:bit, but there is a crypto cell in its nRF58233 chip that may have a similar speedup (100000 blocks/s up from 4300). 

Using SIMD instructions, we can multiply the output speed even more, computing (on my CPU) up to twice the blocks in one go: instead of importing data on one 16 byte ``%xmm`` register, you can load it into a 32-byte ``%ymm`` register, split the data into two 16-byte registers , and run the ``aesenc`` instruction for each of them, doubling throughput. 

There is faster (4x on AVX-512 CPUs) speedup related to using ``%zmm`` registers similarly, but mine can only leverage the AVX2 instruction set for its 256-bit ``%ymm`` registers. The final speed I could get is thus 8000000 blocks per second, which concludes my attempts to speed up AES.

## Conclusion

Although this initially started as a push to make feasible the usage of a secure encryption algorithm on a toy microcontroller I was given for a university project, I feel that there is a lot to learn from this experiment of mine: I've had a good experience deciphering the micropython API to bundle my module with the micro:bit firmware, I've learned a fair bit about optimizing my memory access, and I've gotten a good first look into the basics of using assembly with C, as well as gained knowledge of CPU internals; finally, I've gotten a good opening into parallel processing with SIMD instructions.

However, I still believe that there may be a fair bit of imprecisions in the code: the source of each implementation has thus been provided in this github repo. If you notice anything, or think you could make some improvements to the code, submit an issue and I'll try my best to update it (and the benchmark statistics below).

|Language|Blocks per second|
|-|-|
|Python|3.7 * 10^4|
|Micropython (micro:bit)|5.6 * 10|
|C|5.6 * 10^5|
|C (micro:bit)|4.3 * 10^3|
|assembly|6.8 * 10^5|
|AES-NI|4.0 * 10^6|
|SIMD AES-NI (AVX2)|8.0 * 10^6|

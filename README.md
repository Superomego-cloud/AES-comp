# AES comparison test

So, due to a recent project relating to the BBC micro:bit, I've had to consider the possibility of implementing the AES algorithm for secure encryption in pure software, since the micro:bit's cortex M4 doesn't come packaged with AES-NI instructions; so before I started working on figuring out the mess that is the micropython C API, I decided to see how quickly I could calculate AES on the three languages that the micro:bit could support; but before I get into the details of each implementation, I'd like to first start by showing what is generally expected of an AES implementation:

## What is AES?

The Advanced Encryption Standard (also called Rijndael) is an encryption algorithm first published in 1998 by Belgian cryptographers John Daemen and Vincent Rijmen, as a submission to the AES process run by the NIST between 1997 and 2000, one which aimed to transparently find a secure algorithm to be specified as the new standard for data encryption, just as its predecessor, the DES, was becoming cryptographically insecure. In 2001, and after rigorous testing and examination of all submissions, Rijndael was chosen to be the new standard, and its implementation was published in FIPS-197 (which was the main source for this post). It is a symmetric encryption algorithm, which means that, for its outputs to be decrypted, both parties must be in possession of the same key (which is either 128, 192 or 256 bits), and it relies on bit operations and special mathematical operations to scramble data beyond computable reversability. AES is a block cipher, which means that it encrypts blocks of data rather than a direct stream of bytes (in that case, it is called "stream cipher"). Each encrypted block is of size equal to that of the key. For this benchmark, we will be using the 128-bit version of AES (called AES-128), as it plenty secure and the simplest to implement.

Now, a quick note about AES:

All operations are done in the Rijndael Galois Field Gf(2^8), which is a field of polynomials with coefficients 0 or 1, where addition is analogous to the XOR bitwise operation, and multiplication is done modulo x^8 + x^4 + x^3 + x^1 + 1. This is an irreducible polynomial in this field, which means it can't be represented as a product of any two other polynomials in Gf(2^8). It would be easier if every entry however, is interpreted as its bitwise representation, where 0 in the nth bit means that the coefficient of the nth power in that represented polynomial is 0, and 1 means that the nth power is 1 (for example, x^7 + x^5 + x^3 + x^1 is represented as 10101010). These two operations fit the criteria for forming a field as XOR is a reversible operation ((a XOR b) XOR b = a), multiplication modulo x^8 + x^4 + x^3 + x^1 + 1 is also reversible, and these two operations are distributive due to the way galois multiplication uses XOR (the same way that normal multiplication uses addition). It is thus important to note that every time any operation regarding addition, multiplication, subtraction or division is done, it is using these defined operations and their inverses.

It is also important to note that the state is stored as a matrix in column-major order, which means the entries 0, 1, 2, 3 appear on the same column top to bottom, then 4, 5, 6, 7... and so on. You can think of the state as a 4x4 matrix, where the entries 0, 1, 2, 3 make the first column vector, entries 4, 5, 6, 7 make the second column vector, and so forth.

The AES algorithm is composed of two steps:

#### The AES key scheduler

This first step consists of expanding the key to 11 times its size: in our 128-bit example, it goes from 16 bytes to 176 bytes. The key is broken down into 4 chunks of 32-bits (called words), and a recursive formula is applied to get the next words in the sequence, with the first four chunks being the raw contents of the key (note that this formula is different for AES-192 and AES-256).

<img src="https://latex.codecogs.com/svg.image?\large&space;\bg{white}&space;W_n=W_{n-4}\;\text{XOR}\;\text{S}(\text{ROT}(W_{n-1},1))\;\text{XOR}\;\text{RConst}_{i/n}\;\text{(}n\equiv&space;0\;\text{mod&space;4)}"/>
<img src="https://latex.codecogs.com/svg.image?\large&space;\bg{white}&space;W_n=W_{n-4}\;\text{XOR}\;W_{n-1}\;\text{(otherwise)}"/>

Here, SUB(W) means substituting all 4 bytes in the word W with their respective entry in the AES S-Box (more on this later), and ROT(W, N) means doing a right rotation of the word W by N bytes. 

Once the key has been calculated, the Nth block of 16 bytes in the expanded result is called the Nth round key.

#### The AES rounds

All three variants of AES have a set amount of rounds for which they run: in AES-128, there are 11 rounds:

- The initial round only consists of XORing the first round key with the raw block of 16 bytes. This block is now the state.

- This state is then fed through nine other rounds that consist of the following operations:
    1 - Sub_Bytes: each of the bytes of the state are replaced with their respective entry in the S-box. An entry in the S-box is computed by an affine transformation where an element is first mapped to its multiplicative inverse, transformed by a matrix and then added to 0x63. Due to how computationally expensive this may be, most implementations keep a lookup table called the S-Box, which contains the result for each entry.
    2 - Shift_bytes: Each row of the state matrix is rotated right by its index in the matrix: the first row is left unchanged, the second row is rotated one byte to the right, the third row is rotated two bytes to the right, and the last row is rotated three bytes to the right (or one to the left).

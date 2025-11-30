# AES comparison test

So, due to a recent project relating to the BBC micro:bit, I've had to consider the possibility of implementing the AES (Advanced encryption Standard) algorithm for secure encryption in pure software, since the micro:bit's cortex M4 doesn't come packaged with AES-NI instructions; so before I started working on figuring out the mess that is the micropython C API, I decided to see how quickly I could calculate AES on the three languages that the micro:bit could support; but before I get into the details of each implementation, I'd like to first start by showing what is generally expected of an AES implementation:

# AES 

The AES algorithm is composed of three main steps:

1 - The AES key schedule: 

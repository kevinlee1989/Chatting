1.
my Chat struct has 
    uint32_t id = 4 bytes
    char userName[15]: 15+1(null) bytes
    char message[255]: 255+1(null) bytes
    char time[50]: 50 bytes

Assume no one reacted to them, then total memory for Chat without reactions would be 326 bytes.

that means 
10 chats without reactions would need approximately 10 x 326 = 3260 bytes.
100 chats without reactions would need approximately 32600 bytes.
1000 chats without reactions would need approximately 326000 bytes.

the additional working memory for handling the /chats request depends on the buffer's bytes. 
Since my buffer is allocated once with a size of 1024 bytes, each "snprintf" reuses this buffer to store the message before it goes to the client. The buffer size is fixed to be 1024bytes and it will reuse multiple times, but the total additional memory remains roughly 1024 bytes. Also no matter how many chats are there since the memory usage will not increase beyond the buffer size, total additional memory remains similar since the buffer is reused.

There are two ways to reduce the memory usage when processing a request. First method is limit reaction count by capping MAX_REACTION. Second method is to instead of using static allocation, the usage of dynamic memory allocation will reduce the memory usage because dynamic memory allocation allocate the memory based on actual need at runtime. Also static allocation has a possibility of a memory leak if the majority of messages are much shorther than the maximun, while dynamic allocation memory is allocated based on the specific length of each message which prevents the memory leak.


2.
Lets think that I remove MAX_MESSAGE_LENGTH with is constraint for message length. User would be happy that they don't have limit for the message length, so they can send a message as long as they want. However, this would led to the risk of excessive memory usage which has potential to lead system crash if user send a very very long length of message. 
The problem for removing the MAX_MESSAGE_LENGTH are risk of memory leaks, buffer overflow, complex memory allocation code, slower responsive from the chat server. Memory leaks or buffer overflow can lead to security risks. the code must dynamically allocate memory for every message if messages all have arbitrary length. These complexity can make bugs and memory management more difficult.
The impact on my implementation memory allocation for every message length has to be reworked to dynamically allocate message buffer. Also I have free the buffer when I after I use the buffer, these process makes much harder and complex within the memory management. Also if I have to handle very very long message while maintaining the responsiveness, It might need to have a method that require to divide it into several times such as queuing and batching long messages to avoid blocking other requests. This implementation also requires additional logic and increasing the complexity of the code. 


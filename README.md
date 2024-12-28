# Overview of the Chat Server Implementation in C
This C program implements a basic HTTP chat server that supports various operations such as posting chats, reacting to messages, and managing chat data. The code is designed to handle HTTP requests, parse query parameters, and respond appropriately, all within a custom server setup.

# Key Features
## Chat Management:

### Adding Chats: 
Users can add new chat messages using the /post endpoint. Each chat is given a unique ID and timestamped.
### Adding Reactions: 
The /react endpoint allows users to add reactions to existing chats. This includes a username and a message, subject to character limits.
### Editing Messages:
Users can edit their messages using the /edit endpoint by providing the chat ID and the new message content.
### Resetting Chats: 
All chats and reactions can be reset to initial state using the /reset endpoint, clearing all stored data.

## Request Handling:
The server handles different types of HTTP requests by parsing the path and executing the corresponding function. It supports fetching chats, posting new chats, adding reactions, and editing or resetting chat data.

## Data Structures:
Chats and reactions are stored in predefined arrays, chats and reactions respectively, which are part of the Chat struct. This struct includes fields for usernames, messages, timestamps, and a dynamic list of reactions.

## Utilities:
### URL Decoding: 
Parameters extracted from the URLs are decoded to handle HTTP encoding.
### Date Formatting: 
Timestamps for each chat are formatted to "YYYY-MM-DD HH:MM:SS" using standard C time functions.

## Detailed Function Descriptions
### add_chat(): 
Adds a new chat to the global chats array if there is space available.
### add_reaction(): 
Appends a reaction to a specified chat, checking for valid ID and space constraints.
### handle_post(): 
Processes post requests, extracts parameters, decodes them, and adds the chat.
### handle_reaction(): 
Similar to handle_post but for reactions, including validations for message length.
### reset(): 
Resets all chat and reaction data, preparing the system for new data without needing a restart.

## Security and Error Handling
The implementation includes basic checks for buffer overflows, null pointers, and array bounds. Error messages are sent back to clients when requests cannot be processed correctly.

## Compilation and Execution
To compile and run the server, use a C compiler like GCC and specify the port number as a command-line argument. Example: gcc -o chat_server chat_server.c && ./chat_server 8080

## Possible Improvements
Enhancing security measures to prevent potential exploits.
Implementing persistent storage for chats and reactions.
Adding user authentication to ensure that reactions and edits are performed by the original poster or authorized users.

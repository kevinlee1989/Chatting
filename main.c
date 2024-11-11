    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdint.h>
    #include <time.h>
    #include <assert.h>
    #include <unistd.h>
    #include "http-server.h"

    #define MAX_USERNAME_LENGTH 15
    #define MAX_MESSAGE_LENGTH 255
    #define MAX_CHATS 1000
    #define MAX_TIME 50
    #define MAX_REACTIONS 100

    struct Reaction {
        char username[MAX_USERNAME_LENGTH+1];
        char message[MAX_MESSAGE_LENGTH+1];
    };

    struct Chat{
        uint32_t id;
        char userName[15];
        char message[255];
        char time[MAX_TIME];
        struct Reaction reactions[MAX_REACTIONS];
        uint32_t reaction_count;
    };


    // Global variable
    struct Chat chats[MAX_CHATS];
    static uint32_t current_chat_count = 0;

    uint8_t add_chat(char* username, char* message);
    uint8_t add_reaction(char* username, char* message, char* id_str);
    void reset();
    void respond_with_chats(int client);
    void handle_post(char* path, int client);
    void handle_reaction(char* path, int client);
    char* parse_user(char* path);
    char* parse_message(char* path);
    char* parse_id(char* path);
    void url_decode(char *src, char *dest);


    /****Data Handling Functions****/
    uint8_t add_chat(char* userName, char* message){

        //if chat count is over the limit of chat size then error
        if (current_chat_count >= MAX_CHATS)
        {
            fprintf(stderr, "Error: Maximum number of chats reached \n");
            return 0;
        }


        struct Chat* new_chat = &chats[current_chat_count];
        new_chat -> id = current_chat_count + 1;
        strncpy(new_chat -> userName, userName, MAX_USERNAME_LENGTH);
        new_chat -> userName[MAX_USERNAME_LENGTH] = '\0';
        strncpy(new_chat -> message, message, MAX_MESSAGE_LENGTH);
        new_chat -> message[MAX_MESSAGE_LENGTH] = '\0';


        // showing current time
        time_t now = time(NULL);
        struct tm* current_time = localtime(&now);
        strftime(new_chat-> time, MAX_TIME, "%Y-%m-%d %H:%M", current_time);

        current_chat_count += 1;

        return current_chat_count;
    }


    uint8_t add_reaction(char* username, char* message, char* _id){
        uint32_t id = (uint32_t)atoi(_id);

        if (id == 0 || id > current_chat_count) {
            printf("Error: Invalid chat ID.\n");
            return 0;
        }

        // looking for a chat to react.
        // it chat starts with 0 and id starts with 1 
        struct Chat* chat = &chats[id-1];

        if (chat->reaction_count >= MAX_REACTIONS) {
            printf("Error: Maximum number of reactions reached for chat ID %d.\n", id);
            return 0; // Fail
        }

        struct Reaction* new_reaction = &chat->reactions[chat->reaction_count];
        strncpy(new_reaction->username, username, MAX_USERNAME_LENGTH);
        new_reaction->username[MAX_USERNAME_LENGTH] = '\0';

        strncpy(new_reaction->message, message, MAX_MESSAGE_LENGTH);
        new_reaction->message[MAX_MESSAGE_LENGTH] = '\0';

        // increase reaction count
        chat->reaction_count += 1;

        return 1;
    }


    void reset() {
        // reaction count reset
        uint32_t i =0;
        for (i = 0; i < current_chat_count; i++) {
            chats[i].reaction_count = 0;  
        }
        
        // reseting all the chat count
        current_chat_count = 0;
    }

    /****Request and Response Handling Functions****/
    void respond_with_chats(int client) {
        // temporary buffer for respond
        char buffer[1024]; 
        int fixed_spacing = 37;	


        // chatting data
        uint32_t i =0;
        for ( i = 0; i < current_chat_count; i++) {
            struct Chat* chat = &chats[i];

	    int spacing = fixed_spacing - (int)(strlen(chat->time) + 6 + strlen(chat->userName));
            // format 
            snprintf(buffer, sizeof(buffer), "[#%d %s]%*s%s: %s\n",chat->id, chat->time, spacing, "", chat->userName, chat->message);
	    write(client, buffer, strlen(buffer));

            // reaction
            uint32_t j =0;
            for (j = 0; j < chat->reaction_count; j++) {
                struct Reaction* reaction = &chat->reactions[j];
		int padding = 34 - (int)strlen(reaction->username);  // Calculate remaining spaces to reach 10 characters
            	snprintf(buffer, sizeof(buffer), "%*s(%s)  %s\n", padding, "", reaction->username, reaction->message);
            	write(client, buffer, strlen(buffer));	
            }
        }

    }


    void handle_post(char* path, int client) {

        // extract query parameter from path
        char* user_param = parse_user(path);
        char* message_param = parse_message(path);
	char decoded_user[MAX_USERNAME_LENGTH + 1];
    	char decoded_message[MAX_MESSAGE_LENGTH + 1];
	url_decode(parse_user(path), decoded_user);
    	url_decode(parse_message(path), decoded_message);

        if (!decoded_user[0] || !decoded_message[0]) {
            // if there is no user or message then output error
            const char* error_msg = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nMissing user or message parameter\n";
            write(client, error_msg, strlen(error_msg));
            return;
        }

        if (add_chat(decoded_user, decoded_message)) {
            // if chat sending is succesful
            respond_with_chats(client);
        } else {
            // if chat sending is unsuccesful
            const char* error_msg = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nFailed to add chat\n";
            write(client, error_msg, strlen(error_msg));
        }
    }

    void handle_reaction(char* path, int client) {
        // extract query param
        char* user_param = parse_user(path);
        char* message_param = parse_message(path);
        char* id_param = parse_id(path);
	url_decode(parse_user(path), user_param);
    	url_decode(parse_message(path), message_param);
    	url_decode(parse_id(path), id_param);
        if (!user_param || !message_param || !id_param) {
            // Error
            const char* error_msg = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nMissing user, message, or id parameter\n";
            write(client, error_msg, strlen(error_msg));
            
            return;
        }

        // add reactions
        if (add_reaction(user_param, message_param, id_param)) {
            // if success
            respond_with_chats(client);
        } else {
            // Fail
            const char* error_msg = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nFailed to add reaction\n";
            write(client, error_msg, strlen(error_msg));
        }
    }



    /*****Parsing Functions****/
    char* parse_user(char* path) {
        // static array char type user
        static char user[MAX_USERNAME_LENGTH + 1];
        
        // finding user= part
        const char* user_start = strstr(path, "user=");
        if (user_start == NULL) {
            user[0] = '\0';  // if there is no user_start then return null
            return user;
        }
        user_start += strlen("user=");  // move the user_start after "user=" 

        // putting in to the user array
        int i = 0;
        while (user_start[i] != '\0' && user_start[i] != '&' && i < MAX_USERNAME_LENGTH) {
            user[i] = user_start[i];
            i += 1;
        }
        user[i] = '\0';  // put null at the end of the array

	return user;
    }

    char* parse_message(char* path) {
        static char message[MAX_MESSAGE_LENGTH + 1];
        
        const char* message_start = strstr(path, "message=");
        if (message_start == NULL) {
            message[0] = '\0';
            return message;
        }   
        message_start += strlen("message=");

        int i = 0;
        while (message_start[i] != '\0' && message_start[i] != '&' && i < MAX_MESSAGE_LENGTH) {
            message[i] = message_start[i];
            i += 1;
        }
        message[i] = '\0';

        return message;
    }

    char* parse_id(char* path) {
        static char id[11]; 
        
        const char* id_start = strstr(path, "id=");
        if (id_start == NULL) {
            id[0] = '\0';
            return id;
        }
        id_start += strlen("id=");

        int i = 0;
        while (id_start[i] != '\0' && id_start[i] != '&' && i < 10) {
            id[i] = id_start[i];
            i++;
        }
        id[i] = '\0';

        return id;
    }


void handle_request(char* request, int client) {
   //extracting only first line to render the chat service
   char *line = strtok(request, "\r\n");


    if (strstr(line, "/chats") != NULL) {
        respond_with_chats(client);
    }
    
    else if (strstr(line, "/post?") != NULL) {
        char *message_start = strstr(line, "message=");
        if (message_start) {
            message_start += strlen("message=");
            char *end = strchr(message_start, ' ');
            if (end) {
                *end = '\0';             }
        }
        handle_post(line, client);
    }
    
    else if (strstr(line, "/react?") != NULL) {
        handle_reaction(line, client);
    }
        else if (strstr(line, "/reset") != NULL) {
        reset();
        const char* response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nChat server has been reset.\n";
        write(client, response, strlen(response));
    }

    else {
        const char* error_msg = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nInvalid request\n";
        write(client, error_msg, strlen(error_msg));
    }
}

uint8_t hex_to_byte(char hex) {
    if ('0' <= hex && hex <= '9') return hex - '0';
    if ('a' <= hex && hex <= 'f') return hex - 'a' + 10;
    if ('A' <= hex && hex <= 'F') return hex - 'A' + 10;
    return 0;
}


void url_decode(char *src, char *dest) {
    char *p_src = src;
    char *p_dest = dest;

    while (*p_src != '\0') {
        if (*p_src == '%') {
            //
            uint8_t high = hex_to_byte(*(p_src + 1));
            uint8_t low = hex_to_byte(*(p_src + 2));
            uint8_t final = (high << 4) | low;
            *p_dest = (unsigned char) final;

            // increment
            p_src +=2;

        }  else {
            *p_dest = *p_src;

        }
        p_src++;
        p_dest++;
    }
    *p_dest = '\0'; 
}




int main(int argc, char* argv[]) {
    int port = 8080;  // if no port num then default port num will be 8080

    // setting up port num
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    // message print
    printf("Starting chat server on port %d\n", port);

    // server start
    start_server(handle_request, port);

    return 0;
}

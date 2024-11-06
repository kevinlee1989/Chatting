#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

#define MAX_USERNAME_LENGTH 15
#define MAX_MESSAGE_LENGTH 255
#define MAX_CHATS 100000
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
uint32_t current_chat_count = 0;

uint8_t add_chat(char* username, char* message);
uint8_t add_reaction(char* username, char* message, char* id_str);
void reset();
void respond_with_chats(int client);
void handle_post(char* path, int client);
void handle_reaction(char* path, int client);
char* parse_user(char* path);
char* parse_message(char* path);
char* parse_id(char* path);


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
    strftime(new_chat-> time, MAX_TIME, "%Y year %m month %d day:  %H:%M", current_time);

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

    //return 1;
}


void reset() {
    // reaction count reset
    for (uint32_t i = 0; i < current_chat_count; i++) {
        chats[i].reaction_count = 0;  
    }
    
    // reseting all the chat count
    current_chat_count = 0;
}

/****Request and Response Handling Functions****/
void respond_with_chats(int client) {
    // temporary buffer for respond
    char buffer[1024];  


    // chatting data
    for (uint32_t i = 0; i < current_chat_count; i++) {
        struct Chat* chat = &chats[i];

        // format 
        snprintf(buffer, sizeof(buffer), "[#%d %s]    %s: %s\n", chat->id, chat->time, chat->userName, chat->message);
        write(client, buffer, strlen(buffer));

        // reaction
        for (uint32_t j = 0; j < chat->reaction_count; j++) {
            struct Reaction* reaction = &chat->reactions[j];
            snprintf(buffer, sizeof(buffer), "                          (%s)  %s\n", reaction->username, reaction->message);
            write(client, buffer, strlen(buffer));
        }
    }

}


void handle_post(char* path, int client) {

    // extract query parameter from path
    char* user_param = parse_user(path);
    char* message_param = parse_message(path);

    if (!user_param || !message_param) {
        // if there is no user or message then output error
        const char* error_msg = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nMissing user or message parameter\n";
        write(client, error_msg, strlen(error_msg));
        return;
    }

    if (add_chat(user_param, message_param)) {
        // if chat sending is succesful
        respond_with_chats(client);
    } else {
        // if chat sending is unsuccesful
        const char* error_msg = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nFailed to add chat\n";
        write(client, error_msg, strlen(error_msg));
    }
}

void handle_reaction(char* path, int client) {
    // 쿼리 파라미터 추출
    char* user_param = parse_user(path);
    char* message_param = parse_message(path);
    char* id_param = parse_id(path);

    if (!user_param || !message_param || !id_param) {
        // 필수 파라미터가 없을 경우 에러 메시지 전송
        const char* error_msg = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nMissing user, message, or id parameter\n";
        write(client, error_msg, strlen(error_msg));
        
        return;
    }

    // 반응 추가
    if (add_reaction(user_param, message_param, id_param)) {
        // 성공적으로 반응이 추가된 경우 응답 전송
        respond_with_chats(client);
    } else {
        // 실패 시 오류 메시지 전송
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
    user_start += strlen("user=");  // "user=" 이후로 이동

    // putting in to the user array
    int i = 0;
    while (user_start[i] != '\0' && user_start[i] != '&' && i < MAX_USERNAME_LENGTH) {
        user[i] = user_start[i];
        i += 1;
    }
    user[i] = '\0';  // put null at the end of the array
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

int main() {
    /*
    // 초기 상태 설정
    current_chat_count = 0;

    // 테스트 1: 채팅 추가
    assert(add_chat("joe", "Hello, world!") == 1);
    assert(current_chat_count == 1);
    assert(strcmp(chats[0].userName, "joe") == 0);
    assert(strcmp(chats[0].message, "Hello, world!") == 0);

    // 테스트 2: printf로 결과 확인
    printf("Chat ID: %d\n", chats[0].id);
    printf("Chat User: %s\n", chats[0].userName);
    printf("Chat Message: %s\n", chats[0].message);
    printf("Chat Timestamp: %s\n", chats[0].time);

    return 0;
    

   // 채팅 추가 테스트
    uint32_t chat1_id = add_chat("yash", "hi all! react with 👍🏻 if you think the lab is good to go, or 😬 if you think it needs more testing");
    uint32_t chat2_id = add_chat("yash", "OK we'll go with what joe said");
    uint32_t chat3_id = add_chat("Kevin", "Hi");

    assert(chat1_id == 1);
    assert(chat2_id == 2);
    assert(chat3_id == 3);

    // 반응 추가 테스트
    add_reaction("aaron", "👍🏻", "1");
    add_reaction("elena", "😬", "1");
    add_reaction("arunan", "😬", "1");
    add_reaction("janet", "😬", "1");
    add_reaction("travis", "😬", "1");
    add_reaction("joe", "🐿️", "3");

    // 채팅 및 반응 출력
 

    return 0;

*/
    return 0;
}
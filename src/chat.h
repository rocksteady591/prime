#pragma once
#include <string>
#include <connection_pool.h>

struct Message{
    int id;
    int chat_id;
    int sender_id;
    std::string text;
    std::string send_time;
};

struct ChatInfo{
    int id;
    int user1_id;
    int user2_id;
    std::string create_chat_time;
};

struct ContactInfo{
    std::string username;
    std::string login;
};

class ChatManager{
public:
    explicit ChatManager(ConnectionPool& pool);
    int CreateOrGetChat(int user1_id, int user2_id);
    std::vector<Message> GetMessages(int chat_id, int messages_count = 50, int offset = 0);
    void AddMessage(int sender_id, int chat_id, const std::string& message);
    std::vector<ChatInfo> GetChats(int user_id);
    std::vector<ContactInfo> GetContacts(int user_id);
private:
    ConnectionPool& pool_;
};

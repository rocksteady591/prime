#include "chat.h"
#include "connection_pool.h"
#include <string>
#include <utility>
#include <vector>

using pqxx::operator""_zv;

ChatManager::ChatManager(ConnectionPool& pool) : pool_(pool){}

int ChatManager::CreateOrGetChat(int user1_id, int user2_id){
    if(user1_id > user2_id){
        std::swap(user1_id, user2_id);
    }
    auto wrapper = pool_.GetConnection();
    pqxx::connection& conn = *wrapper;
    pqxx::work w(conn);
    int chat_id = 0;
    auto result = w.exec_params(
    R"(
        WITH inserted AS (
            INSERT INTO chats (user1_id, user2_id)
            VALUES ($1, $2)
            ON CONFLICT (user1_id, user2_id) DO NOTHING
            RETURNING id
        )
        SELECT id FROM inserted
        UNION ALL
        SELECT id FROM chats
        WHERE user1_id = $1 AND user2_id = $2
        LIMIT 1;
    )"_zv,
    user1_id, user2_id
    );
    chat_id = result[0][0].as<int>();
    w.commit();
    return chat_id;
}

std::vector<Message> ChatManager::GetMessages(int chat_id, int messages_count, int offset){
    std::vector<Message> messages;
    constexpr auto select_messages_query = "SELECT id, chat_id, sender_id, content, created_at FROM messages WHERE chat_id=$1 LIMIT $2 OFFSET $3;"_zv;
    auto wrapper = pool_.GetConnection();
    pqxx::connection& conn = *wrapper;
    pqxx::read_transaction r(conn);
    pqxx::result result = r.exec_params(select_messages_query, chat_id, messages_count, offset);
    messages.reserve(result.size());
    for(const auto& row : result){
        int id = row[0].as<int>();
        int chat_id = row[1].as<int>();
        int sender_id = row[2].as<int>();
        std::string content(std::move(row[3].as<std::string>()));
        std::string timestamp = row[4].as<std::string>();
        messages.emplace_back(id, chat_id, sender_id, content, timestamp);
    }
    return messages;
}

std::vector<ChatInfo> ChatManager::GetChats(int user_id){
    std::vector<ChatInfo> chats;
    constexpr auto query_select = "SELECT id, user1_id, user2_id, created_at FROM chats WHERE user1_id=$1 OR user2_id=$1 ;"_zv;
    auto wrapper = pool_.GetConnection();
    pqxx::connection& conn = *wrapper;
    pqxx::read_transaction r(conn);
    pqxx::result select_result = r.exec_params(query_select, user_id);
    chats.reserve(select_result.size());
    for(const auto& row : select_result){
        int id = row[0].as<int>();
        int user1_id = row[1].as<int>();
        int user2_id = row[2].as<int>();
        std::string timestamp = row[3].as<std::string>();
        chats.emplace_back(id, user1_id, user2_id, timestamp);
    }
    return chats;
}

void ChatManager::AddMessage(int sender_id, int chat_id, const std::string& message){
    auto wrapper = pool_.GetConnection();
    pqxx::connection& conn = *wrapper;
    pqxx::work w(conn);
    w.exec_params(
            "INSERT INTO messages (chat_id, sender_id, content) VALUES ($1, $2, $3);",
            chat_id, sender_id, message);
    w.commit();
}

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
    auto result = w.exec(
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
    pqxx::params{user1_id, user2_id}
    );
    chat_id = result[0][0].as<int>();
    w.commit();
    return chat_id;
}

std::vector<ContactInfo> ChatManager::GetContacts(int user_id){
    std::vector<ContactInfo> contacts;
    constexpr auto query = "SELECT u.username, u.login FROM users u JOIN contacts c ON c.contact_id = u.id WHERE c.user_id = $1;"_zv;
    auto wrapper = pool_.GetConnection();
    pqxx::read_transaction r(*wrapper);
    auto result = r.exec(query, pqxx::params{user_id});
    contacts.reserve(result.size());
    for(const auto& row : result){
        std::string username = row[0].as<std::string>();
        std::string login = row[1].as<std::string>();
        contacts.emplace_back(username, login);
    }
    return contacts;
}

std::vector<Message> ChatManager::GetMessages(int user_id, int chat_id, int limit, int offset){
    std::vector<Message> messages;
    auto wrapper = pool_.GetConnection();
    pqxx::read_transaction r(*wrapper);

    // Проверка доступа
    auto check = r.exec(
        "SELECT 1 FROM chats WHERE id = $1 AND (user1_id = $2 OR user2_id = $2)",
        pqxx::params{chat_id, user_id});
    if (check.empty()) throw std::runtime_error("Access denied");

    auto result = r.exec(
        "SELECT id, chat_id, sender_id, content, sent_at FROM messages WHERE chat_id=$1 ORDER BY sent_at DESC LIMIT $2 OFFSET $3",
        pqxx::params{chat_id, limit, offset});
    messages.reserve(result.size());
    for (const auto& row : result) {
        messages.emplace_back(
            row[0].as<int>(),
            row[1].as<int>(),
            row[2].as<int>(),
            row[3].as<std::string>(),
            row[4].as<std::string>()
        );
    }
    return messages;
}

std::vector<ChatInfo> ChatManager::GetChats(int user_id){
    std::vector<ChatInfo> chats;
    constexpr auto query_select = "SELECT id, user1_id, user2_id, created_at FROM chats WHERE user1_id=$1 OR user2_id=$1 ;"_zv;
    auto wrapper = pool_.GetConnection();
    pqxx::connection& conn = *wrapper;
    pqxx::read_transaction r(conn);
    pqxx::result select_result = r.exec(query_select, pqxx::params{user_id});
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
    w.exec(
            "INSERT INTO messages (chat_id, sender_id, content) VALUES ($1, $2, $3);",
             pqxx::params{chat_id, sender_id, message});
    w.commit();
}

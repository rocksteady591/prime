#pragma once
#include <memory>
#include <unordered_set>
#include <string>

class User : public std::enable_shared_from_this<User>{
public:
    User(const std::string& logn, const std::string& password);
    //void SendMessage(const std::string& message);
private:
    
    std::string login_;
    std::string password_;
    std::unordered_set<std::string> friends_;
};
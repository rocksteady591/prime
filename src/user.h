#pragma once
#include <string>
#include <unordered_map>
#include <mutex>

class User {
public:
	User(std::string login, std::string pass, std::size_t id);
	const std::string& GetToken()const noexcept;
private:
	std::string login_;
	std::string hash_pass_;
	std::size_t id_;
	std::string token_;
	std::string GenerateToken();
};

class Users {
public:
	void CreateUser(std::string login, std::string pass);
	User* FindUser(const std::string& token);
	std::size_t GetCounter()const noexcept;
private:
	//<token, User>
	std::unordered_map<std::string, User> users_;
	std::size_t counter_ = 0;
	std::mutex mutex_;
};
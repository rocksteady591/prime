#include "user.h"

User::User(const std::string& login, const std::string& password) 
: login_(std::move(login)), password_(std::move(password)){}

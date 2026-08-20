#pragma once
#include <condition_variable>
#include <string>
#include <mutex>
#include <vector>
#include <memory>
#include <pqxx/pqxx>

class ConnectionPool{
public:
    class ConnectionWrapper{
    public:
        ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, ConnectionPool* pool)
        : connection_(std::move(conn)), pool_(pool){}

        ConnectionWrapper(const ConnectionWrapper&) = delete;
        ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;
        ConnectionWrapper(ConnectionWrapper&&) = default;
        ConnectionWrapper& operator=(ConnectionWrapper&&) = default;

        pqxx::connection& operator*() const& noexcept{
            return *connection_;
        }

        pqxx::connection& operator*() const&& = delete;

        pqxx::connection* operator->() const& noexcept{
            return connection_.get();
        }

        ~ConnectionWrapper(){
            if(pool_ != nullptr){
                pool_->ReturnConnection(std::move(connection_));
            }
        }
    private:
        std::shared_ptr<pqxx::connection> connection_;
        ConnectionPool* pool_;
    };

    ConnectionPool(size_t connection_count, const std::string& connection){
        pool_.reserve(connection_count);
        for(size_t i = 0; i < connection_count; ++i){
            pool_.emplace_back(std::make_shared<pqxx::connection>(connection));
        }
    }

    ConnectionWrapper GetConnection(){
        std::unique_lock lock(mutex_);
        cond_var.wait(lock, [this]{
            return used_conenctions_ < pool_.size();
        });
        return {std::move(pool_[used_conenctions_++]), this};
    }

private:
    void ReturnConnection(std::shared_ptr<pqxx::connection>&& conn){
        {
            std::lock_guard lock(mutex_);
            assert(used_conenctions_ != 0);
            pool_[--used_conenctions_] = std::move(conn);
        }
        cond_var.notify_one();
    }
    size_t used_conenctions_ = 0;
    std::vector<std::shared_ptr<pqxx::connection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_var;
};

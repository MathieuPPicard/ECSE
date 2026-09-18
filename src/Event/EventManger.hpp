#pragma once
#include "../Type.hpp"
#include "../IdQueue.hpp"
#include <iostream>
#include <unordered_map>

namespace ECS
{
    template<typename Event>
    struct EventManager {
        using callback_t = std::function<void(const Event&)>;

        IdQueue<subId_t> subIdQueue;
        std::unordered_map<subId_t, callback_t> subscribers;

        EventManager() {}
        ~EventManager() {}
        
        subId_t subscribe(callback_t callback) 
        {
            subId_t newId = subIdQueue.getNextId();
            auto subscriber = subscribers.find(newId);
            if(subscriber != subscribers.end()){
                throw std::logic_error("Trying to subscribe with an already used subId_t");
            }
            subscribers.emplace(newId, std::move(callback));
            return newId;
        }

        void unsubscibe(subId_t id) 
        {
            auto subscriber = subscribers.find(id);
            if(subscriber == subscribers.end()){
                throw std::logic_error("Trying to unsubscribe with an unused subId_t.");
            }
            subIdQueue.removeId(id);
            subscribers.erase(id);
        }

        void publish(const Event& event)
        {
            for(const auto& [id,callback] : subscribers){
                callback(event);
            }
        }
    };
}
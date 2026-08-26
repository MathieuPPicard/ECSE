#pragma once
#include <iostream>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <typeindex>

namespace ECS
{
    typedef uint8_t index_t;

    struct IStorage {
        virtual ~IStorage() = default;
    };

    template<typename T>
    struct Storage : IStorage {
        std::vector<T> storage;

        std::vector<T>* get() { return storage; }
        T* at(index_t idx) { 
            if(idx >= 0 && idx < storage.size()){ 
                return &storage.at(idx); 
            }
            return nullptr;
        }
        index_t count() { return storage.size(); }
        void add(T data) { storage.push_back(std::move(data)); }
        void remove(index_t idx) { storage.erase(storage.begin() + idx); }
    };

    struct Archetype{
        std::unordered_map<std::type_index, std::unique_ptr<IStorage>> components;

        template<typename T>
        void add(T data){
            auto it = components.find(typeid(T));
            if(it == components.end()){
                auto storagePtr = std::make_unique<Storage<T>>();
                storagePtr->add(data);
                components.emplace(typeid(T), std::move(storagePtr));
            } else {
                auto* storagePtr = static_cast<Storage<T>*>(it->second.get());
                storagePtr->storage.push_back(std::move(data));
            }
        }

        template<typename T>
        Storage<T>& getAllData(){
            auto it = components.find(typeid(T));
            if(it != components.end()){
                return *static_cast<Storage<T>*>(it->second.get());
            }
            throw std::runtime_error("Invalid Component Type for this Archetype.");
        }

        template<typename T>
        T* getData(index_t idx){
            auto it = components.find(typeid(T));
            if(it != components.end()){
                return static_cast<Storage<T>*>(it->second.get())->at(idx);
            }
            throw std::runtime_error("Invalid Component Type for this Archetype.");          
        }

        template<typename T>
        void remove(index_t idx){
            getAllData<T>().remove(idx);           
        }
    };

    struct EntityManager {
        std::unordered_map<index_t,Archetype> archetypes;
    };
}
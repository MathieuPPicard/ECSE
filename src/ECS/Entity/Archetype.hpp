#pragma once
#include "Type.hpp"
#include <iostream>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <typeindex>

namespace ECS
{   
    struct IStorage {
        virtual ~IStorage() = default;

        virtual void moveElementTo(index_t idx, IStorage& destination) = 0;
        virtual std::unique_ptr<IStorage> moveElement(index_t idx) = 0;
        virtual void remove(index_t idx) = 0;
        virtual index_t count() = 0;
    };

    template<typename T>
    struct Storage : IStorage {
        std::vector<T> storage;

        void moveElementTo(index_t idx, IStorage& destination) override {
            auto& typedDestination = dynamic_cast<Storage<T>&>(destination);

            typedDestination.storage.emplace_back(
                std::move(storage.at(idx))
            );
        }
        
        std::unique_ptr<IStorage> moveElement(index_t idx) override{
            auto result = std::make_unique<Storage<T>>();
            result->storage.emplace_back(std::move(storage.at(idx)));
            return result;
        }

        void remove(index_t idx) override{
            if(idx != storage.size() - 1){
                storage[idx] = std::move(storage.back());
            }
            storage.pop_back();
        }
        
        index_t count() override { return storage.size(); }

        T* at(index_t idx) { 
            if(idx >= 0 && idx < storage.size()){ 
                return &storage.at(idx); 
            }
            return nullptr;
        }
        std::vector<T>* get() { return storage; }
        void add(T data) { storage.push_back(std::move(data)); }
    };

    struct Archetype{
        std::unordered_map<std::type_index, std::unique_ptr<IStorage>> components;
        std::vector<index_t> entities;
        index_t archetypeId;

        template <typename T>
        index_t addOne(T&& data) {
            using Component = std::decay_t<T>;

            auto it = components.find(typeid(Component));

            if (it == components.end()) {
                auto storage = std::make_unique<Storage<Component>>();
                storage->add(std::forward<T>(data));

                components.emplace(typeid(Component), std::move(storage));
                return 0;
            }

            auto* storage = static_cast<Storage<Component>*>(it->second.get());
            storage->storage.push_back(std::forward<T>(data));

            return storage->storage.size() - 1;
        }

        template <typename... Ts>
        index_t add(Ts&&... values) {
            index_t position = 0;
            bool hasPosition = false;

            auto addAndCheckPosition = [&](auto&& value) {
                const index_t newPosition =
                    addOne(std::forward<decltype(value)>(value));

                // All storages in an archetype must stay aligned.
                if (hasPosition && newPosition != position) {
                    throw std::logic_error("Archetype component storages are misaligned");
                }

                position = newPosition;
                hasPosition = true;
            };

            (addAndCheckPosition(std::forward<Ts>(values)), ...);
            return position;
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
        T& getData(index_t idx){
            auto it = components.find(typeid(T));
            if(it != components.end()){
                return static_cast<Storage<T>*>(it->second.get())->storage.at(idx);
            }
            throw std::runtime_error("Invalid Component Type for this Archetype.");          
        }
    
        template<typename T>
        bool has(){
            return components.contains(typeid(T));
        }

        template<typename Component>
        void set(index_t pos, Component&& value){
            getData<Component>(pos) = std::forward<Component>(value);
        }

        std::vector<std::type_index> getSignature(){
            std::vector<std::type_index> signature;
            int size = components.size();
            for (const auto& [key, value] : components) {
                signature.push_back(key);
            }
            std::sort(signature.begin(),signature.end());
            return signature;
        }
    
        index_t size(){
            // Get the size of any storage(component) as they all have the same size
            if(components.empty()){
                return 0;
            }
            return components.begin()->second->count();
        }

        void debugPrintStorageSizes() const {
            std::cout << "Archetype storage sizes:\n";

            for (const auto& [type, storage] : components) {
                std::cout << "  " << type.name()
                        << ": " << storage->count()
                        << '\n';
            }
        }
    };

}

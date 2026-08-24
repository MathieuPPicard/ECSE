#pragma once
#include <iostream>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <stdexcept>

namespace ECS
{
    typedef uint8_t index_t; 
    typedef index_t EntityId;
    typedef index_t ComponentRegisterId;
    typedef index_t ComponentId;
    typedef index_t ArchetypeId;
    typedef index_t StorageId;

    struct EntityManager;
    
    ////////////// COMPONENT SECTION //////////////
    struct IComponentStorage{
        virtual ~IComponentStorage() = default;

        virtual void* get(size_t index) = 0;
        virtual void addDefault() = 0;
        virtual size_t size() = 0;
    };

    template<typename T>
    struct ComponentStorage : IComponentStorage{
        std::vector<T> data;

        void* get(size_t index) override { return &data[index]; }
        void addDefault() override { data.emplace_back(); }
        size_t size() override { return data.size(); }
        template<typename... Args>
        //To have acces to add to cast into ComponentStorage<T>
        T& add(Args&&... args){
            data.emplace_back(std::forward<Args>(args)...);
            return data.back();
        }
    };

    struct ComponentRegister {
        ComponentRegisterId componentId;
        std::unique_ptr<IComponentStorage> (*createStorage)();
    };

    ////////////// ARCHETYPE MAP //////////////
    struct Archetype {
        ArchetypeId archetypeId;
        std::unordered_map<ComponentId, std::unique_ptr<IComponentStorage>> components;
        bool has(index_t componentId){ return components.contains(componentId); }
    };
    
    ////////////// Entity //////////////
    struct Entity{
        EntityId entityId;
        EntityManager* manager;
        std::vector<ComponentRegister> componentsToAdd;

        void add(index_t id);
    };

    struct EntityMetada{
        EntityId entityId;
        // To try with archetypes unorder_map, if not work transform archetypes to vector
        // Archetype* ptrArch;
        ArchetypeId archetpyeId;
        StorageId storageId;
    };

    ////////////// EntityManager //////////////
    struct EntityManager
    {
        index_t nextComponentId = 0;
        index_t nextEntityId = 0;
        index_t nextArchetypeId = 0;

        std::vector<ComponentRegister> componentRegistry;
        std::unordered_map<EntityId,EntityMetada> entitiesMetadata;
        std::unordered_map<ArchetypeId, Archetype> archetypes;

        // Register the wanted components
        void registerNewComponent(ComponentRegister newRegister);
        bool isComponentRegister(index_t id);
        ComponentRegister getComponentRegister(index_t id);

        // Creating entity
        void createEntity(Entity newEntity);
        void addEntityToArchetype(Entity& entity, index_t archetypeId);
        bool findingMatchingArchetype(std::vector<ComponentRegister>& compoRegistry, index_t* outIndex);
        index_t createNewArchetype(Entity& entity);
        
        template<typename T>    //Templating function need to stay in .hpp so that the importing project can use it
        void setValue(index_t entityId, index_t componentId ,T value){
            EntityMetada md = entitiesMetadata.at(entityId);
            if(archetype(md.archetpyeId).has(componentId)){
                ComponentStorage<T>* storage = dynamic_cast<ComponentStorage<T>*>(archetype(md.archetpyeId).components.at(componentId).get());
                T* data = static_cast<T*>(storage->get(md.storageId));
                *data = std::move(value);
            }
        }
        
        template<typename T>
        ECS::ComponentStorage<T>* storeComponent(index_t archetypeId, index_t componentId){
            if(archetype(archetypeId).has(componentId)){
                return dynamic_cast<ECS::ComponentStorage<T>*>(
                    archetype(archetypeId).components.at(componentId).get()
                );
            }
            throw std::runtime_error("Invalid component: " + std::to_string(componentId) + " for archetype: " + std::to_string(archetypeId)  + "\n");
        }

        Archetype& archetype(index_t id);
        index_t getNextArchetypeId() { return nextArchetypeId++; }
        index_t getNextEntityId() { return nextEntityId++; }
        index_t getNextComponentId(){ return nextComponentId++; }
    }; 
}
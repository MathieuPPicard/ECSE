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

        void* get(size_t index) override{
            return &data[index];
        }
        void addDefault() override{
            data.emplace_back();
        }
        size_t size() override{
            return data.size();
        }
        template<typename... Args>
        T& add(Args&&... args){   //To have acces to add, I need to cast into ComponentStorage<T>
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

        bool has(index_t componentId){
            return components.contains(componentId);
        }
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
        void registerNewComponent(ComponentRegister newRegister){
            if(!isComponentRegister(newRegister.componentId)){
                componentRegistry.push_back(std::move(newRegister));
            }
        }

        bool isComponentRegister(index_t id){
            for(auto registry : componentRegistry){
                if(registry.componentId == id){
                    return true;
                }
            }
            return false;
        }

        ComponentRegister getComponentRegister(index_t id){
            for(auto registry: componentRegistry){
                if(registry.componentId == id){
                    return registry;
                }
            }
            throw std::runtime_error("Invalid name of component in getComponentRegister() >> " + id);
        }

        // Creating entity
        void createEntity(Entity newEntity){
            index_t idArchetype = 0;
            // If combinaison of component isnt already an Archetype
            if(!findingMatchingArchetype(newEntity.componentsToAdd, &idArchetype)){
                idArchetype = createNewArchetype(newEntity);  // Also add to the componentStorage
            } 
            addEntityToArchetype(newEntity, idArchetype);

            // Metadata to be able to look for it
            EntityMetada newMetaData {
                .entityId = newEntity.entityId,
                .archetpyeId = idArchetype,
                .storageId = (index_t)(archetypes.at(newMetaData.archetpyeId).components.at(0)->size() - 1),
            };
            entitiesMetadata.insert({newEntity.entityId,std::move(newMetaData)});
        }

        void addEntityToArchetype(Entity& entity, index_t archetypeId){
            for(auto comp : entity.componentsToAdd){
                auto it = archetypes[archetypeId].components.find(comp.componentId);
                if(it != archetypes[archetypeId].components.end()){
                    it->second->addDefault();
                }
            }
        }

        bool findingMatchingArchetype(std::vector<ComponentRegister>& compoRegistry, index_t* outIndex){
            // Safety: ensure the output pointer is valid
            if (outIndex == nullptr) {
                return false;
            }
            // Iterate over every existing archetype
            for (size_t archIdx = 0; archIdx < archetypes.size(); ++archIdx) {
                // Fast fail: if the number of components differs, they can't match
                if (compoRegistry.size() != archetypes[archIdx].components.size()) {
                    continue;
                }
                bool allFound = true;
                // Check that every component in the registry exists in this archetype
                for (const auto& reg : compoRegistry) {
                    if (archetypes[archIdx].components.find(reg.componentId) ==
                        archetypes[archIdx].components.end()) {
                        allFound = false;
                        break; // No need to check the rest of the registry
                    }
                }
                // If every component matched, we found the archetype
                if (allFound) {
                    *outIndex = static_cast<index_t>(archIdx);
                    return true;
                }
            }
            // No archetype matched
            return false;

        }

        index_t createNewArchetype(Entity& entity){
            Archetype newArchetype;
            index_t newId = getNextArchetypeId();
            newArchetype.archetypeId = newId;
            for(auto component : componentRegistry){
                for(auto entityCompo : entity.componentsToAdd){
                    if(component.componentId == entityCompo.componentId){
                        newArchetype.components[component.componentId] = component.createStorage();        
                    }
                }
            }
            // Add it to the archetypes
            archetypes[newId] = std::move(newArchetype);
            return newId;
        }

        template<typename T>
        void setValue(index_t entityId, index_t componentId ,T value){
            EntityMetada md = entitiesMetadata.at(entityId);
            if(archetypes.at(md.archetpyeId).has(componentId)){
                ComponentStorage<T>* storage = dynamic_cast<ComponentStorage<T>*>(archetypes.at(md.archetpyeId).components.at(componentId).get());
                T* data = static_cast<T*>(storage->get(md.storageId));
                *data = std::move(value);
            }
        }

        Archetype& archetype(index_t id){
            if(id < nextArchetypeId){
                return archetypes.at(id);
            }
            throw std::runtime_error("Invalid archetype id: " + id);
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

        index_t getNextArchetypeId() {
            return nextArchetypeId++;
        }

        index_t getNextEntityId() {
            return nextEntityId++;
        }

        index_t getNextComponentId(){
            return nextComponentId++;
        }
    }; 
}
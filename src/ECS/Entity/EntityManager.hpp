#pragma once
#include "Archetype.hpp"
#include "Type.hpp"
#include "Entity.hpp"
#include "IdQueue.hpp"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <algorithm>
#include <typeindex>

namespace ECS
{
    struct EntityManager {
        IdQueue<index_t> entityIdQueue;
        IdQueue<index_t> archetypeIdQueue;
        std::unordered_map<index_t, Archetype> archetypes;
        std::unordered_map<index_t, Entity> entityMetadata;

        // Get all the archetype that contains the Component 
        template<typename T>
        std::vector<Archetype*> has(){
            std::vector<Archetype*> result;
            std::type_index signature = typeid(T);

            for(auto& [id,archetype] : archetypes){
                std::vector<std::type_index> currSignature = archetype.getSignature();

                const bool matches = std::find(
                    currSignature.begin(),currSignature.end(), 
                    signature
                ) != currSignature.end();

                if(matches){
                    result.push_back(&archetype);
                }
            }

            return result;
        }

        template<typename... T>
        std::vector<Archetype*> hasAll(){
            std::vector<Archetype*> result;
            std::vector<std::type_index> wanted = {typeid(T)...};

            for(auto& [id,archetype] : archetypes){
                std::vector<std::type_index> currSignature = archetype.getSignature();
                if(wanted.size() != currSignature.size()){
                    return result;
                }

                bool allMatch = true;
                for(const auto& component : wanted){
                    bool found = false;
                    for(const auto& existing : currSignature){
                        if(component == existing){
                            found = true;
                            break;
                        }
                    }
                    if(!found){
                        allMatch = false;
                        break;
                    }
                }
                if(allMatch){
                    result.push_back(&archetype);
                }
            }
            return result;
        }

        template<typename Component>
        void set(Entity entity, Component&& value){
            entity.archetypePtr->getData<Component>(entity.storageIdx) = std::forward<Component>(value);
        }

        template<typename... Components>
        index_t createEntity(Components... components) 
        {
            index_t storePos = 0;

            std::vector<std::type_index> newSignature {
                typeid(Components)...
            };
            Archetype* archPtr = archetypeExists(&newSignature);
            if(archPtr == nullptr){
                // Create the new Archetype
                Archetype newArchetype;
                storePos = newArchetype.add(std::forward<Components>(components)...);
                index_t nextArchId = getNextArchetypeId();
                newArchetype.archetypeId = nextArchId;
                archetypes[nextArchId] = std::move(newArchetype);
                archPtr = &archetypes.at(nextArchId);
            } else {
                // Add components to the already existing Archetype
                storePos = archPtr->add(std::forward<Components>(components)...);
            }

            // Create Entity(metadata) and add to entityMetadata
            index_t entityId = getNextEntityId();
            archPtr->entities.push_back(entityId);
            return addMetaData(entityId, archPtr, storePos);
        }

        void removeEntity(Entity& entity){
            Archetype* oldArchetype = entity.archetypePtr;
            const index_t oldStorPos = entity.storageIdx;

            for(auto& [id, component] : oldArchetype->components){
                component->remove(oldStorPos);
            }
            synchEntitiesMetaData(oldStorPos, *oldArchetype);
            removeMetaData(entity.entityId);
            if(oldArchetype->size() == 0){
                archetypeIdQueue.removeId(oldArchetype->archetypeId);
                archetypes.erase(oldArchetype->archetypeId);
            }
        }

        template<typename T>
        void addComponent(Entity& entity, T&& data){
            Archetype* oldArchetype = entity.archetypePtr;

            // Archetype/Entity already have this component
            if(oldArchetype->has<T>()){
                throw std::logic_error("Archetype/Entity already got component.");
            }

            // Adding to the signature of the entity
            std::vector<std::type_index> signature = oldArchetype->getSignature();
            std::type_index newCompoSignature = typeid(T);
            signature.push_back(newCompoSignature);

            moveEntityToArchetype(
                entity,
                std::move(signature),
                nullptr,
                [&data](Archetype& destination) {
                    destination.add<T>(std::move(data));
                }
            );
        }

        template<typename T>
        void removeComponent(Entity& entity){
            Archetype* oldArchetype = entity.archetypePtr;

            if(!oldArchetype->has<T>()){
                throw std::logic_error("Archetype/Entity doesn't have this component.");
            }

            std::vector<std::type_index> signature = oldArchetype->getSignature();
            std::type_index toRemove = typeid(T);
            for(size_t i = 0; i < signature.size(); i++){
                if(signature[i] == toRemove){
                    signature.erase(signature.begin() + i);
                }
            }

            if(signature.size() == 0){
                const index_t oldStorPos = entity.storageIdx;
                for(auto& [id, component] : oldArchetype->components){
                    component->remove(oldStorPos);
                }
                synchEntitiesMetaData(oldStorPos, *oldArchetype);
                removeMetaData(entity.entityId);
                if(oldArchetype->size() == 0){
                    archetypeIdQueue.removeId(oldArchetype->archetypeId);
                    archetypes.erase(oldArchetype->archetypeId);
                }
                return;
            }

            const std::type_index removedComponent = typeid(T);
            moveEntityToArchetype(
                entity,
                std::move(signature),
                &removedComponent,
                [](Archetype&) {}
            );

            if(oldArchetype->size() == 0){
                archetypeIdQueue.removeId(oldArchetype->archetypeId);
                archetypes.erase(oldArchetype->archetypeId);
            }
        }

        template<typename PopulateDestination>
        void moveEntityToArchetype(Entity& entity, std::vector<std::type_index> targetSignature,
            const std::type_index* skippedComponent, PopulateDestination&& populateDestination)
        {
            Archetype* oldArchetype = entity.archetypePtr;
            const index_t oldStoragePos = entity.storageIdx;

            Archetype* destination = archetypeExists(&targetSignature);
            index_t destinationPos = 0;

            if(destination == nullptr){
                Archetype newArchetype;

                for(auto& [type, storage] : oldArchetype->components){
                    if(skippedComponent != nullptr && type == *skippedComponent){
                        continue;
                    }
                    newArchetype.components.emplace(
                        type,
                        storage->moveElement(oldStoragePos)
                    );
                }

                const index_t archetypeId = getNextArchetypeId();
                newArchetype.archetypeId = archetypeId;
                archetypes[archetypeId] = std::move(newArchetype);
                destination = &archetypes.at(archetypeId);
            } else {
                // Before moving any data, all destination storages are aligned.
                // This is the row that the transitioning entity will occupy.
                destinationPos = destination->size();

                for(auto& [type, storage] : oldArchetype->components){
                    if(skippedComponent != nullptr && type == *skippedComponent){
                        continue;
                    }

                    auto destinationStorage = destination->components.find(type);
                    if(destinationStorage == destination->components.end()){
                        throw std::logic_error("Destination archetype is missing an existing component storage.");
                    }
                    storage->moveElementTo(oldStoragePos, *destinationStorage->second);
                }
            }

            std::forward<PopulateDestination>(populateDestination)(*destination);

            // Adding a component must extend every destination storage by one row.
            if(destination->size() != destinationPos + 1){
                throw std::logic_error("Destination archetype component storages are misaligned");
            }

            destination->entities.push_back(entity.entityId);
            entity.archetypePtr = destination;
            entity.storageIdx = destinationPos;

            for(auto& [type, storage] : oldArchetype->components){
                storage->remove(oldStoragePos);
            }
            synchEntitiesMetaData(oldStoragePos, *oldArchetype);
        }

        void synchEntitiesMetaData(index_t removePos, Archetype& oldArchetype){
            const index_t last = oldArchetype.entities.size() - 1;
            const index_t movedEntityId = oldArchetype.entities[last];

            if(removePos != last){
                oldArchetype.entities.at(removePos) = movedEntityId;
                entityMetadata[movedEntityId].storageIdx = removePos;
            }

            oldArchetype.entities.pop_back();
        }

        ECS::Entity* entity(index_t entityIdx){
            return &entityMetadata.at(entityIdx);
        }

        Archetype* archetypeExists(std::vector<std::type_index>* newSign){
            // Sort the unordered signature
            std::sort(newSign->begin(),newSign->end());
            
            // For each already exisitng archetype
            for(auto& [id,archetype] : archetypes){
                bool allMatched = true;
                // Not same amount of component
                if(newSign->size() != archetype.components.size()){
                    continue;
                }
                std::vector<std::type_index> archSignature = archetype.getSignature();
                if(*newSign == archSignature){
                    return &archetype;
                }
            }
            return nullptr;
        }

        index_t addMetaData(index_t entityId, Archetype* archPtr, index_t storagePos){
            entityMetadata[entityId] = std::move(Entity{
                .entityId = entityId,
                .archetypePtr= archPtr,
                .storageIdx = storagePos
            });
            return entityId;
        }

        void removeMetaData(index_t entityId){
            entityIdQueue.removeId(entityId);
            entityMetadata.erase(entityId);
        }

        index_t getNextEntityId() { return entityIdQueue.getNextId(); }
        index_t getNextArchetypeId() { return archetypeIdQueue.getNextId(); }
    };
}

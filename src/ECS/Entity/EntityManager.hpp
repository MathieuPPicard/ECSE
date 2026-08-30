#pragma once
#include "Archetype.hpp"
#include "Type.hpp"
#include "Entity.hpp"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <typeindex>

namespace ECS
{

    struct EntityManager {
        index_t nextEntityId = 0;
        index_t nextArchetypeId = 0;
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
                archetypes[nextArchId] = std::move(newArchetype);
                archPtr = &archetypes.at(nextArchId);
            } else {
                // Add components to the already existing Archetype
                storePos = archPtr->add(std::forward<Components>(components)...);
            }

            // Create Entity(metadata) and add to entityMetadata
            return addMetaData(archPtr, storePos);
        }

        template<typename Component>
        void set(Entity entity, Component&& value){
            entity.archetypePtr->getData<Component>(entity.storageIdx) = std::forward<Component>(value);
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

            index_t newPos = 0;
            Archetype* newArchetypePtr = archetypeExists(&signature);
            // Verify if the new signature already exist or not
            if(newArchetypePtr != nullptr){
                // Push the data in the already existing storage for each component
                for(auto& [id,component] : oldArchetype->components){
                    auto destinationIt = newArchetypePtr->components.find(id);
                    if (destinationIt == newArchetypePtr->components.end()) {
                        throw std::logic_error("Destination archetype is missing an existing component storage.");
                    }
                    component->moveElementTo(
                        entity.storageIdx,
                        *destinationIt->second
                    );
                }
                newPos = newArchetypePtr->add<T>(std::move(data));
            } else {
                Archetype newArchetype;

                for(auto& [id, component] : oldArchetype->components){
                    newArchetype.components.emplace(
                        id,
                        component->moveElement(entity.storageIdx)
                    );
                }
                // Add the new component
                newPos = newArchetype.add<T>(std::move(data));


                // Add the new archetype into archetypes
                index_t nextArchId = getNextArchetypeId();
                archetypes[nextArchId] = std::move(newArchetype);
                newArchetypePtr = &archetypes.at(nextArchId);
            }

            index_t oldStorPos = entity.storageIdx;
            // Modify metadata information with new information
            entity.archetypePtr = newArchetypePtr;
            entity.storageIdx = newPos;

           for(auto& [id, component] : oldArchetype->components){
                component->remove(oldStorPos);
           }
        }

        template<typename T>
        void removeComponent(Entity entity){
            //TODO
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

        index_t addMetaData(Archetype* archPtr, index_t storagePos){

            index_t entityId = getNextEntityId();
            entityMetadata[entityId] = std::move(Entity{
                .entityId = entityId,
                .archetypePtr= archPtr,
                .storageIdx = storagePos
            });
            return entityId;
        }

        index_t getNextEntityId() { return nextEntityId++; }
        index_t getNextArchetypeId() { return nextArchetypeId++; }
    };
}
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
        std::vector<Entity> entityMetadata;

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
        void createEntity(Components... components) 
        {
            std::vector<std::type_index> newSignature {
                typeid(Components)...
            };
            Archetype* archPtr = archetypeExists(newSignature);
            if(archPtr != nullptr){
                // Create the new Archetype
                Archetype newArchetype;
                newArchetype.add<Components...>(components...);
                index_t nextArchId = getNextArchetypeId();
                archetypes[nextArchId] = std::move(newArchetype);
                archPtr = &archetypes.at(nextArchId);

            } else {
                // Add components to the already existing Archetype
                archPtr->add<Components...>(components...);
            }

            index_t storePos = newSignature.size() - 1;

            // Create Entity(metadata) and add to entityMetadata
            // not even sure if necessary
            addMetaData(archPtr, storePos);
        }

        Archetype* archetypeExists(std::vector<std::type_index> signature){
            // Iterate through each Archetype
            // sort signature param
            // archetype.getSignature() -> sorted list of component
            // compare getSignature() and signature from param
            // If not same size return nullptr
            // else if one is not equal return nullptr
            // else if all have an equal return the ptr to the archetype
            // TODO

            return nullptr;
        }

        // not even sure if necessary
        void addMetaData(Archetype* archPtr, index_t storagePos){
            // Create Entity struct with info given from createEntity
            // std::move into entityMetadata vector
            // TODO
        }

        index_t getNextEntityId() { return nextEntityId++; }
        index_t getNextArchetypeId() { return nextArchetypeId++; }
    };
}
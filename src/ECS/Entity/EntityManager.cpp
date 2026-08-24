#include "EntityManager.hpp"
using namespace ECS;

void Entity::add(index_t id)
{
    if(manager->isComponentRegister(id)){
        componentsToAdd.push_back(manager->getComponentRegister(id));
    }
}

void EntityManager::registerNewComponent(ComponentRegister newRegister){
    if(!isComponentRegister(newRegister.componentId)){
        componentRegistry.push_back(std::move(newRegister));
    }
}

bool EntityManager::isComponentRegister(index_t id){
    for(auto registry : componentRegistry){
        if(registry.componentId == id){
            return true;
        }
    }
    return false;
}

ComponentRegister EntityManager::getComponentRegister(index_t id){
    for(auto registry: componentRegistry){
        if(registry.componentId == id){
            return registry;
        }
    }
    throw std::runtime_error("Invalid name of component in getComponentRegister() >> " + id);
}

void EntityManager::createEntity(Entity newEntity){
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
        .storageId = (index_t)(archetype(newMetaData.archetpyeId).components.at(0)->size() - 1),
    };
    entitiesMetadata.insert({newEntity.entityId,std::move(newMetaData)});
}

void EntityManager::addEntityToArchetype(Entity& entity, index_t archetypeId){
    for(auto comp : entity.componentsToAdd){
        auto it = archetypes[archetypeId].components.find(comp.componentId);
        if(it != archetypes[archetypeId].components.end()){
            it->second->addDefault();
        }
    }
}

bool EntityManager::findingMatchingArchetype(std::vector<ComponentRegister>& compoRegistry, index_t* outIndex){
    // Safety: ensure the output pointer is valid
    if (outIndex == nullptr) {
        return false;
    }
    // Iterate over every existing archetype
    for (size_t archIdx = 0; archIdx < archetypes.size(); ++archIdx) {
        // Fast fail: if the number of components differs, they can't match
        if (compoRegistry.size() != archetype(archIdx).components.size()) {
            continue;
        }
        bool allFound = true;
        // Check that every component in the registry exists in this archetype
        for (const auto& reg : compoRegistry) {
            if (archetype(archIdx).components.find(reg.componentId) ==
                archetype(archIdx).components.end()) {
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

index_t EntityManager::createNewArchetype(Entity& entity){
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

Archetype& EntityManager::archetype(index_t id){
    if(id < nextArchetypeId){
        return archetypes.at(id);
    }
    throw std::runtime_error("Invalid archetype id: " + id);
}
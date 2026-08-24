#include "EntityManager.hpp"
using namespace ECS;

void Entity::add(index_t id)
{
    if(manager->isComponentRegister(id)){
        componentsToAdd.push_back(manager->getComponentRegister(id));
    }
}
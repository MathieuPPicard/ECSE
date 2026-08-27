#pragma once
#include "Archetype.hpp"
#include "Type.hpp"

namespace ECS
{
    struct Entity{
        index_t entityId = 0;
        Archetype* archetypePtr = nullptr;
        index_t storageIdx = 0;
    };
}
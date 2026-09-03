#pragma once
#include <type_traits>
#include <queue>

template<typename idType>
struct IdQueue{
    idType counter = 0;
    std::queue<idType> queue;

    idType getNextId(){
        if(!queue.empty()){
            idType first = queue.front();
            queue.pop();
            return first;
        }
        return counter++;
    }

    void removeId(idType id){
        queue.push(id);
    }
};
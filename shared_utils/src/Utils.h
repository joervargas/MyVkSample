#pragma once

#include <vector>


template<typename T>
inline void mergeVectors(std::vector<T>& v1, const std::vector<T>& v2)
{
    v1.insert( v1.end(), v2.begin(), v2.end() );
}
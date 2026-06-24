#pragma once

#include "Export.hpp"

/*
 * Deletes copy constructor and copy assignment operator
 * Implicitly deletes move operator as well!
 * Does not have virtual functions therefore has no V-table overhead
 */
class ENGINE_API NotCopyable
{
public:
    NotCopyable() = default;
    NotCopyable(const NotCopyable&) = delete;
    void operator=(const NotCopyable&) = delete;
};
#pragma once

#include <string_view>
#include "Macros.hpp"
#include "Hash.hpp"

/*
 * Adapted from Entt's type name
 */
template<typename Type>
NO_DISCARD static constexpr std::string_view TypeNameWithQualifiers() {
#if DS_COMPILER_MSVC
    std::string_view prettyFunction(__FUNCSIG__);
    std::string_view sub = prettyFunction.substr(prettyFunction.find_first_of('T'));
    uint64 offset = sub.find_first_of(' ') + 1;
    return sub.substr(offset, sub.find_first_of('>') - offset);
#elif DS_COMPILER_CLANG
    std::string_view prettyFunction(static_cast<const char* >(__PRETTY_FUNCTION__));
    uint64 first = prettyFunction.find_first_not_of(' ', prettyFunction.find_first_of('=') + 1);
    return prettyFunction.substr(first, prettyFunction.find_last_of(']') - first);
#elif DS_COMPILER_GCC
    std::string_view prettyFunction(static_cast<const char* >(__PRETTY_FUNCTION__));
    uint64 first = prettyFunction.find_first_not_of(' ', prettyFunction.find_first_of('=') + 1);
    return prettyFunction.substr(first, prettyFunction.find_last_of(';') - first);
#else
    ASSERT("Unsupported compiler: Cannot compute the type name!")
    return std::string_view("");
#endif
}

template<typename Type>
NO_DISCARD static constexpr std::string_view TypeName() {
    return TypeNameWithQualifiers<std::remove_cv_t<std::remove_reference_t<Type>>>();
}

template<typename Type>
NO_DISCARD static constexpr uint32 TypeHash() {
    return FNV1aHash(TypeName<Type>().data(), TypeName<Type>().size());
}





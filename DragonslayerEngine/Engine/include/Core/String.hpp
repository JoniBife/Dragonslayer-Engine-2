#pragma once

#include <cmath>
#include <string_view>

#include "Assert.hpp"
#include "Types.hpp"
#include "Hash.hpp"

/**
 * Basic Fixed Sized String
 * - No heap allocations
 * - Constexpr support
 * TODO Define exactly how the size and the null terminator relate, is it included, is it not?
 * TODO Complete API
 */
template<uint32 MaxCharacters>
class InlineString {
    static_assert(MaxCharacters > 0, "InlineString must allow at least a null terminator");

private:
    char data[MaxCharacters] {};
    uint32 size = 0;

public:
    constexpr InlineString() = default;

    template<uint32 StringSize>
    constexpr InlineString(const InlineString<StringSize>& string) {
        static_assert(StringSize <= MaxCharacters, "Parameter String size must be <= MaxCharacters");
        Assign(string.CStr(), string.Size());
    }

    constexpr InlineString(const char* string) { Assign(std::string_view(string)); }

    constexpr InlineString(const std::string_view stringView) { Assign(stringView); }

    template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
    explicit constexpr InlineString(T value) { AssignInteger(value); }

    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    explicit InlineString(T value, uint32 precision = 6) { AssignFloat(value, precision); }

    // -----------------------------
    // Queries
    // -----------------------------

    NO_DISCARD constexpr bool Contains(const std::string_view substring) const {

        if (substring.empty()) // TODO Should we return true in this case?
            return true;

        if (substring.size() > size)
            return false;

        for (uint32 i = 0; i <= size - substring.size(); ++i) {
            uint32 j = 0;
            for (; j < substring.size(); ++j) {
                if (data[i + j] != substring[j])
                    break;
            }
            if (j == substring.size())
                return true;
        }
        return false;
    }

    NO_DISCARD constexpr bool StartsWith(const std::string_view prefix) const {
        if (prefix.size() > size)
            return false;

        for (uint32 i = 0; i < prefix.size(); ++i) {
            if (data[i] != prefix[i])
                return false;
        }
        return true;
    }

    NO_DISCARD constexpr bool EndsWith(const std::string_view suffix) const {
        if (suffix.size() > size)
            return false;

        const uint32 offset = size - static_cast<uint32>(suffix.size());
        for (uint32 i = 0; i < suffix.size(); ++i) {
            if (data[offset + i] != suffix[i])
                return false;
        }
        return true;
    }

    // -----------------------------
    // Operators
    // -----------------------------

    constexpr InlineString operator+(const char* rhs) const {
        InlineString result;
        result.Append(std::string_view(data, size));
        result.Append(rhs);
        return result;
    }

    constexpr InlineString operator+(const std::string_view rhs) const {
        InlineString result;
        result.Append(std::string_view(data, size));
        result.Append(rhs);
        return result;
    }

    constexpr InlineString operator+(const InlineString& rhs) const {
        InlineString result;
        result.Append(std::string_view(data, size));
        result.Append(std::string_view(rhs.CStr(), rhs.Size()));
        return result;
    }

    constexpr InlineString& operator+=(const char* rhs) {
        Append(rhs);
        return *this;
    }

    constexpr InlineString& operator+=(const std::string_view rhs) {
        Append(rhs);
        return *this;
    }

    constexpr InlineString& operator+=(const InlineString& rhs) {
        Append(std::string_view(rhs.CStr(), rhs.Size()));
        return *this;
    }

    constexpr InlineString& operator=(const char* rhs) {
        Assign(rhs);
        return *this;
    }

    constexpr InlineString& operator=(const std::string_view rhs) {
        Assign(rhs);
        return *this;
    }

    constexpr bool operator==(const char* rhs) const {
        if (size != std::string_view(rhs).size())
            return false;

        for (uint32 i = 0; i < size; ++i) {
            if (data[i] != rhs[i])
                return false;
        }
        return true;
    }

    constexpr bool operator==(const std::string_view rhs) const {
        if (size != rhs.size())
            return false;

        for (uint32 i = 0; i < size; ++i) {
            if (data[i] != rhs[i])
                return false;
        }
        return true;
    }

    constexpr bool operator!=(const std::string_view rhs) const {
        return !(*this == rhs);
    }

    constexpr bool operator==(const InlineString& other) const {
        if (size != other.size)
            return false;

        for (uint32 i = 0; i < size; ++i) {
            if (data[i] != other.data[i])
                return false;
        }
        return true;
    }

    constexpr bool operator!=(const InlineString& other) const {
        return !(*this == other);
    }

    // -----------------------------
    // Access
    // -----------------------------

    NO_DISCARD constexpr const char* CStr() const {
        return data;
    }

    NO_DISCARD char* Data() {
        return data;
    }

    NO_DISCARD constexpr uint32 Size() const {
        return size;
    }

    NO_DISCARD constexpr uint32 Capacity() const {
        return MaxCharacters - 1;
    }

    NO_DISCARD constexpr uint32 Hash() const {
        return FNV1aHash(data, size);
    }

private:

    // -----------------------------
    // Helpers
    // -----------------------------

    constexpr void Assign(const char* string, uint32 size) {

        this->size = size;
        for (uint32 i = 0; i < size; ++i) {
            data[i] = string[i];
        }
        data[size] = '\0';
    }

    constexpr void Assign(const std::string_view sv) {

        size = static_cast<uint32>(sv.size());
        for (uint32 i = 0; i < size; ++i) {
            data[i] = sv[i];
        }
        data[size] = '\0';
    }

    constexpr void Append(const std::string_view sv) {

        for (uint32 i = 0; i < sv.size(); ++i) {
            data[size + i] = sv[i];
        }
        size += static_cast<uint32>(sv.size());
        data[size] = '\0';
    }

    constexpr void AssignFloat(double value, uint32 precision = 6) {

        // TODO Refactor this function!

        char buffer[MaxCharacters];
        uint32 pos = 0;

        // Handle negative
        if (value < 0.0) {
            buffer[pos++] = '-';
            value = -value;
        }

        // Handle special cases manually
        if (std::isnan(value)) { // TODO Not constexpr
            buffer[pos++] = 'N'; buffer[pos++] = 'a'; buffer[pos++] = 'N';
            size = pos;
            for (uint32 i = 0; i < size; ++i) data[i] = buffer[i];
            data[size] = '\0';
            return;
        }

        if (std::isinf(value)) { // TODO Not constexpr
            buffer[pos++] = 'I'; buffer[pos++] = 'n'; buffer[pos++] = 'f';
            size = pos;
            for (uint32 i = 0; i < size; ++i) data[i] = buffer[i];
            data[size] = '\0';
            return;
        }

        // Integer and fractional parts
        uint64 intPart = static_cast<uint64>(value);
        double fracPart = value - static_cast<double>(intPart);

        // Convert integer part
        char intBuffer[MaxCharacters];
        uint32 intPos = 0;
        if (intPart == 0) intBuffer[intPos++] = '0';
        else {
            uint64 tmp = intPart;
            while (tmp != 0) {
                intBuffer[intPos++] = '0' + tmp % 10;
                tmp /= 10;
            }
        }

        for (int i = intPos - 1; i >= 0; --i) buffer[pos++] = intBuffer[i];

        buffer[pos++] = '.';

        // Convert fractional part
        for (uint32 i = 0; i < precision; ++i) {
            fracPart *= 10.0;
            uint32 digit = static_cast<uint32>(fracPart);
            buffer[pos++] = '0' + digit;
            fracPart -= digit;
        }

        // Copy into data
        size = pos;
        for (uint32 i = 0; i < size; ++i) data[i] = buffer[i];
        data[size] = '\0';
    }

    template<typename T>
    constexpr void AssignInteger(T value) {

        // TODO Refactor this function!

        static_assert(std::is_integral_v<T>, "Integral type required");

        char buffer[MaxCharacters];
        uint32 pos = 0;

        bool isNegative = false;
        using Unsigned = std::make_unsigned_t<T>;
        Unsigned uvalue;

        if constexpr (std::is_signed_v<T>) {
            if (value < 0) { isNegative = true; uvalue = static_cast<Unsigned>(-value); }
            else uvalue = static_cast<Unsigned>(value);
        } else {
            uvalue = value;
        }

        if (uvalue == 0) buffer[pos++] = '0';
        else {
            while (uvalue != 0) { buffer[pos++] = '0' + static_cast<char>(uvalue % 10); uvalue /= 10; }
        }

        if (isNegative) buffer[pos++] = '-';

        // Reverse digits into data
        size = pos;
        for (uint32 i = 0; i < pos; ++i) data[i] = buffer[pos - 1 - i];
        data[size] = '\0';
    }
};

// -----------------------------
// Free operators to support lhs = const char*
// -----------------------------
template<uint32 MaxCharacters>
FORCE_INLINE constexpr InlineString<MaxCharacters> operator+(const char* lhs, const InlineString<MaxCharacters>& rhs) {
    return InlineString<MaxCharacters>(lhs) + rhs;
}

template<uint32 MaxCharacters>
FORCE_INLINE constexpr InlineString<MaxCharacters> operator+(const std::string_view lhs, const InlineString<MaxCharacters>& rhs) {
    return InlineString<MaxCharacters>(lhs) + rhs;
}

// For support in HashMap
template<uint32 N>
struct std::hash<InlineString<N>> {
    size_t operator()(const InlineString<N>& key) const noexcept {
        return key.Hash();
    }
};

constexpr uint32 NAME_STRING_LENGTH = 64;
using NameString = InlineString<NAME_STRING_LENGTH>;

// Note: It's the same size in both
#if DS_PLATFORM_WINDOWS
using PathString = InlineString<260>;
#elif DS_PLATFORM_LINUX
using PathString = InlineString<260>;
#endif



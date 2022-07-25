/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-07-24
 */


#include "compiler/type.hpp"

#include <ranges>

namespace cyn {

    std::optional<u32> ArrayType::size() const
    {
        if (element == nullptr)
            return std::nullopt;

        auto size = element->size();
        if (!size) return std::nullopt;

        for (auto& dim: dimensions) {
            *size *= dim;
        }

        return size;
    }

    std::optional<u32> TupleType::size() const
    {
        if (members.empty()) return 0;

        u32 size = 0;
        for (auto& member: members) {
            size += *member->size();
        }

        return size;
    }

    std::optional<u32> StructType::size() const
    {
        if (fields.empty()) return 0;
        u32 size;

        for (auto& field: fields) {
            size += *field->type->size();
        }

        return size;
    }

    std::optional<u32> UnionType::size() const
    {
        if (types.empty()) return 0;
        u32 size = 0;
        for (auto &type: types)
            size = std::max(*type->size(), size);

        return size;
    }
}
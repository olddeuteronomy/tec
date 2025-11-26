
#pragma once

#include <unordered_map>

#include "tec/net/tec_net_data.hpp"


namespace tec {


class NdDump {
public:

protected:
    using TagsMap = std::unordered_map<NetData::Tag, const char*>;

    inline static TagsMap tag_map = {
        {NetData::Tags::kI8, "uint8"},
        {NetData::Tags::kI8 | NetData::Meta::kSigned, "sint8"},
        {NetData::Tags::kI16, "uint16"},
        {NetData::Tags::kI16 | NetData::Meta::kSigned, "sint16"},
        {NetData::Tags::kI32, "uint32"},
        {NetData::Tags::kI32 | NetData::Meta::kSigned, "sint32"},
        {NetData::Tags::kI64, "uint64"},
        {NetData::Tags::kI64 | NetData::Meta::kSigned, "uint64"},
        {NetData::Tags::kBool, "bool"},
        {NetData::Tags::kF32, "real32"},
        {NetData::Tags::kF64, "real64"},
        {NetData::Tags::kF128, "real128"},
        {NetData::Tags::kBytes, "bytes"},
        {NetData::Tags::kString, "string"},
        {NetData::Tags::kContainer, "container"},
        {NetData::Tags::kSerializable, "object"},
    };

};


} // namespace tec

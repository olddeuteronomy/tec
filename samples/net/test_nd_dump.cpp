
#pragma once

#include <unordered_map>

#include "tec/net/tec_nd_types.hpp"


namespace tec {


class NdDump {
public:

protected:
    using TagsMap = std::unordered_map<NdTypes::Tag, const char*>;

    inline static TagsMap tag_map = {
        {NdTypes::Tags::I8, "uint8"},
        {NdTypes::Tags::I8 | NdTypes::Meta::Signed, "sint8"},
        {NdTypes::Tags::I16, "uint16"},
        {NdTypes::Tags::I16 | NdTypes::Meta::Signed, "sint16"},
        {NdTypes::Tags::I32, "uint32"},
        {NdTypes::Tags::I32 | NdTypes::Meta::Signed, "sint32"},
        {NdTypes::Tags::I64, "uint64"},
        {NdTypes::Tags::I64 | NdTypes::Meta::Signed, "uint64"},
        {NdTypes::Tags::IBool, "bool"},
        {NdTypes::Tags::F32, "real32"},
        {NdTypes::Tags::F64, "real64"},
        {NdTypes::Tags::F128, "real128"},
        {NdTypes::Tags::SByte, "bytes"},
        {NdTypes::Tags::SChar, "string"},
        {NdTypes::Tags::SWChar, "wstring"},
        {NdTypes::Tags::Container, "container"},
        {NdTypes::Tags::Map, "map"},
        {NdTypes::Tags::Object, "object"},
    };

};


} // namespace tec

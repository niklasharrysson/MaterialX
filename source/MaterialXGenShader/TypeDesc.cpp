//
// Copyright Contributors to the MaterialX Project
// SPDX-License-Identifier: Apache-2.0
//

#include <MaterialXGenShader/TypeDesc.h>

#include <MaterialXGenShader/ShaderGenerator.h>

#include <mutex>

MATERIALX_NAMESPACE_BEGIN

using DataBlockPtr = std::shared_ptr<TypeDesc::DataBlock>;
using TypeDescMap = std::unordered_map<string, TypeDesc>;

class TypeDescRegistryImpl
{
public:
    void registerBuiltinType(TypeDesc type)
    {
        // Updating the builtin type registry does not requires thread syncronization
        // since they are created statically during application launch.
        _builtinTypes.push_back(type);
        _builtinTypesByName[type.getName()] = type;
    }

    void registerCustomType(const string& name, uint8_t basetype, uint8_t semantic, uint16_t size, StructMemberDescVecPtr members)
    {
        // Allocate a data block and use to initialize a new type description.
        DataBlockPtr data = std::make_shared<TypeDesc::DataBlock>(name, members);
        const TypeDesc type(name, basetype, semantic, size, data.get());

        // Updating the custom type registry requires thread syncronization
        // since these can be created dynamically during document loading.
        std::lock_guard<std::mutex> guard(_mutex);

        // TODO: How should we handle registration of a type that already exists?

        _dataBlocks.push_back(data);
        _customTypes.push_back(type);
        _customTypesByName[name] = type;
    }

    void clear()
    {
        // Clear the custom types
        std::lock_guard<std::mutex> guard(_mutex);
        _customTypes.clear();
        _customTypesByName.clear();
        _dataBlocks.clear();
    }

    TypeDesc get(const string& name)
    {
        // First, check the built-in types
        // No thread syncronization required
        auto it = _builtinTypesByName.find(name);
        if (it != _builtinTypesByName.end())
        {
            return it->second;
        }

        // Second, look through the custom types
        // Since they are allowed to be dynamically loaded/unloaded
        // we need thread syncronization here
        std::lock_guard<std::mutex> guard(_mutex);
        it = _customTypesByName.find(name);
        return (it != _customTypesByName.end() ? it->second : Type::NONE);
    }

    TypeDesc getBuiltinType(const string& name)
    {
        auto it = _builtinTypesByName.find(name);
        return (it != _builtinTypesByName.end() ? it->second : Type::NONE);
    }

    TypeDesc getCustomType(const string& name)
    {
        std::lock_guard<std::mutex> guard(_mutex);
        auto it = _customTypesByName.find(name);
        return (it != _customTypesByName.end() ? it->second : Type::NONE);
    }

    TypeDescVec _builtinTypes;
    TypeDescMap _builtinTypesByName;

    TypeDescVec _customTypes;
    TypeDescMap _customTypesByName;

    vector<DataBlockPtr> _dataBlocks;

    std::mutex _mutex;
};

namespace 
{
    static TypeDescRegistryImpl s_registryImpl;
}

void TypeDesc::registerBuiltinType(TypeDesc type)
{
    s_registryImpl.registerBuiltinType(type);
}

void TypeDesc::registerCustomType(const string& name, uint8_t basetype, uint8_t semantic, uint16_t size, StructMemberDescVecPtr members)
{
    s_registryImpl.registerCustomType(name, basetype, semantic, size, members);
}

void TypeDesc::clearCustomTypes()
{
    s_registryImpl.clear();
}

TypeDesc TypeDesc::get(const string& name)
{
    return s_registryImpl.get(name);
}

TypeDesc TypeDesc::getBuiltinType(const string& name)
{
    return s_registryImpl.getBuiltinType(name);
}

const TypeDescVec& TypeDesc::getBuiltinTypes()
{
    return s_registryImpl._builtinTypes;
}

TypeDesc TypeDesc::getCustomType(const string& name)
{
    return s_registryImpl.getCustomType(name);
}

const TypeDescVec& TypeDesc::getCustomTypes()
{
    return s_registryImpl._customTypes;
}

ValuePtr TypeDesc::createValueFromStrings(const string& value) const
{
    auto structMembers = getStructMembers();
    if (!isStruct() || !structMembers)
    {
        return Value::createValueFromStrings(value, getName());
    }

    // Value::createValueFromStrings() can only create a valid Value for a struct if it is passed
    // the optional TypeDef argument, otherwise it just returns a "string" typed Value.
    // So if this is a struct type we need to create a new AggregateValue.

    StringVec subValues = parseStructValueString(value);
    if (subValues.size() != structMembers->size())
    {
        std::stringstream ss;
        ss << "Wrong number of initializers - expect " << structMembers->size();
        throw ExceptionShaderGenError(ss.str());
    }

    AggregateValuePtr result = AggregateValue::createAggregateValue(getName());
    for (size_t i = 0; i < structMembers->size(); ++i)
    {
        result->appendValue(structMembers->at(i).getType().createValueFromStrings(subValues[i]));
    }

    return result;
}

namespace Type
{

///
/// Register type descriptors for standard types.
///
TYPEDESC_REGISTER_TYPE(NONE)
TYPEDESC_REGISTER_TYPE(BOOLEAN)
TYPEDESC_REGISTER_TYPE(INTEGER)
TYPEDESC_REGISTER_TYPE(INTEGERARRAY)
TYPEDESC_REGISTER_TYPE(FLOAT)
TYPEDESC_REGISTER_TYPE(FLOATARRAY)
TYPEDESC_REGISTER_TYPE(VECTOR2)
TYPEDESC_REGISTER_TYPE(VECTOR3)
TYPEDESC_REGISTER_TYPE(VECTOR4)
TYPEDESC_REGISTER_TYPE(COLOR3)
TYPEDESC_REGISTER_TYPE(COLOR4)
TYPEDESC_REGISTER_TYPE(MATRIX33)
TYPEDESC_REGISTER_TYPE(MATRIX44)
TYPEDESC_REGISTER_TYPE(STRING)
TYPEDESC_REGISTER_TYPE(FILENAME)
TYPEDESC_REGISTER_TYPE(BSDF)
TYPEDESC_REGISTER_TYPE(EDF)
TYPEDESC_REGISTER_TYPE(VDF)
TYPEDESC_REGISTER_TYPE(SURFACESHADER)
TYPEDESC_REGISTER_TYPE(VOLUMESHADER)
TYPEDESC_REGISTER_TYPE(DISPLACEMENTSHADER)
TYPEDESC_REGISTER_TYPE(LIGHTSHADER)
TYPEDESC_REGISTER_TYPE(MATERIAL)

} // namespace Type

MATERIALX_NAMESPACE_END

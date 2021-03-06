//
//  TypeSystem.cpp
//  lldb
//
//  Created by Ryan Brown on 3/29/15.
//
//

#include "lldb/Symbol/TypeSystem.h"

#include <set>

#include "lldb/Core/Error.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Symbol/CompilerType.h"

using namespace lldb_private;

TypeSystem::TypeSystem(LLVMCastKind kind) :
    m_kind (kind),
    m_sym_file (nullptr)
{
}

TypeSystem::~TypeSystem()
{
}

lldb::TypeSystemSP
TypeSystem::CreateInstance (lldb::LanguageType language, Module *module)
{
    uint32_t i = 0;
    TypeSystemCreateInstance create_callback;
    while ((create_callback = PluginManager::GetTypeSystemCreateCallbackAtIndex (i++)) != nullptr)
    {
        lldb::TypeSystemSP type_system_sp = create_callback(language, module, nullptr, nullptr);
        if (type_system_sp)
            return type_system_sp;
    }

    return lldb::TypeSystemSP();
}

lldb::TypeSystemSP
TypeSystem::CreateInstance (lldb::LanguageType language, Target *target, const char *compiler_options)
{
    uint32_t i = 0;
    TypeSystemCreateInstance create_callback;
    while ((create_callback = PluginManager::GetTypeSystemCreateCallbackAtIndex (i++)) != nullptr)
    {
        lldb::TypeSystemSP type_system_sp = create_callback(language, nullptr, target, compiler_options);
        if (type_system_sp)
            return type_system_sp;
    }

    return lldb::TypeSystemSP();
}

bool
TypeSystem::IsAnonymousType (lldb::opaque_compiler_type_t type)
{
    return false;
}

CompilerType
TypeSystem::GetLValueReferenceType (lldb::opaque_compiler_type_t type)
{
    return CompilerType();
}

CompilerType
TypeSystem::GetRValueReferenceType (lldb::opaque_compiler_type_t type)
{
    return CompilerType();
}

CompilerType
TypeSystem::AddConstModifier (lldb::opaque_compiler_type_t type)
{
    return CompilerType();
}

CompilerType
TypeSystem::AddVolatileModifier (lldb::opaque_compiler_type_t type)
{
    return CompilerType();
}

CompilerType
TypeSystem::AddRestrictModifier (lldb::opaque_compiler_type_t type)
{
    return CompilerType();
}

CompilerType
TypeSystem::CreateTypedef (lldb::opaque_compiler_type_t type, const char *name, const CompilerDeclContext &decl_ctx)
{
    return CompilerType();
}

CompilerType
TypeSystem::GetBuiltinTypeByName (const ConstString &name)
{
    return CompilerType();
}

CompilerType
TypeSystem::GetTypeForFormatters (void* type)
{
    return CompilerType(this, type);
}

LazyBool
TypeSystem::ShouldPrintAsOneLiner (void* type, ValueObject* valobj)
{
    return eLazyBoolCalculate;
}

bool
TypeSystem::IsMeaninglessWithoutDynamicResolution (void* type)
{
    return false;
}

Error
TypeSystem::IsCompatible ()
{
    // Assume a language is compatible. Override this virtual function
    // in your TypeSystem plug-in if version checking is desired.
    return Error();
}

ConstString
TypeSystem::GetDisplayTypeName (void *type)
{
    return GetTypeName(type);
}

ConstString
TypeSystem::GetTypeSymbolName (void *type)
{
    return GetTypeName(type);
}

ConstString
TypeSystem::GetMangledTypeName (void *type)
{
    return GetTypeName(type);
}

ConstString
TypeSystem::DeclGetMangledName (void *opaque_decl)
{
    return ConstString();
}

CompilerDeclContext
TypeSystem::DeclGetDeclContext (void *opaque_decl)
{
    return CompilerDeclContext();
}

CompilerType
TypeSystem::DeclGetFunctionReturnType(void *opaque_decl)
{
    return CompilerType();
}

size_t
TypeSystem::DeclGetFunctionNumArguments(void *opaque_decl)
{
    return 0;
}

CompilerType
TypeSystem::DeclGetFunctionArgumentType (void *opaque_decl, size_t arg_idx)
{
    return CompilerType();
}


std::vector<CompilerDecl>
TypeSystem::DeclContextFindDeclByName (void *opaque_decl_ctx, ConstString name)
{
    return std::vector<CompilerDecl>();
}


#pragma mark TypeSystemMap

TypeSystemMap::TypeSystemMap() :
    m_mutex (),
    m_map ()
{
}


TypeSystemMap::TypeSystemMap(const TypeSystemMap &rhs) :
    m_mutex (),
    m_map(rhs.m_map)
{
}

TypeSystemMap::~TypeSystemMap()
{
}

void
TypeSystemMap::operator =(const TypeSystemMap &rhs)
{
    m_map = rhs.m_map;
}

void
TypeSystemMap::Clear ()
{
    Mutex::Locker locker (m_mutex);
    m_map.clear();
}


void
TypeSystemMap::ForEach (std::function <bool(TypeSystem *)> const &callback)
{
    Mutex::Locker locker (m_mutex);
    // Use a std::set so we only call the callback once for each unique
    // TypeSystem instance
    std::set<TypeSystem *> visited;
    for (auto pair : m_map)
    {
        TypeSystem *type_system = pair.second.get();
        if (type_system && !visited.count(type_system))
        {
            visited.insert(type_system);
            if (callback (type_system) == false)
                break;
        }
    }
}

TypeSystem *
TypeSystemMap::GetTypeSystemForLanguage (lldb::LanguageType language, Module *module, bool can_create)
{
    Mutex::Locker locker (m_mutex);
    collection::iterator pos = m_map.find(language);
    if (pos != m_map.end())
        return pos->second.get();

    for (const auto &pair : m_map)
    {
        if (pair.second && pair.second->SupportsLanguage(language))
        {
            // Add a new mapping for "language" to point to an already existing
            // TypeSystem that supports this language
            m_map[language] = pair.second;
            return pair.second.get();
        }
    }

    if (!can_create)
        return nullptr;

    // Cache even if we get a shared pointer that contains null type system back
    lldb::TypeSystemSP type_system_sp = TypeSystem::CreateInstance (language, module);
    m_map[language] = type_system_sp;
    return type_system_sp.get();
}

TypeSystem *
TypeSystemMap::GetTypeSystemForLanguage (lldb::LanguageType language, Target *target, bool can_create, const char *compiler_options)
{
    Mutex::Locker locker (m_mutex);
    collection::iterator pos = m_map.find(language);
    if (pos != m_map.end())
        return pos->second.get();

    for (const auto &pair : m_map)
    {
        if (pair.second && pair.second->SupportsLanguage(language))
        {
            // Add a new mapping for "language" to point to an already existing
            // TypeSystem that supports this language
            m_map[language] = pair.second;
            return pair.second.get();
        }
    }

    if (!can_create)
        return nullptr;

    // Cache even if we get a shared pointer that contains null type system back
    lldb::TypeSystemSP type_system_sp = TypeSystem::CreateInstance (language, target, compiler_options);
    m_map[language] = type_system_sp;
    return type_system_sp.get();
}

void
TypeSystemMap::RemoveTypeSystemsForLanguage (lldb::LanguageType language)
{
    Mutex::Locker locker (m_mutex);
    collection::iterator pos = m_map.find(language);
    if (pos != m_map.end())
        m_map.erase(pos);
}


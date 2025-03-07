//------------------------------------------------------------------------------
// Entity.cpp
//------------------------------------------------------------------------------
// Copyright (c) 2021 John Hughes
// Created by John Hughes on 3/13/21.
//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include "ae/Entity.h"
#include "ae/Editor.h"

//------------------------------------------------------------------------------
// Registration
//------------------------------------------------------------------------------
AE_REGISTER_CLASS( ae, Component );

namespace ae {

//------------------------------------------------------------------------------
// ae::Component member functions
//------------------------------------------------------------------------------
const char* Component::GetEntityName() const
{
	return m_reg->GetNameByEntity( m_entity );
}

Component& Component::GetComponent( const char* typeName )
{
	AE_ASSERT( m_reg );
	return m_reg->GetComponent( m_entity, typeName );
}

Component* Component::TryGetComponent( const char* typeName )
{
	AE_ASSERT( m_reg );
	return m_reg->TryGetComponent( m_entity, typeName );
}

const Component& Component::GetComponent( const char* typeName ) const
{
	AE_ASSERT( m_reg );
	return m_reg->GetComponent( m_entity, typeName );
}

const Component* Component::TryGetComponent( const char* typeName ) const
{
	AE_ASSERT( m_reg );
	return m_reg->TryGetComponent( m_entity, typeName );
}

//------------------------------------------------------------------------------
// Registry member functions
//------------------------------------------------------------------------------
Registry::Registry( const ae::Tag& tag ) :
	m_tag( tag ),
	m_entityNames( tag ),
	m_components( tag )
{}

void Registry::SetOnCreateFn( void* userData, void(*fn)(void*, Component*) )
{
	m_onCreateFn = fn;
	m_onCreateUserData = userData;
}

Entity Registry::CreateEntity( const char* name )
{
	AE_ASSERT_MSG( !m_destroying, "Cannot create entity while destroying" );
	m_lastEntity++;
	Entity entity = m_lastEntity;
	
	if ( name && name[ 0 ] )
	{
		AE_ASSERT_MSG( !m_entityNames.TryGet( name ), "Entity with name '#' already exists", name );
		m_entityNames.Set( name, entity );
	}
	
	return entity;
}

Entity Registry::CreateEntity( Entity entity, const char* name )
{
	AE_ASSERT_MSG( !m_destroying, "Cannot create entity while destroying" );
	for ( uint32_t i = 0; i < m_components.Length(); i++ )
	{
		AE_ASSERT( !m_components.GetValue( i ).TryGet( entity ) );
	}

	m_lastEntity = ae::Max( m_lastEntity, entity );
	
	if ( name && name[ 0 ] )
	{
		AE_ASSERT_MSG( !m_entityNames.TryGet( name ), "Entity with name '#' already exists", name );
		m_entityNames.Set( name, entity );
	}
	
	return entity;
}

Component* Registry::AddComponent( Entity entity, const char* typeName )
{
	return AddComponent( entity, ae::GetTypeByName( typeName ) );
}

Component* Registry::AddComponent( Entity entity, const ae::Type* type )
{
	AE_ASSERT_MSG( !m_destroying, "Cannot add component while destroying" );
	if ( !type )
	{
		return nullptr;
	}
	AE_ASSERT_MSG( type->IsType< ae::Component >(), "Type '#' does not inherit from ae::Component", type->GetName() );
	AE_ASSERT_MSG( !type->IsAbstract(), "Type '#' is abstract", type->GetName() );
	AE_ASSERT_MSG( type->IsDefaultConstructible(), "Type '#' is not default constructible", type->GetName() );
	
	ae::Object* object = (ae::Object*)ae::Allocate( m_tag, type->GetSize(), type->GetAlignment() );
	type->New( object );
	
	Component* component = ae::Cast< Component >( object );
	AE_ASSERT( component );
	component->m_entity = entity;
	component->m_reg = this;
	
	ae::Map< Entity, Component* >* components = m_components.TryGet( type->GetId() );
	if ( !components )
	{
		components = &m_components.Set( type->GetId(), m_tag );
	}
	components->Set( entity, component );
	
	if ( m_onCreateFn )
	{
		m_onCreateFn( m_onCreateUserData, component );
	}
	return component;
}

Component* Registry::TryGetComponent( const char* name, const char* typeName )
{
	return TryGetComponent( m_entityNames.Get( name, kInvalidEntity ), typeName );
}

Component* Registry::TryGetComponent( Entity entity, const char* typeName )
{
	if ( entity == kInvalidEntity || !typeName || !typeName[ 0 ] )
	{
		return nullptr;
	}
	
	const ae::Type* type = ae::GetTypeByName( typeName );
	if ( !type )
	{
		return nullptr;
	}
	
	if ( ae::Map< Entity, Component* >* components = m_components.TryGet( type->GetId() ) )
	{
		return components->Get( entity, nullptr );
	}
	
	return nullptr;
}

Component& Registry::GetComponent( Entity entity, const char* typeName )
{
	Component* component = TryGetComponent( entity, typeName );
	AE_ASSERT_MSG( component, "Entity '#' has no compoent '#'", entity );
	return *component;
}

Component& Registry::GetComponent( const char* name, const char* typeName )
{
	AE_ASSERT_MSG( name && name[ 0 ], "No name specified" );
	Component* component = TryGetComponent( name, typeName );
	AE_ASSERT_MSG( component, "Entity '#' has no compoent '#'", name );
	return *component;
}

Entity Registry::GetEntityByName( const char* name ) const
{
	return m_entityNames.Get( name, kInvalidEntity );
}

const char* Registry::GetNameByEntity( Entity entity ) const
{
	for ( uint32_t i = 0; i < m_entityNames.Length(); i++ )
	{
		if ( m_entityNames.GetValue( i ) == entity )
		{
			return m_entityNames.GetKey( i ).c_str();
		}
	}
	return "";
}

void Registry::SetEntityName( Entity entity, const char* name )
{
	if ( !entity )
	{
		return;
	}

	AE_ASSERT_MSG( !m_destroying, "Cannot set entity name while destroying" );
	
	for ( uint32_t i = 0; i < m_entityNames.Length(); i++ )
	{
		if ( m_entityNames.GetValue( i ) == entity )
		{
			m_entityNames.Remove( m_entityNames.GetKey( i ) );
		}
	}
	
	if ( name[ 0 ] )
	{
		m_entityNames.Set( name, entity );
	}
}

uint32_t Registry::GetTypeCount() const
{
	return m_components.Length();
}

const ae::Type* Registry::GetTypeByIndex( uint32_t index ) const
{
	return ae::GetTypeById( m_components.GetKey( index ) );
}

int32_t Registry::GetTypeIndexByType( const ae::Type* type ) const
{
	if ( !type )
	{
		return -1;
	}
	return m_components.GetIndex( type->GetId() );
}

uint32_t Registry::GetComponentCountByIndex( int32_t typeIndex ) const
{
	if ( typeIndex < 0 || typeIndex >= m_components.Length() )
	{
		return 0;
	}
	return m_components.GetValue( typeIndex ).Length();
}

const Component& Registry::GetComponentByIndex( int32_t typeIndex, uint32_t componentIndex ) const
{
	AE_ASSERT( typeIndex >= 0 && typeIndex < m_components.Length() );
	return *m_components.GetValue( typeIndex ).GetValue( componentIndex );
}

Component& Registry::GetComponentByIndex( int32_t typeIndex, uint32_t componentIndex )
{
	AE_ASSERT( typeIndex >= 0 && typeIndex < m_components.Length() );
	return *m_components.GetValue( typeIndex ).GetValue( componentIndex );
}

void Registry::Destroy( Entity entity )
{
	AE_ASSERT_MSG( !m_destroying, "Cannot destroy while already destroying" );
	m_destroying = true;

	const char* name = GetNameByEntity( entity );
	if ( name && name[ 0 ] )
	{
		m_entityNames.Remove( name );
	}
	// Get components each loop because m_components could grow at any iteration
	for ( uint32_t i = 0; i < m_components.Length(); i++ )
	{
		Component* c;
		ae::Map< Entity, Component* >* components = &m_components.GetValue( i );
		if ( components->Remove( entity, &c ) )
		{
			c->~Component();
			ae::Free( c );
		}
	}

	m_destroying = false;
}

void Registry::Clear()
{
	AE_ASSERT_MSG( !m_destroying, "Cannot destroy while already destroying" );
	m_destroying = true;

	// Get components each loop because m_components could grow at any iteration
	for ( uint32_t i = 0; i < m_components.Length(); i++ )
	{
		for ( uint32_t j = 0; j < m_components.GetValue( i ).Length(); j++ )
		{
			Component* c = m_components.GetValue( i ).GetValue( j );
			c->~Component();
			ae::Free( c );
		}
	}
	m_components.Clear();
	m_lastEntity = kInvalidEntity;
	m_entityNames.Clear();

	m_destroying = false;
}

bool Registry::Load( const ae::EditorLevel* level, CreateObjectFn fn )
{
	if ( !level )
	{
		return false;
	}

	Clear();

	uint32_t objectCount = level->objects.Length();
	// Create all components
	for ( uint32_t i = 0; i < objectCount; i++ )
	{
		const ae::EditorObject& levelObject = level->objects.GetValue( i );
		Entity entity = CreateEntity( levelObject.id, levelObject.name.c_str() );
		if ( fn )
		{
			fn( levelObject, entity, this );
		}
		for ( const EditorComponent& levelComponent : levelObject.components )
		{
			AddComponent( entity, levelComponent.type.c_str() );
		}
	}
	// Serialize all components (second phase to handle references)
	for ( uint32_t i = 0; i < objectCount; i++ )
	{
		const ae::EditorObject& levelObject = level->objects.GetValue( i );
		Entity entity = levelObject.id;
		for ( const EditorComponent& levelComponent : levelObject.components )
		{
			const char* typeName = levelComponent.type.c_str();
			const ae::Type* type = ae::GetTypeByName( typeName );
			const ae::Dict& props = levelComponent.members;
			Component* component = TryGetComponent( entity, typeName );
			if ( !component )
			{
				continue;
			}
			uint32_t varCount = type->GetVarCount( true );
			for ( uint32_t j = 0; j < varCount; j++ )
			{
				const ae::Var* var = type->GetVarByIndex( j, true );
				if ( var->IsArray() )
				{
					uint32_t length = props.GetInt( var->GetName(), 0 );
					length = var->SetArrayLength( component, length );
					for ( uint32_t arrIdx = 0; arrIdx < length; arrIdx++ )
					{
						ae::Str32 key = ae::Str32::Format( "#::#", var->GetName(), arrIdx );
						if ( const char* value = props.GetString( key.c_str(), nullptr ) )
						{
							var->SetObjectValueFromString( component, value, arrIdx );
						}
					}
				}
				else if ( const char* value = props.GetString( var->GetName(), nullptr ) )
				{
					var->SetObjectValueFromString( component, value );
				}
			}
		}
	}
	return true;
}

Component* Registry::m_AddComponent( Entity entity, const ae::Type* type )
{
	Component* component = (Component*)ae::Allocate( m_tag, type->GetSize(), type->GetAlignment() );
	type->New( component );
	component->m_entity = entity;
	component->m_reg = this;
	
	ae::Map< Entity, Component* >* components = m_components.TryGet( type->GetId() );
	if ( !components )
	{
		components = &m_components.Set( type->GetId(), m_tag );
	}
	components->Set( entity, component );
	
	if ( m_onCreateFn )
	{
		m_onCreateFn( m_onCreateUserData, component );
	}
	return component;
}

} // End ae namespace

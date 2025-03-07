//------------------------------------------------------------------------------
// MetaTest.cpp
//------------------------------------------------------------------------------
// Copyright (c) 2020 John Hughes
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include "MetaTest.h"
#include "catch2/catch.hpp"

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
TEST_CASE( "Can get base type by name", "[aeMeta]" )
{
	REQUIRE( ae::GetTypeByName( "ae::Object" ) );
}

TEST_CASE( "Can get base type with templates", "[aeMeta]" )
{
	REQUIRE( ae::GetType< ae::Object >() );
}

TEST_CASE( "Class registration", "[aeMeta]" )
{
	REQUIRE( ae::GetType< SomeClass >() );
	REQUIRE( ae::GetTypeByName( "SomeClass" ) );
	REQUIRE( ae::GetType< SomeClass >() == ae::GetTypeByName( "SomeClass" ) );
}

TEST_CASE( "Class properties", "[aeMeta]" )
{
	const ae::Type* type = ae::GetType< SomeClass >();
	REQUIRE( type );
	REQUIRE( type->GetPropertyCount() == 3 );
	
	REQUIRE( type->HasProperty( "someProp0" ) );
	REQUIRE( type->GetPropertyIndex( "someProp0" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyName( 0 ), "someProp0" ) == 0 );
	REQUIRE( type->GetPropertyValueCount( 0 ) == 0 );
	REQUIRE( type->GetPropertyValueCount( "someProp0" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( 0, 0 ), "" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( "someProp0", 0 ), "" ) == 0 );

	REQUIRE( type->HasProperty( "someProp1" ) );
	REQUIRE( type->GetPropertyIndex( "someProp1" ) == 1 );
	REQUIRE( strcmp( type->GetPropertyName( 1 ), "someProp1" ) == 0 );
	REQUIRE( type->GetPropertyValueCount( 1 ) == 2 );
	REQUIRE( type->GetPropertyValueCount( "someProp1" ) == 2 );
	REQUIRE( strcmp( type->GetPropertyValue( 1, 0 ), "v0" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( "someProp1", 0 ), "v0" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( 1, 1 ), "v1" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( "someProp1", 1 ), "v1" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( 1, 2 ), "" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( "someProp1", 2 ), "" ) == 0 );

	REQUIRE( type->HasProperty( "someProp2" ) );
	REQUIRE( type->GetPropertyIndex( "someProp2" ) == 2 );
	REQUIRE( strcmp( type->GetPropertyName( 2 ), "someProp2" ) == 0 );
	REQUIRE( type->GetPropertyValueCount( 2 ) == 3 );
	REQUIRE( type->GetPropertyValueCount( "someProp2" ) == 3 );
	REQUIRE( strcmp( type->GetPropertyValue( 2, 0 ), "v0" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( "someProp2", 0 ), "v0" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( 2, 1 ), "v1" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( "someProp2", 1 ), "v1" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( 2, 2 ), "v2" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( "someProp2", 2 ), "v2" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( 2, 3 ), "" ) == 0 );
	REQUIRE( strcmp( type->GetPropertyValue( "someProp2", 3 ), "" ) == 0 );

	REQUIRE( !type->HasProperty( "someProp3" ) );
	REQUIRE( type->GetPropertyIndex( "someProp3" ) == -1 );
	// REQUIRE_THROWS_AS( type->GetPropertyName( 3 ), std::runtime_error );
	// REQUIRE_THROWS( type->GetPropertyValueCount( 3 ) );
	// REQUIRE( type->GetPropertyValueCount( "someProp3" ) == 3 );
	// REQUIRE( strcmp( type->GetPropertyValue( 3, 0 ), "" ) == 0 );
	// REQUIRE( strcmp( type->GetPropertyValue( "someProp3", 0 ), "" ) == 0 );
}

TEST_CASE( "Class vars", "[aeMeta]" )
{
	const ae::Type* type = ae::GetType< SomeClass >();
	REQUIRE( type );
	REQUIRE( type->GetVarCount( false ) );

	REQUIRE( type->GetVarByName( "intMember", false ) );
	REQUIRE( type->GetVarByIndex( 0, false ) );
	REQUIRE( type->GetVarByName( "intMember", false ) == type->GetVarByIndex( 0, false ) );
	const ae::Var* intVar = type->GetVarByName( "intMember", false );
	REQUIRE( intVar->HasProperty( "intProp" ) );
	REQUIRE( intVar->GetPropertyIndex( "intProp" ) == 0 );
	REQUIRE( intVar->GetPropertyCount() == 1 );
	REQUIRE( strcmp( intVar->GetPropertyName( 0 ), "intProp" ) == 0 );
	REQUIRE( intVar->GetPropertyValueCount( 0 ) == 0 );
	REQUIRE( intVar->GetPropertyValueCount( "intProp" ) == 0 );

	REQUIRE( type->GetVarByName( "boolMember", false ) );
	REQUIRE( type->GetVarByIndex( 1, false ) );
	REQUIRE( type->GetVarByName( "boolMember", false ) == type->GetVarByIndex( 1, false ) );
	const ae::Var* boolVar = type->GetVarByName( "boolMember", false );
	REQUIRE( boolVar->HasProperty( "boolProp" ) );
	REQUIRE( boolVar->GetPropertyIndex( "boolProp" ) == 0 );
	REQUIRE( boolVar->GetPropertyCount() == 1 );
	REQUIRE( strcmp( boolVar->GetPropertyName( 0 ), "boolProp" ) == 0 );
	REQUIRE( boolVar->GetPropertyValueCount( 0 ) == 1 );
	REQUIRE( boolVar->GetPropertyValueCount( "boolProp" ) == 1 );
	REQUIRE( strcmp( boolVar->GetPropertyValue( 0, 0 ), "val" ) == 0 );
	REQUIRE( strcmp( boolVar->GetPropertyValue( "boolProp", 0 ), "val" ) == 0 );

	REQUIRE( type->GetVarByName( "enumTest", false ) );
	REQUIRE( type->GetVarByIndex( 2, false ) );
	REQUIRE( type->GetVarByName( "enumTest", false ) == type->GetVarByIndex( 2, false ) );
	const ae::Var* enumVar = type->GetVarByName( "enumTest", false );
	REQUIRE( enumVar->GetPropertyCount() == 2 );
	
	// prop0
	REQUIRE( enumVar->HasProperty( "prop0" ) );
	REQUIRE( enumVar->GetPropertyIndex( "prop0" ) == 0 );
	REQUIRE( strcmp( enumVar->GetPropertyName( 0 ), "prop0" ) == 0 );
	REQUIRE( enumVar->GetPropertyValueCount( 0 ) == 1 );
	REQUIRE( enumVar->GetPropertyValueCount( "prop0" ) == 1 );

	REQUIRE( strcmp( enumVar->GetPropertyValue( 0, 0 ), "val0" ) == 0 );
	REQUIRE( strcmp( enumVar->GetPropertyValue( "prop0", 0 ), "val0" ) == 0 );

	// prop1
	REQUIRE( enumVar->HasProperty( "prop1" ) );
	REQUIRE( enumVar->GetPropertyIndex( "prop1" ) == 1 );
	REQUIRE( strcmp( enumVar->GetPropertyName( 1 ), "prop1" ) == 0 );
	REQUIRE( enumVar->GetPropertyValueCount( 1 ) == 2 );
	REQUIRE( enumVar->GetPropertyValueCount( "prop1" ) == 2 );

	REQUIRE( strcmp( enumVar->GetPropertyValue( 1, 0 ), "val0" ) == 0 );
	REQUIRE( strcmp( enumVar->GetPropertyValue( "prop1", 0 ), "val0" ) == 0 );

	REQUIRE( strcmp( enumVar->GetPropertyValue( 1, 1 ), "val1" ) == 0 );
	REQUIRE( strcmp( enumVar->GetPropertyValue( "prop1", 1 ), "val1" ) == 0 );
}

//------------------------------------------------------------------------------
// NamespaceClass
//------------------------------------------------------------------------------
TEST_CASE( "Class registration in namespaces", "[aeMeta]" )
{
	REQUIRE( ae::GetType< Namespace0::Namespace1::NamespaceClass >() );
	REQUIRE( ae::GetTypeByName( "Namespace0::Namespace1::NamespaceClass" ) );
	REQUIRE( ae::GetType< Namespace0::Namespace1::NamespaceClass >() == ae::GetTypeByName( "Namespace0::Namespace1::NamespaceClass" ) );
}

//------------------------------------------------------------------------------
// PlayerState
//------------------------------------------------------------------------------
TEST_CASE( "enum registration", "[aeMeta]" )
{
	const ae::Enum* playerStateEnum = ae::GetEnum( "PlayerState" );
	REQUIRE( playerStateEnum->GetName() == ae::Str32( "PlayerState" ) );
	
	REQUIRE( playerStateEnum->TypeSize() == 2 );
	REQUIRE( playerStateEnum->TypeIsSigned() == false );
	
	REQUIRE( playerStateEnum->Length() == 3 );
	
	REQUIRE( playerStateEnum->GetNameByIndex( 0 ) == "Idle" );
	REQUIRE( playerStateEnum->GetValueByIndex( 0 ) == 0 );
	
	REQUIRE( playerStateEnum->GetNameByIndex( 1 ) == "Run" );
	REQUIRE( playerStateEnum->GetValueByIndex( 1 ) == 1 );
	
	REQUIRE( playerStateEnum->GetNameByIndex( 2 ) == "Jump" );
	REQUIRE( playerStateEnum->GetValueByIndex( 2 ) == 2 );
}

TEST_CASE( "Aggregate vars", "[aeMeta]" )
{
	const ae::Type* type = ae::GetType< AggregateClass >();
	REQUIRE( type );
	REQUIRE( type->GetVarCount( false ) == 2 );

	AggregateClass c;

	{
		const ae::Var* someClass = type->GetVarByName( "someClass", false );
		REQUIRE( someClass );
		REQUIRE( someClass->GetType() == ae::BasicType::Class );
		REQUIRE( someClass->GetTypeName() == ae::Str32( "SomeClass" ) );
		REQUIRE( someClass->GetSize() == sizeof(SomeClass) );
		REQUIRE( someClass->GetSubType() == ae::GetType< SomeClass >() );
		SomeClass* someClassPtr = someClass->GetPointer< SomeClass >( &c );
		REQUIRE( someClassPtr == &c.someClass );
		REQUIRE( someClass->GetPointer< SomeClass >( &c, 0 ) == nullptr );
		REQUIRE( someClass->GetPointer< SomeClass >( &c, 1 ) == nullptr );
		const ae::Type* varType = ae::GetTypeFromObject( someClassPtr );
		REQUIRE( varType );
		REQUIRE( varType == ae::GetType< SomeClass >() );
	}
	{
		const ae::Var* someClass1 = type->GetVarByName( "someClass1", false );
		REQUIRE( someClass1 );
		REQUIRE( someClass1->GetType() == ae::BasicType::Class );
		REQUIRE( someClass1->GetTypeName() == ae::Str32( "SomeClass" ) );
		REQUIRE( someClass1->GetSize() == sizeof(SomeClass) );
		REQUIRE( someClass1->GetSubType() == ae::GetType< SomeClass >() );
		SomeClass* someClass1Ptr = someClass1->GetPointer< SomeClass >( &c );
		REQUIRE( someClass1Ptr == &c.someClass1 );
		REQUIRE( someClass1->GetPointer< SomeClass >( &c, 0 ) == nullptr );
		REQUIRE( someClass1->GetPointer< SomeClass >( &c, 1 ) == nullptr );
		const ae::Type* varType = ae::GetTypeFromObject( someClass1Ptr );
		REQUIRE( varType );
		REQUIRE( varType == ae::GetType< SomeClass >() );
	}
}

TEST_CASE( "Array vars", "[aeMeta]" )
{
	const ae::Type* type = ae::GetType< ArrayClass >();
	REQUIRE( type );
	REQUIRE( type->GetVarCount( false ) == 6 );

	ArrayClass c;

	{
		const ae::Var* intArray = type->GetVarByName( "intArray", false );
		REQUIRE( intArray );
		REQUIRE( intArray->IsArray() );
		REQUIRE( intArray->GetType() == ae::BasicType::Int32 );
		REQUIRE( intArray->GetSize() == sizeof(int32_t) );
		REQUIRE( intArray->IsArrayFixedLength() );
		REQUIRE( intArray->GetArrayLength( &c ) == 3 );
		REQUIRE( intArray->GetArrayMaxLength() == 3 );
		REQUIRE( intArray->SetArrayLength( &c, 0 ) == 3 );
		REQUIRE( intArray->SetArrayLength( &c, 3 ) == 3 );
		REQUIRE( intArray->SetArrayLength( &c, 4 ) == 3 );
		REQUIRE( intArray->SetArrayLength( &c, 5 ) == 3 );
	}
	{
		const ae::Var* intArray2 = type->GetVarByName( "intArray2", false );
		REQUIRE( intArray2 );
		REQUIRE( intArray2->IsArray() );
		REQUIRE( intArray2->GetType() == ae::BasicType::Int32 );
		REQUIRE( intArray2->GetSize() == sizeof(int32_t) );
		REQUIRE( !intArray2->IsArrayFixedLength() );
		REQUIRE( intArray2->GetArrayLength( &c ) == 0 );
		REQUIRE( intArray2->GetArrayMaxLength() == 4 );
		REQUIRE( intArray2->SetArrayLength( &c, 4 ) == 4 );
		REQUIRE( c.intArray2.Length() == 4 );
		REQUIRE( intArray2->SetArrayLength( &c, 2 ) == 2 );
		REQUIRE( c.intArray2.Length() == 2 );
		REQUIRE( intArray2->SetArrayLength( &c, 3 ) == 3 );
		REQUIRE( c.intArray2.Length() == 3 );
		REQUIRE( intArray2->SetArrayLength( &c, 0 ) == 0 );
		REQUIRE( c.intArray2.Length() == 0 );
		REQUIRE( intArray2->SetArrayLength( &c, 1 ) == 1 );
		REQUIRE( c.intArray2.Length() == 1 );
	}
	{
		const ae::Var* intArray3 = type->GetVarByName( "intArray3", false );
		REQUIRE( intArray3 );
		REQUIRE( intArray3->IsArray() );
		REQUIRE( intArray3->GetType() == ae::BasicType::Int32 );
		REQUIRE( intArray3->GetSize() == sizeof(int32_t) );
		REQUIRE( !intArray3->IsArrayFixedLength() );
		REQUIRE( intArray3->GetArrayLength( &c ) == 0 );
		REQUIRE( intArray3->GetArrayMaxLength() == ae::MaxValue< uint32_t >() );
		REQUIRE( intArray3->SetArrayLength( &c, 4 ) == 4 );
		REQUIRE( c.intArray3.Length() == 4 );
		REQUIRE( intArray3->SetArrayLength( &c, 2 ) == 2 );
		REQUIRE( c.intArray3.Length() == 2 );
		REQUIRE( intArray3->SetArrayLength( &c, 3 ) == 3 );
		REQUIRE( c.intArray3.Length() == 3 );
		REQUIRE( intArray3->SetArrayLength( &c, 0 ) == 0 );
		REQUIRE( c.intArray3.Length() == 0 );
		REQUIRE( intArray3->SetArrayLength( &c, 1 ) == 1 );
		REQUIRE( c.intArray3.Length() == 1 );
		REQUIRE( intArray3->SetArrayLength( &c, 10020 ) == 10020 );
		REQUIRE( c.intArray3.Length() == 10020 );
		REQUIRE( intArray3->SetArrayLength( &c, 0 ) == 0 );
		REQUIRE( c.intArray3.Length() == 0 );
	}
	{
		const ae::Var* someClassArray = type->GetVarByName( "someClassArray", false );
		REQUIRE( someClassArray );
		REQUIRE( someClassArray->IsArray() );
		REQUIRE( someClassArray->GetType() == ae::BasicType::Class );
		REQUIRE( someClassArray->GetSize() == sizeof(SomeClass) );
		REQUIRE( someClassArray->GetSubType() == ae::GetType< SomeClass >() );
		REQUIRE( someClassArray->IsArrayFixedLength() );
		REQUIRE( someClassArray->GetArrayLength( &c ) == 3 );
		REQUIRE( someClassArray->GetArrayMaxLength() == 3 );
		REQUIRE( someClassArray->SetArrayLength( &c, 0 ) == 3 );
		REQUIRE( someClassArray->SetArrayLength( &c, 3 ) == 3 );
		REQUIRE( someClassArray->SetArrayLength( &c, 4 ) == 3 );
		REQUIRE( someClassArray->SetArrayLength( &c, 5 ) == 3 );
	}
	{
		const ae::Var* someClassArray2 = type->GetVarByName( "someClassArray2", false );
		REQUIRE( someClassArray2 );
		REQUIRE( someClassArray2->IsArray() );
		REQUIRE( someClassArray2->GetType() == ae::BasicType::Class );
		REQUIRE( someClassArray2->GetSize() == sizeof(SomeClass) );
		REQUIRE( someClassArray2->GetSubType() == ae::GetType< SomeClass >() );
		REQUIRE( !someClassArray2->IsArrayFixedLength() );
		REQUIRE( someClassArray2->GetArrayLength( &c ) == 0 );
		REQUIRE( someClassArray2->GetArrayMaxLength() == 4 );
		REQUIRE( someClassArray2->SetArrayLength( &c, 4 ) == 4 );
		REQUIRE( c.someClassArray2.Length() == 4 );
		REQUIRE( someClassArray2->SetArrayLength( &c, 2 ) == 2 );
		REQUIRE( c.someClassArray2.Length() == 2 );
		REQUIRE( someClassArray2->SetArrayLength( &c, 3 ) == 3 );
		REQUIRE( c.someClassArray2.Length() == 3 );
		REQUIRE( someClassArray2->SetArrayLength( &c, 0 ) == 0 );
		REQUIRE( c.someClassArray2.Length() == 0 );
		REQUIRE( someClassArray2->SetArrayLength( &c, 1 ) == 1 );
		REQUIRE( c.someClassArray2.Length() == 1 );
	}
	{
		const ae::Var* someClassArray3 = type->GetVarByName( "someClassArray3", false );
		REQUIRE( someClassArray3 );
		REQUIRE( someClassArray3->IsArray() );
		REQUIRE( someClassArray3->GetType() == ae::BasicType::Class );
		REQUIRE( someClassArray3->GetSize() == sizeof(SomeClass) );
		REQUIRE( someClassArray3->GetSubType() == ae::GetType< SomeClass >() );
		REQUIRE( !someClassArray3->IsArrayFixedLength() );
		REQUIRE( someClassArray3->GetArrayLength( &c ) == 0 );
		REQUIRE( someClassArray3->GetArrayMaxLength() == ae::MaxValue< uint32_t >() );
		REQUIRE( someClassArray3->SetArrayLength( &c, 4 ) == 4 );
		REQUIRE( c.someClassArray3.Length() == 4 );
		REQUIRE( someClassArray3->SetArrayLength( &c, 2 ) == 2 );
		REQUIRE( c.someClassArray3.Length() == 2 );
		REQUIRE( someClassArray3->SetArrayLength( &c, 3 ) == 3 );
		REQUIRE( c.someClassArray3.Length() == 3 );
		REQUIRE( someClassArray3->SetArrayLength( &c, 0 ) == 0 );
		REQUIRE( c.someClassArray3.Length() == 0 );
		REQUIRE( someClassArray3->SetArrayLength( &c, 1 ) == 1 );
		REQUIRE( c.someClassArray3.Length() == 1 );
		REQUIRE( someClassArray3->SetArrayLength( &c, 10020 ) == 10020 );
		REQUIRE( c.someClassArray3.Length() == 10020 );
		REQUIRE( someClassArray3->SetArrayLength( &c, 0 ) == 0 );
		REQUIRE( c.someClassArray3.Length() == 0 );

		REQUIRE( someClassArray3->SetArrayLength( &c, 3 ) == 3 );
		REQUIRE( someClassArray3->GetPointer< SomeClass >( &c, -1 ) == nullptr );
		REQUIRE( someClassArray3->GetPointer< SomeClass >( &c, 0 ) == &c.someClassArray3[ 0 ] );
		REQUIRE( someClassArray3->GetPointer< SomeClass >( &c, 1 ) == &c.someClassArray3[ 1 ] );
		REQUIRE( someClassArray3->GetPointer< SomeClass >( &c, 2 ) == &c.someClassArray3[ 2 ] );
	}
}

TEST_CASE( "enum string conversions", "[aeMeta]" )
{
	const ae::Enum* playerStateEnum = ae::GetEnum( "PlayerState" );
	REQUIRE( playerStateEnum->GetName() == ae::Str32( "PlayerState" ) );

	REQUIRE( playerStateEnum->GetNameByValue( PlayerState::Idle ) == "Idle" );
	REQUIRE( playerStateEnum->GetValueFromString( "Idle", (PlayerState)666 ) == PlayerState::Idle );
	REQUIRE( playerStateEnum->GetValueFromString( "0", (PlayerState)666 ) == PlayerState::Idle );

	REQUIRE( playerStateEnum->GetNameByValue( PlayerState::Run ) == "Run" );
	REQUIRE( playerStateEnum->GetValueFromString( "Run", (PlayerState)666 ) == PlayerState::Run );
	REQUIRE( playerStateEnum->GetValueFromString( "1", (PlayerState)666 ) == PlayerState::Run );

	REQUIRE( playerStateEnum->GetNameByValue( PlayerState::Jump ) == "Jump" );
	REQUIRE( playerStateEnum->GetValueFromString( "Jump", (PlayerState)666 ) == PlayerState::Jump );
	REQUIRE( playerStateEnum->GetValueFromString( "2", (PlayerState)666 ) == PlayerState::Jump );

	REQUIRE( ae::ToString( PlayerState::Idle ) == "Idle" );
	REQUIRE( ae::FromString< PlayerState >( "Idle", (PlayerState)666 ) == PlayerState::Idle );
	REQUIRE( ae::FromString< PlayerState >( "0", (PlayerState)666 ) == PlayerState::Idle );

	REQUIRE( ae::ToString( PlayerState::Run ) == "Run" );
	REQUIRE( ae::FromString< PlayerState >( "Run", (PlayerState)666 ) == PlayerState::Run );
	REQUIRE( ae::FromString< PlayerState >( "1", (PlayerState)666 ) == PlayerState::Run );

	REQUIRE( ae::ToString( PlayerState::Jump ) == "Jump" );
	REQUIRE( ae::FromString< PlayerState >( "Jump", (PlayerState)666 ) == PlayerState::Jump );
	REQUIRE( ae::FromString< PlayerState >( "2", (PlayerState)666 ) == PlayerState::Jump );
}

TEST_CASE( "enum string conversions using missing values", "[aeMeta]" )
{
	const ae::Enum* playerStateEnum = ae::GetEnum( "PlayerState" );
	REQUIRE( playerStateEnum->GetName() == ae::Str32( "PlayerState" ) );

	REQUIRE( playerStateEnum->GetNameByValue( (PlayerState)666 ) == "" );
	REQUIRE( playerStateEnum->GetValueFromString( "3", PlayerState::Jump ) == PlayerState::Jump );
	REQUIRE( playerStateEnum->GetValueFromString( "", PlayerState::Jump ) == PlayerState::Jump );

	REQUIRE( ae::ToString( (PlayerState)666 ) == "" );
	REQUIRE( ae::FromString< PlayerState >( "3", PlayerState::Jump ) == PlayerState::Jump );
	REQUIRE( ae::FromString< PlayerState >( "", PlayerState::Jump ) == PlayerState::Jump );
}

//------------------------------------------------------------------------------
// SomeClass + TestEnumClass
//------------------------------------------------------------------------------
TEST_CASE( "can read enum values from object using meta definition", "[aeMeta]" )
{
	SomeClass c;
	const ae::Type* type = ae::GetTypeFromObject( &c );
	const ae::Var* enumTestVar = type->GetVarByName( "enumTest", false );
	REQUIRE( enumTestVar->GetType() == ae::BasicType::Enum );
	
	c.enumTest = TestEnumClass::Five;
	REQUIRE( enumTestVar->GetObjectValueAsString( &c ) == "Five" );
	c.enumTest = TestEnumClass::Four;
	REQUIRE( enumTestVar->GetObjectValueAsString( &c ) == "Four" );
	c.enumTest = TestEnumClass::Three;
	REQUIRE( enumTestVar->GetObjectValueAsString( &c ) == "Three" );
	c.enumTest = TestEnumClass::Two;
	REQUIRE( enumTestVar->GetObjectValueAsString( &c ) == "Two" );
	c.enumTest = TestEnumClass::One;
	REQUIRE( enumTestVar->GetObjectValueAsString( &c ) == "One" );
	c.enumTest = TestEnumClass::Zero;
	REQUIRE( enumTestVar->GetObjectValueAsString( &c ) == "Zero" );
	c.enumTest = TestEnumClass::NegativeOne;
	REQUIRE( enumTestVar->GetObjectValueAsString( &c ) == "NegativeOne" );
}

TEST_CASE( "can't read invalid enum values from object using meta definition", "[aeMeta]" )
{
	SomeClass c;
	const ae::Type* type = ae::GetTypeFromObject( &c );
	const ae::Var* enumTestVar = type->GetVarByName( "enumTest", false );
	REQUIRE( enumTestVar->GetType() == ae::BasicType::Enum );
	
	c.enumTest = (TestEnumClass)6;
	REQUIRE( enumTestVar->GetObjectValueAsString( &c ) == "" );
}

TEST_CASE( "can set enum values on object using meta definition", "[aeMeta]" )
{
	SomeClass c;
	const ae::Type* type = ae::GetTypeFromObject( &c );
	const ae::Var* enumTestVar = type->GetVarByName( "enumTest", false );
	REQUIRE( enumTestVar->GetType() == ae::BasicType::Enum );
	
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "Five" ) );
	REQUIRE( c.enumTest == TestEnumClass::Five );
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "Four" ) );
	REQUIRE( c.enumTest == TestEnumClass::Four );
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "Three" ) );
	REQUIRE( c.enumTest == TestEnumClass::Three );
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "Two" ) );
	REQUIRE( c.enumTest == TestEnumClass::Two );
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "One" ) );
	REQUIRE( c.enumTest == TestEnumClass::One );
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "Zero" ) );
	REQUIRE( c.enumTest == TestEnumClass::Zero );
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "NegativeOne" ) );
	REQUIRE( c.enumTest == TestEnumClass::NegativeOne );
	
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "5" ) );
	REQUIRE( c.enumTest == TestEnumClass::Five );
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "4" ) );
	REQUIRE( c.enumTest == TestEnumClass::Four );
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "3" ) );
	REQUIRE( c.enumTest == TestEnumClass::Three );
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "2" ) );
	REQUIRE( c.enumTest == TestEnumClass::Two );
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "1" ) );
	REQUIRE( c.enumTest == TestEnumClass::One );
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "0" ) );
	REQUIRE( c.enumTest == TestEnumClass::Zero );
	REQUIRE( enumTestVar->SetObjectValueFromString( &c, "-1" ) );
	REQUIRE( c.enumTest == TestEnumClass::NegativeOne );
}

TEST_CASE( "can't set invalid enum values on object using meta definition", "[aeMeta]" )
{
	SomeClass c;
	const ae::Type* type = ae::GetTypeFromObject( &c );
	const ae::Var* enumTestVar = type->GetVarByName( "enumTest", false );
	REQUIRE( enumTestVar->GetType() == ae::BasicType::Enum );
	
	c.enumTest = TestEnumClass::Four;
	REQUIRE( !enumTestVar->SetObjectValueFromString( &c, "Six" ) );
	REQUIRE( c.enumTest == TestEnumClass::Four );
	REQUIRE( !enumTestVar->SetObjectValueFromString( &c, "6" ) );
	REQUIRE( c.enumTest == TestEnumClass::Four );
	REQUIRE( !enumTestVar->SetObjectValueFromString( &c, "" ) );
	REQUIRE( c.enumTest == TestEnumClass::Four );
}

//------------------------------------------------------------------------------
// SomeOldEnum
//------------------------------------------------------------------------------
TEST_CASE( "can register an already existing c-style enum", "[aeMeta]" )
{
	const ae::Enum* enumType = ae::GetEnum< SomeOldEnum >();
	REQUIRE( enumType == ae::GetEnum( "SomeOldEnum" ) );
	REQUIRE( enumType->Length() == 3 );
	REQUIRE( enumType->GetNameByIndex( 0 ) == "Bleep" );
	REQUIRE( enumType->GetNameByIndex( 1 ) == "Bloop" );
	REQUIRE( enumType->GetNameByIndex( 2 ) == "Blop" );
	REQUIRE( enumType->GetValueByIndex( 0 ) == 4 );
	REQUIRE( enumType->GetValueByIndex( 1 ) == 5 );
	REQUIRE( enumType->GetValueByIndex( 2 ) == 7 );
}

TEST_CASE( "existing c-style enum string conversions", "[aeMeta]" )
{
	const ae::Enum* enumType = ae::GetEnum< SomeOldEnum >();
	REQUIRE( enumType == ae::GetEnum( "SomeOldEnum" ) );

	REQUIRE( enumType->GetNameByValue( SomeOldEnum::Bleep ) == "Bleep" );
	REQUIRE( enumType->GetValueFromString( "Bleep", (SomeOldEnum)666 ) == SomeOldEnum::Bleep );
	REQUIRE( enumType->GetValueFromString( "4", (SomeOldEnum)666 ) == SomeOldEnum::Bleep );

	REQUIRE( enumType->GetNameByValue( SomeOldEnum::Bloop ) == "Bloop" );
	REQUIRE( enumType->GetValueFromString( "Bloop", (SomeOldEnum)666 ) == SomeOldEnum::Bloop );
	REQUIRE( enumType->GetValueFromString( "5", (SomeOldEnum)666 ) == SomeOldEnum::Bloop );

	REQUIRE( enumType->GetNameByValue( SomeOldEnum::Blop ) == "Blop" );
	REQUIRE( enumType->GetValueFromString( "Blop", (SomeOldEnum)666 ) == SomeOldEnum::Blop );
	REQUIRE( enumType->GetValueFromString( "7", (SomeOldEnum)666 ) == SomeOldEnum::Blop );
	
	REQUIRE( ae::ToString( SomeOldEnum::Bleep ) == "Bleep" );
	REQUIRE( ae::FromString< SomeOldEnum >( "Bleep", (SomeOldEnum)666 ) == SomeOldEnum::Bleep );
	REQUIRE( ae::FromString< SomeOldEnum >( "4", (SomeOldEnum)666 ) == SomeOldEnum::Bleep );

	REQUIRE( ae::ToString( SomeOldEnum::Bloop ) == "Bloop" );
	REQUIRE( ae::FromString< SomeOldEnum >( "Bloop", (SomeOldEnum)666 ) == SomeOldEnum::Bloop );
	REQUIRE( ae::FromString< SomeOldEnum >( "5", (SomeOldEnum)666 ) == SomeOldEnum::Bloop );

	REQUIRE( ae::ToString( SomeOldEnum::Blop ) == "Blop" );
	REQUIRE( ae::FromString< SomeOldEnum >( "Blop", (SomeOldEnum)666 ) == SomeOldEnum::Blop );
	REQUIRE( ae::FromString< SomeOldEnum >( "7", (SomeOldEnum)666 ) == SomeOldEnum::Blop );

	REQUIRE( enumType->GetNameByValue( (SomeOldEnum)666 ) == "" );
	REQUIRE( enumType->GetValueFromString( "3", SomeOldEnum::Bleep ) == SomeOldEnum::Bleep );
	REQUIRE( enumType->GetValueFromString( "", SomeOldEnum::Bleep ) == SomeOldEnum::Bleep );

	REQUIRE( ae::ToString( (SomeOldEnum)666 ) == "" );
	REQUIRE( ae::FromString< SomeOldEnum >( "3", SomeOldEnum::Bleep ) == SomeOldEnum::Bleep );
	REQUIRE( ae::FromString< SomeOldEnum >( "", SomeOldEnum::Bleep ) == SomeOldEnum::Bleep );
}

//------------------------------------------------------------------------------
// SomeOldPrefixEnum
//------------------------------------------------------------------------------
TEST_CASE( "can register an already existing c-style enum where each value has a prefix", "[aeMeta]" )
{
	const ae::Enum* enumType = ae::GetEnum< SomeOldPrefixEnum >();
	REQUIRE( enumType == ae::GetEnum( "SomeOldPrefixEnum" ) );
	REQUIRE( enumType->Length() == 3 );
	REQUIRE( enumType->GetNameByIndex( 0 ) == "Bleep" );
	REQUIRE( enumType->GetNameByIndex( 1 ) == "Bloop" );
	REQUIRE( enumType->GetNameByIndex( 2 ) == "Blop" );
	REQUIRE( enumType->GetValueByIndex( 0 ) == 4 );
	REQUIRE( enumType->GetValueByIndex( 1 ) == 5 );
	REQUIRE( enumType->GetValueByIndex( 2 ) == 7 );
}

//------------------------------------------------------------------------------
// SomeOldRenamedEnum
//------------------------------------------------------------------------------
TEST_CASE( "can register an already existing c-style enum where each value has a manually specified name", "[aeMeta]" )
{
	const ae::Enum* enumType = ae::GetEnum< SomeOldRenamedEnum >();
	REQUIRE( enumType == ae::GetEnum( "SomeOldRenamedEnum" ) );
	REQUIRE( enumType->Length() == 3 );
	REQUIRE( enumType->GetNameByIndex( 0 ) == "Bleep" );
	REQUIRE( enumType->GetNameByIndex( 1 ) == "Bloop" );
	REQUIRE( enumType->GetNameByIndex( 2 ) == "Blop" );
	REQUIRE( enumType->GetValueByIndex( 0 ) == 4 );
	REQUIRE( enumType->GetValueByIndex( 1 ) == 5 );
	REQUIRE( enumType->GetValueByIndex( 2 ) == 7 );
}

//------------------------------------------------------------------------------
// SomeNewEnum
//------------------------------------------------------------------------------
TEST_CASE( "can register an already existing enum class", "[aeMeta]" )
{
	const ae::Enum* enumType = ae::GetEnum< SomeNewEnum >();
	REQUIRE( enumType == ae::GetEnum( "SomeNewEnum" ) );
	REQUIRE( enumType->Length() == 3 );
	REQUIRE( enumType->GetNameByIndex( 0 ) == "Bleep" );
	REQUIRE( enumType->GetNameByIndex( 1 ) == "Bloop" );
	REQUIRE( enumType->GetNameByIndex( 2 ) == "Blop" );
	REQUIRE( enumType->GetValueByIndex( 0 ) == 4 );
	REQUIRE( enumType->GetValueByIndex( 1 ) == 5 );
	REQUIRE( enumType->GetValueByIndex( 2 ) == 7 );
}

//------------------------------------------------------------------------------
// A::B::SomeNewEnum
//------------------------------------------------------------------------------
TEST_CASE( "can register an already existing enum class in a nested namespace", "[aeMeta]" )
{
	const ae::Enum* enumType = ae::GetEnum< A::B::SomeNewEnum >();
	REQUIRE( enumType == ae::GetEnum( "A::B::SomeNewEnum" ) );
	REQUIRE( enumType->Length() == 3 );
	REQUIRE( enumType->GetNameByIndex( 0 ) == "Bleep" );
	REQUIRE( enumType->GetNameByIndex( 1 ) == "Bloop" );
	REQUIRE( enumType->GetNameByIndex( 2 ) == "Blop" );
	REQUIRE( enumType->GetValueByIndex( 0 ) == 4 );
	REQUIRE( enumType->GetValueByIndex( 1 ) == 5 );
	REQUIRE( enumType->GetValueByIndex( 2 ) == 7 );
}

//------------------------------------------------------------------------------
// Reference testing
//------------------------------------------------------------------------------
std::string RefTester::GetIdString( const RefTester* obj )
{
	return obj ? std::to_string( obj->id ) : std::string( "0" );
}

bool RefTester::StringToId( const char* str, uint32_t* idOut )
{
	char* endPtr = nullptr;
	uint32_t id = strtoul( str, &endPtr, 10 );
	if ( endPtr != str )
	{
		*idOut = id;
		return true;
	}
	return false;
}

void RefTesterManager::Destroy( RefTester* object )
{
	m_objectMap.Remove( object->id );
	ae::Delete( object );
}

RefTester* RefTesterManager::GetObjectById( uint32_t id )
{
	return id ? m_objectMap.Get( id, nullptr ) : nullptr;
}

TEST_CASE( "meta system can manipulate registered reference vars", "[aeMeta]" )
{
	const ae::Type* typeA = ae::GetType< RefTesterA >();
	REQUIRE( typeA );
	const ae::Var* typeA_notRef = typeA->GetVarByName( "notRef", false );
	const ae::Var* typeA_varA = typeA->GetVarByName( "refA", false );
	const ae::Var* typeA_varB = typeA->GetVarByName( "refB", false );
	REQUIRE( typeA_notRef );
	REQUIRE( typeA_varA );
	REQUIRE( typeA_varB );
	
	const ae::Type* typeB = ae::GetType< RefTesterB >();
	REQUIRE( typeB );
	const ae::Var* typeB_varA = typeB->GetVarByName( "refA", false );
	REQUIRE( typeB_varA );
	
	REQUIRE( !typeA_notRef->GetSubType() );
	REQUIRE( typeA_varA->GetSubType() == typeA );
	REQUIRE( typeA_varB->GetSubType() == typeB );
	REQUIRE( typeB_varA->GetSubType() == typeA );
	
	class RefSerializer : public ae::Var::Serializer
	{
	public:
		RefSerializer( RefTesterManager* manager ) : m_manager( manager ) {}
		std::string ObjectPointerToString( const ae::Object* obj ) const override
		{
			return RefTester::GetIdString( ae::Cast< RefTester >( obj ) );
		}
		bool StringToObjectPointer( const char* pointerVal, ae::Object** objOut ) const override
		{
			uint32_t id = 0;
			if ( RefTester::StringToId( pointerVal, &id ) )
			{
				*objOut = m_manager->GetObjectById( id );
				return true;
			}
			return false;
		}
	private:
		RefTesterManager* m_manager;
	};
	RefTesterManager manager;
	RefSerializer refSerializer = &manager;
	ae::Var::SetSerializer( &refSerializer );
	
	RefTesterA* testerA1 = manager.Create< RefTesterA >();
	RefTesterA* testerA2 = manager.Create< RefTesterA >();
	RefTesterB* testerB3 = manager.Create< RefTesterB >();
	
	// Validate ids
	REQUIRE( testerA1->id == 1 );
	REQUIRE( testerA2->id == 2 );
	REQUIRE( testerB3->id == 3 );
	REQUIRE( RefTester::GetIdString( testerA1 ) == "1" );
	REQUIRE( RefTester::GetIdString( testerA2 ) == "2" );
	REQUIRE( RefTester::GetIdString( testerB3 ) == "3" );
	
	// Validate initial reference values
	REQUIRE( testerA1->notRef == 0xfdfdfdfd );
	REQUIRE( testerA1->refA == nullptr );
	REQUIRE( testerA1->refB == nullptr );
	REQUIRE( testerA2->notRef == 0xfdfdfdfd );
	REQUIRE( testerA2->refA == nullptr );
	REQUIRE( testerA2->refB == nullptr );
	REQUIRE( testerB3->refA == nullptr );
	REQUIRE( typeA_varA->GetObjectValueAsString( testerA1 ) == "0" );
	REQUIRE( typeA_varA->GetObjectValueAsString( testerA2 ) == "0" );
	REQUIRE( typeB_varA->GetObjectValueAsString( testerB3 ) == "0" );
	
	// Set type A's ref to type A
	REQUIRE( typeA_varA->SetObjectValueFromString( testerA1, "2" ) );
	REQUIRE( testerA1->notRef == 0xfdfdfdfd );
	REQUIRE( testerA1->refA == testerA2 );
	REQUIRE( testerA1->refB == nullptr );
	REQUIRE( testerA2->notRef == 0xfdfdfdfd );
	REQUIRE( testerA2->refA == nullptr );
	REQUIRE( testerA2->refB == nullptr );
	REQUIRE( testerB3->refA == nullptr );
	
	// Set type B's ref to type A
	REQUIRE( typeB_varA->SetObjectValueFromString( testerB3, "2" ) );
	REQUIRE( testerA1->notRef == 0xfdfdfdfd );
	REQUIRE( testerA1->refA == testerA2 );
	REQUIRE( testerA1->refB == nullptr );
	REQUIRE( testerA2->notRef == 0xfdfdfdfd );
	REQUIRE( testerA2->refA == nullptr );
	REQUIRE( testerA2->refB == nullptr );
	REQUIRE( testerB3->refA == testerA2 );
	
	REQUIRE( typeA_varA->GetObjectValueAsString( testerA1 ) == "2" );
	REQUIRE( typeA_varA->GetObjectValueAsString( testerA2 ) == "0" );
	REQUIRE( typeB_varA->GetObjectValueAsString( testerB3 ) == "2" );
	
	// Set type A's ref B to type B
	REQUIRE( typeA_varB->SetObjectValueFromString( testerA2, "3" ) );
	REQUIRE( testerA1->notRef == 0xfdfdfdfd );
	REQUIRE( testerA1->refA == testerA2 );
	REQUIRE( testerA1->refB == nullptr );
	REQUIRE( testerA2->notRef == 0xfdfdfdfd );
	REQUIRE( testerA2->refA == nullptr );
	REQUIRE( testerA2->refB == testerB3 );
	REQUIRE( testerB3->refA == testerA2 );
	
	// Setting type A ref A to type B should do nothing
	REQUIRE( !typeA_varA->SetObjectValueFromString( testerA1, "3" ) );
	REQUIRE( testerA1->notRef == 0xfdfdfdfd );
	REQUIRE( testerA1->refA == testerA2 );
	REQUIRE( testerA1->refB == nullptr );
	REQUIRE( testerA2->notRef == 0xfdfdfdfd );
	REQUIRE( testerA2->refA == nullptr );
	REQUIRE( testerA2->refB == testerB3 );
	REQUIRE( testerB3->refA == testerA2 );
	
	// Setting ref from random string value does nothing
	REQUIRE( !typeA_varA->SetObjectValueFromString( testerA1, "qwerty" ) );
	REQUIRE( testerA1->notRef == 0xfdfdfdfd );
	REQUIRE( testerA1->refA == testerA2 );
	REQUIRE( testerA1->refB == nullptr );
	REQUIRE( testerA2->notRef == 0xfdfdfdfd );
	REQUIRE( testerA2->refA == nullptr );
	REQUIRE( testerA2->refB == testerB3 );
	REQUIRE( testerB3->refA == testerA2 );
	
	// Setting ref to null value succeeds and clears ref
	REQUIRE( typeA_varA->SetObjectValueFromString( testerA1, "0" ) );
	REQUIRE( testerA1->notRef == 0xfdfdfdfd );
	REQUIRE( testerA1->refA == nullptr );
	REQUIRE( testerA1->refB == nullptr );
	REQUIRE( testerA2->notRef == 0xfdfdfdfd );
	REQUIRE( testerA2->refA == nullptr );
	REQUIRE( testerA2->refB == testerB3 );
	REQUIRE( testerB3->refA == testerA2 );
	
	manager.Destroy( testerA1 );
	manager.Destroy( testerA2 );
	manager.Destroy( testerB3 );
}

TEST_CASE( "bitfield registration", "[aeMeta]" )
{
	const ae::Enum* gamePadBitFieldEnum = ae::GetEnum( "GamePadBitField" );

	REQUIRE( gamePadBitFieldEnum->GetName() == ae::Str32( "GamePadBitField" ) );
	REQUIRE( gamePadBitFieldEnum->GetValueFromString( "A", (GamePadBitField)666 ) == GamePadBitField::A );
	REQUIRE( gamePadBitFieldEnum->GetValueFromString( "1", (GamePadBitField)666 ) == GamePadBitField::A );

	uint16_t something = GamePadBitField::Y | GamePadBitField::X;
	something = something | GamePadBitField::A;
	something = GamePadBitField::B | something;
	//something |= GamePadBitField::B;
	
	// REQUIRE( playerStateEnum->TypeSize() == 2 );
	// REQUIRE( playerStateEnum->TypeIsSigned() == false );
	
	// REQUIRE( playerStateEnum->Length() == 3 );
	
	// REQUIRE( playerStateEnum->GetNameByIndex( 0 ) == "Idle" );
	// REQUIRE( playerStateEnum->GetValueByIndex( 0 ) == 0 );
	
	// REQUIRE( playerStateEnum->GetNameByIndex( 1 ) == "Run" );
	// REQUIRE( playerStateEnum->GetValueByIndex( 1 ) == 1 );
	
	// REQUIRE( playerStateEnum->GetNameByIndex( 2 ) == "Jump" );
	// REQUIRE( playerStateEnum->GetValueByIndex( 2 ) == 2 );
}

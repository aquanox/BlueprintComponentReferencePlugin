// Copyright 2024, Aquanox.

#pragma once

#include "CoreMinimal.h"
#include "UObject/UnrealType.h"

class UBlueprint;

#ifndef WITH_METADATA_MARSHALLER
#define WITH_METADATA_MARSHALLER 0
#endif

/**
 * An abstraction of various metadata sources, providing unified accessor for container
 */
class BLUEPRINTCOMPONENTREFERENCEEDITOR_API FMetadataSettingsSource
{
public:
	virtual ~FMetadataSettingsSource() = default;
	virtual bool IsEditable() const { return false; }
	virtual bool IsNoClear() const  { return false; }
	virtual FString GetName() const = 0;

	virtual bool HasValue(const FName& InName) const = 0;
	virtual void UnsetValue(const FName& InName) = 0;
	virtual TOptional<FString> GetStringValue(const FName& InName) const = 0;
	TOptional<bool> GetBooleanValue(const FName& InName, bool IfEmpty = true) const;
	TOptional<bool> GetFlagValue(const FName& InName) const;

	template<typename T>
	TOptional<T> GetEnumValue(const FName& InName) const
	{
		auto IntValue = GetEnumValue( StaticEnum<T>(), InName );
		if (IntValue.IsSet())
			return TOptional<T>((T)IntValue.GetValue());
		return TOptional<T>();
	}

	TOptional<int64> GetEnumValue(const UEnum* Enum, const FName& InName) const;

	TOptional<FSoftClassPath> GetLazyClassValue(const FName& InName) const;
	TOptional<TArray<FSoftClassPath>> GetLazyClassListValue(const FName& InName) const;

	template<typename T>
	TOptional<TSoftClassPtr<T>> GetLazyClassValue(const FName& InName) const
	{
		auto Value = GetLazyClassValue(InName);
		if (Value.IsSet())
		{
			// build out of FSoftObjectPath, templates fail deduce
			return TOptional<TSoftClassPtr<T>>( TSoftClassPtr<T> (  *Value ) );
		}
		return TOptional<TSoftClassPtr<T>>();
	}

	template<typename T>
	TOptional<TArray<TSoftClassPtr<T>>> GetLazyClassListValue(const FName& InName) const
	{
		auto Value = GetLazyClassListValue(InName);
		if (Value.IsSet())
		{
			TArray<TSoftClassPtr<T>> Result;
			Algo::Transform(*Value, Result, [](const FSoftClassPath& E) -> TSoftClassPtr<T> { return TSoftClassPtr<T>(E); });
			return TOptional<TArray<TSoftClassPtr<T>>>( Result );
		}
		return TOptional<TArray<TSoftClassPtr<T>>>();
	}

	virtual void SetStringValue(const FName& InName, TOptional<FString> InValue) const = 0;
	void SetBooleanValue(const FName& InName, TOptional<bool> InValue);
	void SetFlagValue(const FName& InName, TOptional<bool> InValue);

	template<typename T>
	void SetEnumValue(const FName& InName, TOptional<T> InValue)
	{
		TOptional<FString> AsString;
		if (InValue.IsSet())
		{
			AsString = StaticEnum<T>()->GetNameStringByValue((int64)InValue.GetValue());
		}
		SetStringValue(InName, AsString);
	}

	void SetLazyClassValue(const FName& InName, TOptional<FSoftClassPath> InValue);
	void SetLazyClassListValue(const FName& InName, TOptional<TArray<FSoftClassPath>> InValue);

	template<typename T>
	void SetLazyClassValue(const FName& InName, TOptional<TSoftClassPtr<T>> InValue)
	{
		TOptional<FSoftClassPath> Converted;
		if (InValue.IsSet())
		{
			Converted = TOptional<FSoftClassPath>( FSoftClassPath( InValue->ToString() ) );
		}
		SetLazyClassValue(InName, Converted);
	}

	template<typename T>
	void SetLazyClassListValue(const FName& InName, TOptional<TArray<TSoftClassPtr<T>>> InValue)
	{
		TOptional<TArray<FSoftClassPath>> Converted;
		if (InValue.IsSet())
		{
			TArray<FSoftClassPath> Tmp;
			Algo::Transform(*InValue, Tmp, [](const TSoftClassPtr<T> & E) -> FSoftClassPath { return E.ToString(); });
			Converted = TOptional<TArray<FSoftClassPath>>( Tmp );
		}
		SetLazyClassListValue(InName, Converted);
	}
};

/**
 *
 */
class BLUEPRINTCOMPONENTREFERENCEEDITOR_API FMetadataSettingsPropertySource : public FMetadataSettingsSource
{
	const FProperty* const Source;
	const UBlueprint* const Context;
public:
	explicit FMetadataSettingsPropertySource(const FProperty* source, const UBlueprint* Context = nullptr) : Source(source), Context(Context) {}

	virtual bool IsEditable() const override { return true; }
	virtual FString GetName() const override { return Source->GetName(); }
	bool IsNoClear() const override { return (Source->PropertyFlags & CPF_NoClear) != 0; } // idk how to abstract is prettier

	virtual bool HasValue(const FName& InName) const override;
	virtual void UnsetValue(const FName& InName) override;
	virtual TOptional<FString> GetStringValue(const FName& InName) const override;

	virtual void SetStringValue(const FName& InName, TOptional<FString> InValue) const;
};

/**
 *
 */
class BLUEPRINTCOMPONENTREFERENCEEDITOR_API FMetadataSettingsTypeSource : public FMetadataSettingsSource
{
	const UStruct* Source = nullptr;
public:
	explicit FMetadataSettingsTypeSource(const UStruct* source) : Source(source)   {}
	virtual FString GetName() const override { return Source->GetName(); }

	virtual bool HasValue(const FName& InName) const override;
	virtual void UnsetValue(const FName& InName) override {}
	virtual TOptional<FString> GetStringValue(const FName& InName) const override;
	virtual void SetStringValue(const FName& InName, TOptional<FString> InValue) const override { unimplemented(); }
};

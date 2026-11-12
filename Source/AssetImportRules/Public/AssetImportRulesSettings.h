// Copyright 2023-2026 Jimi Demetriou All Rights Reserved. Jimi Games | jimid.io

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "Engine/TextureDefines.h"
#include "AssetImportRulesSettings.generated.h"

class UTexture;

/**
 * Texture settings forced onto every texture imported into a folder.
 * Each setting has a toggle; untoggled settings are left as the importer produced them.
 * Defaults are a typical "UI icon" preset.
 */
USTRUCT(BlueprintType)
struct ASSETIMPORTRULES_API FTextureImportRule
{
	GENERATED_BODY()

	/** Content folder this rule applies to, e.g. /Game/UI/Icons. */
	UPROPERTY(EditAnywhere, Category = "Rule", meta = (LongPackageName))
	FDirectoryPath Folder;

	/** Also apply to textures in subfolders. The most specific (longest) matching folder wins. */
	UPROPERTY(EditAnywhere, Category = "Rule")
	bool bIncludeSubfolders = true;

	UPROPERTY(EditAnywhere, Category = "Rule", meta = (InlineEditConditionToggle))
	bool bSetCompressionSettings = true;

	UPROPERTY(EditAnywhere, Category = "Rule", meta = (EditCondition = "bSetCompressionSettings"))
	TEnumAsByte<TextureCompressionSettings> CompressionSettings = TC_EditorIcon;

	UPROPERTY(EditAnywhere, Category = "Rule", meta = (InlineEditConditionToggle))
	bool bSetLODGroup = true;

	UPROPERTY(EditAnywhere, Category = "Rule", meta = (EditCondition = "bSetLODGroup"))
	TEnumAsByte<TextureGroup> LODGroup = TEXTUREGROUP_UI;

	UPROPERTY(EditAnywhere, Category = "Rule", meta = (InlineEditConditionToggle))
	bool bSetMipGenSettings = true;

	UPROPERTY(EditAnywhere, Category = "Rule", meta = (EditCondition = "bSetMipGenSettings"))
	TEnumAsByte<TextureMipGenSettings> MipGenSettings = TMGS_NoMipmaps;

	UPROPERTY(EditAnywhere, Category = "Rule", meta = (InlineEditConditionToggle))
	bool bSetNeverStream = true;

	UPROPERTY(EditAnywhere, Category = "Rule", meta = (EditCondition = "bSetNeverStream"))
	bool bNeverStream = true;

	UPROPERTY(EditAnywhere, Category = "Rule", meta = (InlineEditConditionToggle))
	bool bSetSRGB = false;

	UPROPERTY(EditAnywhere, Category = "Rule", meta = (EditCondition = "bSetSRGB"))
	bool bSRGB = true;

	UPROPERTY(EditAnywhere, Category = "Rule", meta = (InlineEditConditionToggle))
	bool bSetVirtualTextureStreaming = true;

	/** UI textures can't be virtual textures; keep this off for icon folders. */
	UPROPERTY(EditAnywhere, Category = "Rule", meta = (EditCondition = "bSetVirtualTextureStreaming"))
	bool bVirtualTextureStreaming = false;

	/** Returns true if this rule covers the given package path (e.g. /Game/UI/Icons/Sub). */
	bool Matches(const FString& PackagePath) const;

	/**
	 * Applies the toggled settings. Returns true if anything differs from the rule.
	 * With bDryRun, only reports whether anything would change. Does not Pre/PostEditChange.
	 */
	bool ApplyTo(UTexture* Texture, bool bDryRun = false) const;

	/** Folder path with any trailing slash removed. */
	FString GetNormalizedFolder() const;
};

/** Per-folder import rules. Stored in DefaultEditor.ini so they are shared with the team. */
UCLASS(config = Editor, defaultconfig, meta = (DisplayName = "Asset Import Rules"))
class ASSETIMPORTRULES_API UAssetImportRulesSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	/** Master switch. */
	UPROPERTY(config, EditAnywhere, Category = "General")
	bool bEnabled = true;

	/** Log each texture a rule is applied to. */
	UPROPERTY(config, EditAnywhere, Category = "General")
	bool bLogApplied = true;

	UPROPERTY(config, EditAnywhere, Category = "Textures", meta = (TitleProperty = "Folder"))
	TArray<FTextureImportRule> TextureRules;

	/** Most specific rule matching the asset's folder, or null. */
	const FTextureImportRule* FindTextureRule(const UObject* Asset) const;
};

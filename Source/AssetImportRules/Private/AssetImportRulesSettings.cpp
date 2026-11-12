// Copyright 2023-2026 Jimi Demetriou All Rights Reserved. Jimi Games | jimid.io

#include "AssetImportRulesSettings.h"

#include "Engine/Texture.h"
#include "Misc/PackageName.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AssetImportRulesSettings)

FString FTextureImportRule::GetNormalizedFolder() const
{
	FString Result = Folder.Path;
	while (Result.Len() > 1 && Result.EndsWith(TEXT("/")))
	{
		Result.LeftChopInline(1);
	}
	return Result;
}

bool FTextureImportRule::Matches(const FString& PackagePath) const
{
	const FString RuleFolder = GetNormalizedFolder();
	if (RuleFolder.IsEmpty())
	{
		return false;
	}
	if (PackagePath.Equals(RuleFolder, ESearchCase::IgnoreCase))
	{
		return true;
	}
	return bIncludeSubfolders && PackagePath.StartsWith(RuleFolder + TEXT("/"), ESearchCase::IgnoreCase);
}

bool FTextureImportRule::ApplyTo(UTexture* Texture, bool bDryRun) const
{
	if (!Texture)
	{
		return false;
	}

	bool bChanged = false;
	auto Set = [&bChanged, bDryRun](auto& Target, auto Value)
	{
		if (Target != Value)
		{
			if (!bDryRun)
			{
				Target = Value;
			}
			bChanged = true;
		}
	};

	if (bSetCompressionSettings) { Set(Texture->CompressionSettings, CompressionSettings); }
	if (bSetLODGroup)            { Set(Texture->LODGroup, LODGroup); }
#if WITH_EDITORONLY_DATA
	if (bSetMipGenSettings)      { Set(Texture->MipGenSettings, MipGenSettings); }
#endif

	// Bitfields can't bind to auto&, so handle them explicitly.
	if (bSetNeverStream && !!Texture->NeverStream != bNeverStream)
	{
		if (!bDryRun) { Texture->NeverStream = bNeverStream; }
		bChanged = true;
	}
	if (bSetSRGB && !!Texture->SRGB != bSRGB)
	{
		if (!bDryRun) { Texture->SRGB = bSRGB; }
		bChanged = true;
	}
	if (bSetVirtualTextureStreaming && !!Texture->VirtualTextureStreaming != bVirtualTextureStreaming)
	{
		if (!bDryRun) { Texture->VirtualTextureStreaming = bVirtualTextureStreaming; }
		bChanged = true;
	}

	return bChanged;
}

const FTextureImportRule* UAssetImportRulesSettings::FindTextureRule(const UObject* Asset) const
{
	if (!Asset)
	{
		return nullptr;
	}

	const FString PackagePath = FPackageName::GetLongPackagePath(Asset->GetPackage()->GetName());

	const FTextureImportRule* Best = nullptr;
	int32 BestLen = -1;
	for (const FTextureImportRule& Rule : TextureRules)
	{
		if (Rule.Matches(PackagePath))
		{
			const int32 Len = Rule.GetNormalizedFolder().Len();
			if (Len > BestLen)
			{
				Best = &Rule;
				BestLen = Len;
			}
		}
	}
	return Best;
}

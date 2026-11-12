// Copyright 2023-2026 Jimi Demetriou All Rights Reserved. Jimi Games | jimid.io

#include "AssetImportRulesSettings.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Engine/Texture.h"
#include "HAL/IConsoleManager.h"
#include "InterchangeManager.h"
#include "Modules/ModuleManager.h"
#include "Subsystems/ImportSubsystem.h"
#include "TextureCompiler.h"

DEFINE_LOG_CATEGORY_STATIC(LogAssetImportRules, Log, All);

namespace AssetImportRules
{
	/** Applies the matching rule to a texture. Returns true if the texture was modified. */
	static bool ApplyRules(UObject* Asset)
	{
		const UAssetImportRulesSettings* Settings = GetDefault<UAssetImportRulesSettings>();
		if (!Settings->bEnabled)
		{
			return false;
		}

		UTexture* Texture = Cast<UTexture>(Asset);
		if (!Texture)
		{
			return false;
		}

		const FTextureImportRule* Rule = Settings->FindTextureRule(Texture);
		if (!Rule)
		{
			return false;
		}

		// Dry run first so textures that already match aren't rebuilt or dirtied.
		if (!Rule->ApplyTo(Texture, /*bDryRun*/ true))
		{
			return false;
		}

		// Imported textures may still be compiling asynchronously; finish before editing.
		FTextureCompilingManager::Get().FinishCompilation({ Texture });

		Texture->PreEditChange(nullptr);
		Rule->ApplyTo(Texture);
		Texture->PostEditChange();
		Texture->MarkPackageDirty();

		if (Settings->bLogApplied)
		{
			UE_LOG(LogAssetImportRules, Log, TEXT("Applied texture rule '%s' to %s"),
				*Rule->GetNormalizedFolder(), *Texture->GetPathName());
		}
		return true;
	}

	/** AssetImportRules.ApplyToExisting [/Game/Folder] - re-applies rules to textures already in the project. */
	static void ApplyToExisting(const TArray<FString>& Args)
	{
		const UAssetImportRulesSettings* Settings = GetDefault<UAssetImportRulesSettings>();

		TArray<FName> Folders;
		if (Args.Num() > 0)
		{
			for (const FString& Arg : Args)
			{
				Folders.Add(*Arg);
			}
		}
		else
		{
			for (const FTextureImportRule& Rule : Settings->TextureRules)
			{
				if (!Rule.GetNormalizedFolder().IsEmpty())
				{
					Folders.Add(*Rule.GetNormalizedFolder());
				}
			}
		}

		if (Folders.IsEmpty())
		{
			UE_LOG(LogAssetImportRules, Warning, TEXT("No folders to process. Add texture rules in Project Settings > Plugins > Asset Import Rules."));
			return;
		}

		FARFilter Filter;
		Filter.PackagePaths = Folders;
		Filter.bRecursivePaths = true;
		Filter.ClassPaths.Add(UTexture::StaticClass()->GetClassPathName());
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> Assets;
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get().GetAssets(Filter, Assets);

		int32 NumChanged = 0;
		for (const FAssetData& AssetData : Assets)
		{
			if (UObject* Asset = AssetData.GetAsset())
			{
				NumChanged += ApplyRules(Asset) ? 1 : 0;
			}
		}

		UE_LOG(LogAssetImportRules, Display, TEXT("Checked %d texture(s), updated %d. Save the modified assets to keep the changes."),
			Assets.Num(), NumChanged);
	}
}

class FAssetImportRulesModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		// Legacy factory-based imports.
		if (GEditor)
		{
			if (UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>())
			{
				LegacyHandle = ImportSubsystem->OnAssetPostImport.AddLambda(
					[](UFactory*, UObject* Asset) { AssetImportRules::ApplyRules(Asset); });
			}
		}

		// Interchange imports (the default texture importer in UE 5.x). Fires for import and reimport.
		InterchangeHandle = UInterchangeManager::GetInterchangeManager().OnAssetPostImport.AddLambda(
			[](UObject* Asset) { AssetImportRules::ApplyRules(Asset); });

		ApplyCommand = MakeUnique<FAutoConsoleCommand>(
			TEXT("AssetImportRules.ApplyToExisting"),
			TEXT("Re-apply Asset Import Rules to textures already in the project. Optional args: content folders to limit to (e.g. /Game/UI/Icons)."),
			FConsoleCommandWithArgsDelegate::CreateStatic(&AssetImportRules::ApplyToExisting));
	}

	virtual void ShutdownModule() override
	{
		ApplyCommand.Reset();

		if (GEditor && LegacyHandle.IsValid())
		{
			if (UImportSubsystem* ImportSubsystem = GEditor->GetEditorSubsystem<UImportSubsystem>())
			{
				ImportSubsystem->OnAssetPostImport.Remove(LegacyHandle);
			}
		}

		if (InterchangeHandle.IsValid() && !IsEngineExitRequested())
		{
			UInterchangeManager::GetInterchangeManager().OnAssetPostImport.Remove(InterchangeHandle);
		}
	}

private:
	FDelegateHandle LegacyHandle;
	FDelegateHandle InterchangeHandle;
	TUniquePtr<FAutoConsoleCommand> ApplyCommand;
};

IMPLEMENT_MODULE(FAssetImportRulesModule, AssetImportRules)

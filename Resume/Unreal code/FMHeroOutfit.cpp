
FMHeroOutfitData::FMHeroOutfitData()
{
	constexpr int Count = static_cast<int>(EMUnitPartType::Max);
	Parts.SetNum(Count);
	PartEffects.SetNum(Count);
}

void FMHeroOutfitData::SetFromActorPacket(const MActorT* InActorAction)
{
	TransformTID = 0;

	if (InActorAction)
	{
		const FMPawnData* Outfit = nullptr;
		if (InActorAction->heroCostumeTID > 0)
		{
			Outfit = MDATAMGR->GetPawnData(InActorAction->heroCostumeTID);
		}
		else if (InActorAction->actortid > 0)
		{
			Outfit = MDATAMGR->GetPawnData(InActorAction->actortid);
		}

		for (const std::shared_ptr<MCharacterCustomT>& Custom : InActorAction->customInfo)
		{
			if (Custom->pcTypeToUnitID == static_cast<int>(EMPCTypeToUnitID::Female))
			{
				FemaleCustomizing.Setting(Custom.get());
			}
			else
			{
				MaleCustomizing.Setting(Custom.get());
			}
		}

		BasePawnTID = InActorAction->actortid;
		WeaponCostumeTID = InActorAction->equipmentCostumeTID;
		OutfitData = Outfit;
	}

	Update();
}

void FMHeroOutfitData::SetFromServerCharacterData(const int InCharacterUID)
{
	TransformTID = 0;

	if (const UMNetCharacterData* NetCharacterData = MNETDATAMGR->GetNetCharacterData(InCharacterUID))
	{
		const FMPawnData* Outfit = nullptr;
		if (NetCharacterData->HeroCostumeTID > 0)
		{
			Outfit = MDATAMGR->GetPawnData(NetCharacterData->HeroCostumeTID);
		}
		else if (NetCharacterData->TID > 0)
		{
			Outfit = MDATAMGR->GetPawnData(NetCharacterData->TID);
		}

		MaleCustomizing.Setting(NetCharacterData->MaleCustomizingData);
		FemaleCustomizing.Setting(NetCharacterData->FemaleCustomizingData);

		BasePawnTID = NetCharacterData->TID;
		WeaponCostumeTID = NetCharacterData->EquipmentCostumeTID;
		OutfitData = Outfit;
	}

	Update();
}

void FMHeroOutfitData::SetFromPawnDataWithServerCustomizing(const int InPawnTID, const int InCharacterUID)
{
	TransformTID = 0;

	if (const FMPawnData* Pawn = MDATAMGR->GetPawnData(InPawnTID))
	{
		OutfitData = Pawn;
		BasePawnTID = InPawnTID;
	}

	if (const UMNetCharacterData* NetCharacterData = MNETDATAMGR->GetNetCharacterData(InCharacterUID))
	{
		MaleCustomizing.Setting(NetCharacterData->MaleCustomizingData);
		FemaleCustomizing.Setting(NetCharacterData->FemaleCustomizingData);

		BasePawnTID = NetCharacterData->TID;
		WeaponCostumeTID = NetCharacterData->EquipmentCostumeTID;
	}
	else
	{
		if (OutfitData)
		{
			MaleCustomizing.SettingAsReferenceCharacter(1, OutfitData->PawnClass);
			FemaleCustomizing.SettingAsReferenceCharacter(51, OutfitData->PawnClass);
		}
	}

	Update();
}

void FMHeroOutfitData::SetFromPawnData(const int InPawnTID)
{
	if (const FMPawnData* PawnData = MDATAMGR->GetPawnData(InPawnTID))
	{
		BasePawnTID = InPawnTID;
		OutfitData = PawnData;
		WeaponCostumeTID = 0;
		TransformTID = 0;

		MaleCustomizing.SettingAsReferenceCharacter(1, PawnData->PawnClass);
		FemaleCustomizing.SettingAsReferenceCharacter(51, PawnData->PawnClass);
	}

	Update();
}

void FMHeroOutfitData::SetCostume(const int InPawnTID, const int InWeaponTID)
{
	const int TID = InPawnTID > 0 ? InPawnTID : BasePawnTID;

	OutfitData = MDATAMGR->GetPawnData(TID);
	WeaponCostumeTID = InWeaponTID;

	Update();
}

void FMHeroOutfitData::SetTransform(const int InCharacterUID, const int InTransformID)
{
	SetFromServerCharacterData(InCharacterUID);

	TransformTID = InTransformID;

	Update();
}

void FMHeroOutfitData::Update()
{
	if (OutfitData == nullptr)
	{
		return;
	}

	for (int& Part : Parts)
	{
		Part = 0;
	}

	for (FString& Effect : PartEffects)
	{
		Effect = TEXT("");
	}

	BundleID = OutfitData->UnitID;
	PartEffects[static_cast<int>(EMUnitPartType::Body)] = OutfitData->EffectSocketID;

	Parts[static_cast<int>(EMUnitPartType::Weapon)] = OutfitData->WeaponMeshID;
	Parts[static_cast<int>(EMUnitPartType::Body)] = OutfitData->BodyMeshID;
	Parts[static_cast<int>(EMUnitPartType::Helmet)] = bHideHelmet ? 0 : OutfitData->HelmetMeshID;

	const FMHeroCustomizingInfo& Customizing = OutfitData->Gender == EMGender::Female ? FemaleCustomizing : MaleCustomizing;

	// 얼굴
	const int HeadPartID = Customizing.CustomParts[static_cast<int>(EMUnitPartType::Head)];
	if (HeadPartID > 0)
	{
		if (const FMCustomizingAssetData* Asset = MDATAMGR->GetCustomizingAssetData(HeadPartID))
		{
			Parts[static_cast<int>(EMUnitPartType::Head)] = Asset->Mesh;
		}
	}

	// 머리
	if (bHideHelmet || (OutfitData->HelmetMeshID == 0 && OutfitData->HairMeshID == 0))
	{
		const int HairPartID = Customizing.CustomParts[static_cast<int>(EMUnitPartType::Hair)];
		if (HairPartID > 0)
		{
			if (const FMCustomizingAssetData* Asset = MDATAMGR->GetCustomizingAssetData(HairPartID))
			{
				Parts[static_cast<int>(EMUnitPartType::Hair)] = Asset->Mesh;
			}
		}
	}
	else
	{
		Parts[static_cast<int>(EMUnitPartType::Hair)] = OutfitData->HairMeshID;
	}

	// 어셋을 찾지 못했을 경우 기본 어셋으로 지정해준다.
	if (Parts[static_cast<int>(EMUnitPartType::Head)] == 0)
	{
		if (const FMCustomizingPresetData* Preset = MDATAMGR->GetReferencePresetData(OutfitData->UnitID, OutfitData->PawnClass))
		{
			if (const FMCustomizingAssetData* Asset = MDATAMGR->GetCustomizingAssetData(Preset->FaceMesh))
			{
				Parts[static_cast<int>(EMUnitPartType::Head)] = Asset->Mesh;
			}

			if (const FMCustomizingAssetData* Asset = MDATAMGR->GetCustomizingAssetData(Preset->HairMesh))
			{
				Parts[static_cast<int>(EMUnitPartType::Hair)] = Asset->Mesh;
			}
		}
	}

	Materials = Customizing.CustomMaterials;

	if (WeaponCostumeTID > 0)
	{
		if (const FMItemData* Weapon = MDATAMGR->GetItemData(WeaponCostumeTID))
		{
			Parts[static_cast<int>(EMUnitPartType::Weapon)] = Weapon->WeaponMeshID;
			PartEffects[static_cast<int>(EMUnitPartType::Weapon)] = OutfitData->Gender == EMGender::Female ? Weapon->FemaleEffectSocketID : Weapon->MaleEffectSocketID;
		}
	}

	if (TransformTID > 0)
	{
		if (const FMTransformData* TransformData = MDATAMGR->GetTransformData(TransformTID))
		{
			for (int i = 0; i < TransformData->UnitID.Num(); i++)
			{
				if (TransformData->UnitID[i] != OutfitData->UnitID)
				{
					continue;
				}

				PartEffects[static_cast<int>(EMUnitPartType::Weapon)] = TransformData->EquipmentEffectSocketID.Num() > i ? TransformData->EquipmentEffectSocketID[i] : TEXT("");
			}
		}
	}

	IsOutfitChanged = true;
}

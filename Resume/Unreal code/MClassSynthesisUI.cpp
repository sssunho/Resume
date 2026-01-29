/***********************************************************************************
 *                                                                                 *
 * Copyright. (C) 2023 Mobirix.                                                    *
 * Website: https://mobirix.com													   *
 *                                                                                 *
 ***********************************************************************************/

#include "UI/Class/MClassSynthesisUI.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Data/MDataManager.h"
#include "Data/Base/MDataEnumString.h"
#include "Data/Base/MdataStruct.h"
#include "Util/MGlobalFunctionLib.h"
#include "Network/MNetworkManager.h"
#include "Network/Data/MNetworkDataManager.h"
#include "UI/MUIManager.h"
#include "UI/Class/MClassTabUI.h"
#include "UI/Common/MCharacterListUI.h"
#include "UI/Common/MCommonBtn.h"
#include "UI/Common/Icon/MPawnIconUI.h"
#include "UI/Common/PressButton/MCategoryTabEntryUI.h"
#include "UI/Gacha/MGachaUI.h"
#include "Util/MStringHelper.h"

#define MAX_SYNTHESIS_COUNT 11

void UMClassSynthesisUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (const UMBaseUIWidget* Menubar = Slot->GetTypedOuter<UMClassTabUI>())
	{
		if (const FMUILoadData* LoadData = Menubar->GetUILoadInfo())
		{
			if (LoadData->ID == "ClassUI")
			{
				PawnType = EMPawnType::Hero;
			}
			else if (LoadData->ID == "PetUI")
			{
				PawnType = EMPawnType::Pet;
			}
			else if (LoadData->ID == "VehicleUI")
			{
				PawnType = EMPawnType::Vehicle;
			}
		}
	}

	if (GradeListView)
	{
		GradeListView->OnItemSelectionChanged().AddUObject(this, &UMClassSynthesisUI::OnClickedGrade);
		GradeListView->SetScrollbarVisibility(ESlateVisibility::Collapsed);
	}

	if (ButtonAuto)
	{
		ButtonAuto->SetCommonBtn(EMCommonBtnType::Normal, MStringHelper::FindLocTableText(EMLocTableType::UI, TEXT("Common_UI_Automatic")));
		ButtonAuto->DelegateCommonBtnClickedEvent.BindUObject(this, &UMClassSynthesisUI::OnClickedAutoButton);
	}

	if (ButtonClear)
	{
		ButtonClear->DelegateCommonBtnClickedEvent.BindUObject(this, &UMClassSynthesisUI::OnClockedClearButton);
	}

	if (ButtonSynthesis)
	{
		ButtonSynthesis->SetCommonBtn(EMCommonBtnType::Normal, MStringHelper::FindLocTableText(EMLocTableType::UI, TEXT("Synthesis_UI_Synthesis")));
		ButtonSynthesis->DelegateCommonBtnClickedEvent.BindUObject(this, &UMClassSynthesisUI::OnClickedSynthesisButton);
	}

	MNETMGR->OnRecvCombineAck.AddUObject(this, &UMClassSynthesisUI::RecvCombineAck);

	if (CharacterList)
	{
		CharacterList->HideRedDot(true);
		CharacterList->HideSelection(true);
		CharacterList->HideEquipMark(true);

		EMCharacterListUIType Type = EMCharacterListUIType::Class;
		if (PawnType == EMPawnType::Pet)
		{
			Type = EMCharacterListUIType::Pet;
		}
		else if (PawnType == EMPawnType::Vehicle)
		{
			Type = EMCharacterListUIType::Vehicle;
		}

		CharacterList->SetCharacterListUIType(Type);

		CharacterList->OnClickedCharacterListItem.BindUObject(this, &UMClassSynthesisUI::OnClickedCharacterListItem);

		CharacterList->SetListFilter([ ] (const int InTID)
		{
			return MNETDATAMGR->GetNetPawnHaveCount(InTID) > 1;
		});

		CharacterList->SetDimmedFilter([ this ] (const int InTID)
		{
			return (MNETDATAMGR->GetNetPawnHaveCount(InTID) - this->GetIngredientCountOfTID(InTID)) <= 1;
		});

		CharacterList->SetVisibleCharacterCount(true);

		CharacterList->HideExploringMark(true);
	}

	SynthesisSlots.Emplace(SynthesisSlot1);
	SynthesisSlots.Emplace(SynthesisSlot2);
	SynthesisSlots.Emplace(SynthesisSlot3);
	SynthesisSlots.Emplace(SynthesisSlot4);

	for (int i = 0 ; i < SynthesisSlots.Num(); i++)
	{
		FMClassSynthesisSlot SynthesisSlot;
		SynthesisSlot.Index = i;
		IngredientSlots.Emplace(SynthesisSlot);
	}

	SlotButtons.Emplace(ButtonSlot1);
	SlotButtons.Emplace(ButtonSlot2);
	SlotButtons.Emplace(ButtonSlot3);
	SlotButtons.Emplace(ButtonSlot4);

	ButtonSlot1->OnClicked.AddDynamic(this, &UMClassSynthesisUI::OnClickedSlot1);
	ButtonSlot2->OnClicked.AddDynamic(this, &UMClassSynthesisUI::OnClickedSlot2);
	ButtonSlot3->OnClicked.AddDynamic(this, &UMClassSynthesisUI::OnClickedSlot3);
	ButtonSlot4->OnClicked.AddDynamic(this, &UMClassSynthesisUI::OnClickedSlot4);

	MUIMGR->DelegateGachaAgainSynthesis.Unbind();
	MUIMGR->DelegateGachaAgainSynthesis.BindUObject(this, &UMClassSynthesisUI::OnClickedPopupGachaAgainButton);

	CreateGradeTab();
	ClearIngredient();
	SettingSlotCount();
	UpdateSlot();
	UpdateCharacterCountAll();
	UpdateSynthesisCount();
	UpdateNoIngredientText();
	UpdateSynthesisButton();
	UpdateProbability();
	UpdateCanSynthesisImage();

	if (GradeListView)
	{
		GradeListView->SetSelectedIndex(0);
	}
}

void UMClassSynthesisUI::NativeDestruct()
{
	if (ButtonAuto)
	{
		ButtonAuto->DelegateCommonBtnClickedEvent.Unbind();
	}

	if (ButtonClear)
	{
		ButtonClear->DelegateCommonBtnClickedEvent.Unbind();
	}

	if (ButtonSynthesis)
	{
		ButtonSynthesis->DelegateCommonBtnClickedEvent.Unbind();
	}

	if (CharacterList)
	{
		CharacterList->OnClickedCharacterListItem.Unbind();
		CharacterList->RemoveDimmedFilter();
		CharacterList->RemoveListFilter();
	}

	for (UButton* Button : SlotButtons)
	{
		if (Button)
		{
			Button->OnClicked.RemoveAll(this);
		}
	}

	MNETMGR->OnRecvCombineAck.RemoveAll(this);

	MUIMGR->DelegateGachaAgainSynthesis.Unbind();

	Super::NativeDestruct();
}

void UMClassSynthesisUI::ReOpenUI(ESlateVisibility InVisibility)
{
	Super::ReOpenUI(InVisibility);

	if (GradeListView)
	{
		GradeListView->SetSelectedIndex(0);
	}

	ClearIngredient();
}

void UMClassSynthesisUI::OnClickedAutoButton(EMCommonBtnType ButtonType)
{
	if (AutoPushIngredient() == false)
	{
		MUIMGR->SetGameToastMessage("347");
		return;
	}
	
	UpdateNoIngredientText();
}

void UMClassSynthesisUI::OnClockedClearButton(EMCommonBtnType ButtonType)
{
	ClearIngredient();
	UpdateNoIngredientText();
}

void UMClassSynthesisUI::OnClickedSynthesisButton(EMCommonBtnType ButtonType)
{
	if (CanSynthesis() == false)
	{
		MUIMGR->SetGameToastMessage("348");
		return;
	}

	FCombineReqT Req;
	TMap<int, int> Ingredients;
	int SynthesisCount = GetCurrentSynthesisCount();
	for (const FMClassSynthesisSlot& SynthesisSlot : IngredientSlots)
	{
		for (int i = 0; i < SynthesisSlot.Ingredients.Num() && i < SynthesisCount; i++)
		{
			int Ingredient = SynthesisSlot.Ingredients[i];
			Ingredients.FindOrAdd(Ingredient)++;
		}
	}

	for (TPair<int, int> Pair : Ingredients)
	{
		if (Pair.Value <= 0)
		{
			continue;
		}

		std::shared_ptr<MItemT> Ingredient = std::make_shared<MItemT>();
		Ingredient->itemTID = Pair.Key;
		Ingredient->itemCount = Pair.Value;
		Req.items.push_back(Ingredient);
	}

	MNETMGR->Send(Req);
}

void UMClassSynthesisUI::OnClickedCharacterListItem(int InTID)
{
	int Index = FindPossibleLeastCountSlotIndex();
	if (Index >= 0)
	{
		PushIngredient(Index, InTID);
	}
}

void UMClassSynthesisUI::OnClickedGrade(UObject* InObject)
{
	if (InObject == nullptr)
	{
		return;
	}

	if (const UMCategoryTabEntryData* EntryData = Cast<UMCategoryTabEntryData>(InObject))
	{
		if (const FMSynthesisData* Data = GetCurrentSynthesisData())
		{
			if (static_cast<int>(Data->Grade) != EntryData->Value)
			{
				ClearIngredient();
			}
		}

		if (CharacterList)
		{
			CharacterList->SetCharacterGrade(EntryData->Value);
		}

		int Key = 0;
		for (const TPair<int, const FMSynthesisData*> Pair : MDATAMGR->GetSynthesisMap())
		{
			if (Pair.Value->PawnType != PawnType)
			{
				continue;
			}

			if (Pair.Value->Grade == static_cast<EMGrade>(EntryData->Value))
			{
				Key = Pair.Key;
				break;
			}
		}
		SetSynthesisData(Key);

	}

	UpdateNoIngredientText();
}

void UMClassSynthesisUI::OnClickedSlot1()
{
	PopIngredient(0);
}

void UMClassSynthesisUI::OnClickedSlot2()
{
	PopIngredient(1);
}

void UMClassSynthesisUI::OnClickedSlot3()
{
	PopIngredient(2);
}

void UMClassSynthesisUI::OnClickedSlot4()
{
	PopIngredient(3);
}

void UMClassSynthesisUI::OnClickedPopupGachaAgainButton()
{
	AutoPushIngredient();
	OnClickedSynthesisButton(EMCommonBtnType::None);
}

void UMClassSynthesisUI::CreateGradeTab()
{
	if (GradeListView)
	{
		EMMenuIconType MenuIconType = EMMenuIconType::Hero;
		if (PawnType == EMPawnType::Pet)
		{
			MenuIconType = EMMenuIconType::Pet;
		}

		//	all.
		if (const TObjectPtr<UMCategoryTabEntryData> AllEntryData = NewObject<UMCategoryTabEntryData>(this))
		{
			AllEntryData->Value = INDEX_NONE;
			AllEntryData->Name = MStringHelper::FindLocTableText(EMLocTableType::UI, TEXT("Collection_Category_All"));
			AllEntryData->MenuIconType = MenuIconType;

			GradeListView->AddItem(AllEntryData);
		}

		//	grade.
		for (int i = static_cast<int>(EMGrade::Legendary); i >= static_cast<int>(EMGrade::Common); i--)
		{
			if (TObjectPtr<UMCategoryTabEntryData> EntryData = NewObject<UMCategoryTabEntryData>(this))
			{
				EntryData->Value = i;
				EntryData->Name = UMDataEnumString::GetGradeString(static_cast<EMGrade>(i));
				EntryData->MenuIconType = MenuIconType;

				GradeListView->AddItem(EntryData);
			}
		}
	}

	UpdateNoIngredientText();
}

void UMClassSynthesisUI::PushIngredient(const int InIndex, const int InTID)
{
	if (IngredientSlots.IsValidIndex(InIndex) == false)
	{
		return;
	}

	if (CanPushIngredient(InIndex, InTID) == false)
	{
		return;
	}

	if (GetCurrentSynthesisData() == nullptr)
	{
		if (const FMSynthesisData* SynthesisData = GetSynthesisDataFromPawn(InTID))
		{
			SetSynthesisData(SynthesisData->ID);
		}
	}

	IngredientSlots[InIndex].Ingredients.Emplace(InTID);
	CountMap.FindOrAdd(InTID)++;

	UpdateCharacterCountOfTID(InTID);
	UpdateSlot();
	UpdateSynthesisCount();
	UpdateSynthesisButton();
	UpdateCanSynthesisImage();
}

bool UMClassSynthesisUI::AutoPushIngredient()
{
	int SynthesisCount = 0;
	if (SynthesisTID <= 0)
	{
		for (const TPair<int, const FMSynthesisData*> Pair : MDATAMGR->GetSynthesisMap())
		{
			SynthesisCount = GetPossibleSynthesisCount(Pair.Key);
			if (SynthesisCount > 0)
			{
				SetSynthesisData(Pair.Key);
				break;
			}
		}
	}
	else
	{
		SynthesisCount = GetPossibleSynthesisCount(SynthesisTID);
	}

	if (SynthesisTID <= 0 || SynthesisCount <= 0)
	{
		return false;
	}

	const FMSynthesisData* SynthesisData = GetCurrentSynthesisData();
	if (SynthesisData == nullptr)
	{
		return false;
	}

	TArray<int> Pawns;
	if (CharacterList)
	{
		Pawns = CharacterList->GetItems();
	}

    for (int i = Pawns.Num() - 1; Pawns.IsValidIndex(i); i--)
	{
		int PawnTID = Pawns[i];
		const FMPawnData* PawnData = MDATAMGR->GetPawnData(PawnTID);
		if (PawnData == nullptr)
		{
			continue;
		}

		if (PawnData->Grade != SynthesisData->Grade)
		{
			continue;
		}

		while (true)
		{
			const int Index = FindPossibleLeastCountSlotIndex();
			if (IngredientSlots.IsValidIndex(Index) == false)
			{
				break;
			}

			if (CanPushIngredient(Index, PawnTID) == false)
			{
				break;
			}

			PushIngredient(Index, PawnTID);
		}

		if (IngredientSlots.IsValidIndex(FindPossibleLeastCountSlotIndex()) == false)
		{
			break;
		}
	}

	int Min = IngredientSlots[0].Ingredients.Num();
	for (int i = 1; i < SynthesisData->MaterialCount; i++)
	{
		if (Min > IngredientSlots[i].Ingredients.Num())
		{
			Min = IngredientSlots[i].Ingredients.Num();
		}
	}

	for (int i = 0 ; i < SynthesisData->MaterialCount; i++)
	{
		while (IngredientSlots[i].Ingredients.Num() > Min)
		{
			PopIngredient(i);
		}
	}

	UpdateCharacterCountAll();
	UpdateSlot();
	UpdateSynthesisCount();
	UpdateSynthesisButton();
	UpdateCanSynthesisImage();

	return true;
}

int UMClassSynthesisUI::PopIngredient(const int InIndex)
{
	if (IngredientSlots.IsValidIndex(InIndex) == false)
	{
		return 0;
	}

	if (IngredientSlots[InIndex].Ingredients.Num() <= 0)
	{
		return 0;
	}

	int Index = IngredientSlots[InIndex].Ingredients.Num() - 1;
	int TID = IngredientSlots[InIndex].Ingredients[Index];
	IngredientSlots[InIndex].Ingredients.RemoveAt(Index);
	CountMap[TID]--;

	UpdateCharacterCountOfTID(TID);
	SettingSlotCount();
	UpdateSlot();
	UpdateSynthesisCount();
	UpdateSynthesisButton();
	UpdateCanSynthesisImage();

	return TID;
}

void UMClassSynthesisUI::ClearIngredient()
{
	CountMap.Empty();

	for (FMClassSynthesisSlot& SynthesisSlot : IngredientSlots)
	{
		SynthesisSlot.Ingredients.Reset();
	}

	UpdateCharacterCountAll();
	SettingSlotCount();
	UpdateSlot();
	UpdateSynthesisCount();
	UpdateSynthesisButton();
	UpdateCanSynthesisImage();
}

bool UMClassSynthesisUI::CanPushIngredient(const int InIndex, const int InTID)
{
	if (IngredientSlots.IsValidIndex(InIndex) == false)
	{
		return false;
	}

	if (IsSlotFull(InIndex))
	{
		return false;
	}

	if (const FMSynthesisData* SynthesisData = GetCurrentSynthesisData())
	{
		const FMPawnData* PawnData = MDATAMGR->GetPawnData(InTID);
		if (PawnData == nullptr)
		{
			return false;
		}

		if (PawnData->PawnType != SynthesisData->PawnType ||
			PawnData->Grade != SynthesisData->Grade)
		{
			return false;
		}

		const int InsertedCount = CountMap.FindOrAdd(InTID);
		return MNETDATAMGR->GetNetPawnHaveCount(InTID) - InsertedCount > 1;
	}

	if (GetSynthesisDataFromPawn(InTID))
	{
		return MNETDATAMGR->GetNetPawnHaveCount(InTID) > 1;
	}

	return false;
}

int UMClassSynthesisUI::GetPossibleSynthesisCount(const int InSynthesisTID) const
{
	const FMSynthesisData* SynthesisData = MDATAMGR->GetSynthesisData(InSynthesisTID);
	if (SynthesisData == nullptr)
	{
		return 0;
	}

	if (PawnType != SynthesisData->PawnType)
	{
		return 0;
	}

	const EMGrade Grade = SynthesisData->Grade;

	int Count = 0;
	for (const int TID : MNETDATAMGR->GetHaveNetPawnArr(SynthesisData->PawnType))
	{
		const FMPawnData* PawnData = MDATAMGR->GetPawnData(TID);
		if (PawnData == nullptr)
		{
			continue;
		}
		if (Grade != PawnData->Grade)
		{
			continue;
		}

		if (const int PawnCount = MNETDATAMGR->GetNetPawnHaveCount(TID); PawnCount > 1)
		{
			Count += PawnCount - 1;
		}
	}

	return FMath::Min(Count / SynthesisData->MaterialCount, MAX_SYNTHESIS_COUNT);
}

bool UMClassSynthesisUI::IsSlotFull(const int InIndex) const
{
	if (IngredientSlots.IsValidIndex(InIndex) == false)
	{
		return false;
	}

	return IngredientSlots[InIndex].Ingredients.Num() >= MAX_SYNTHESIS_COUNT;
}

int UMClassSynthesisUI::GetIngredientCountOfTID(const int InTID) const
{
	if (CountMap.Find(InTID))
	{
		return CountMap[InTID];
	}
	return 0;
}

int UMClassSynthesisUI::GetIngredientCount(const int InIndex) const
{
	if (IngredientSlots.IsValidIndex(InIndex))
	{
		return IngredientSlots[InIndex].Ingredients.Num();
	}
	return -1;
}

int UMClassSynthesisUI::GetSlotCount() const
{
	if (const FMSynthesisData* SynthesisData = GetCurrentSynthesisData())
	{
		return SynthesisData->MaterialCount;
	}

	return 4;	// 아무런 재료 투입이 안되어있으면 4개 슬롯 표시
}

int UMClassSynthesisUI::FindPossibleLeastCountSlotIndex() const
{
	int Index = -1;
	int MinCount = MAX_SYNTHESIS_COUNT;

	for (int i = 0 ; i < GetSlotCount(); i++)
	{
		if (IngredientSlots[i].Ingredients.Num() < MinCount)
		{
			Index = IngredientSlots[i].Index;
			MinCount = IngredientSlots[i].Ingredients.Num();
		}
	}

	return Index;
}

int UMClassSynthesisUI::GetCurrentSynthesisCount() const
{
	int MinCount = MAX_SYNTHESIS_COUNT;
	for (int i = 0 ; i < GetSlotCount(); i++)
	{
		if (IngredientSlots.IsValidIndex(i) == false)
		{
			continue;
		}

		if (IngredientSlots[i].Ingredients.Num() < MinCount)
		{
			MinCount = IngredientSlots[i].Ingredients.Num();
		}
	}

	return MinCount;
}

void UMClassSynthesisUI::SettingSlotCount()
{
	int SlotCount = GetSlotCount();

	for (int i = 0; i < SynthesisSlots.Num(); i++)
	{
		if (SynthesisSlots[i])
		{
			SynthesisSlots[i]->SetVisibility(i < SlotCount ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}

	UpdateSlot();
}

void UMClassSynthesisUI::UpdateSlot()
{
	for (int i = 0; i < GetSlotCount(); i++)
	{
		if (IngredientSlots.IsValidIndex(i) == false)
		{
			continue;
		}

		int Count = IngredientSlots[i].Ingredients.Num();
		int Ingredient = Count > 0 ? IngredientSlots[i].Ingredients[0] : 0;

		UMPawnIconUI* SynthesisSlot = SynthesisSlots[i];
		if (SynthesisSlot)
		{
			if (Ingredient <= 0)
			{
				SynthesisSlot->EmptyIcon();
			}
			else
			{
				SynthesisSlot->SetItem(Ingredient, false, false, true, Count, false);
			}
		}
	}
}

void UMClassSynthesisUI::UpdateCharacterCountAll()
{
	if (CharacterList)
	{
		for (const int TID : CharacterList->GetItems())
		{
			UpdateCharacterCountOfTID(TID);
		}
	}
}

void UMClassSynthesisUI::UpdateCharacterCountOfTID(const int InTID)
{
	if (CharacterList)
	{
		int Count = MNETDATAMGR->GetNetPawnHaveCount(InTID) - GetIngredientCountOfTID(InTID) - 1;
		CharacterList->SetCharacterCount(InTID, Count);
	}
}

void UMClassSynthesisUI::UpdateSynthesisCount()
{
	if (CountText)
	{
		int Count = GetCurrentSynthesisCount();
		CountText->SetText(FText::AsNumber(Count));
	}
}

void UMClassSynthesisUI::UpdateNoIngredientText()
{
	if (NoIngredientText)
	{
		NoIngredientText->SetVisibility(CharacterList->GetAddedItemCount() > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
}

void UMClassSynthesisUI::UpdateProbability()
{
	if (ProbabilityText)
	{
		const bool bIsActive = GetCurrentSynthesisData() != nullptr;

		ProbabilityText->SetVisibility(bIsActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

		if (bIsActive)
		{
			const float Probability = GetCurrentSynthesisData()->UpgradeProb / 100.0f;
			FText Text = FText::Format(MStringHelper::FindLocTableText(EMLocTableType::UI, TEXT("Synthesis_UI_SynthesisProb")), Probability);
			ProbabilityText->SetText(Text);
		}
	}
}

void UMClassSynthesisUI::UpdateSynthesisButton()
{
	if (SynthesisButtonCover)
	{
		SynthesisButtonCover->SetVisibility(CanSynthesis() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UMClassSynthesisUI::UpdateCanSynthesisImage()
{
	if (ImageCanSynthesis)
	{
		ImageCanSynthesis->SetVisibility(CanSynthesis() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ImageCanNotSynthesis)
	{
		ImageCanNotSynthesis->SetVisibility(!CanSynthesis() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

bool UMClassSynthesisUI::CanSynthesis()
{
	return GetCurrentSynthesisCount() > 0;
}

const FMSynthesisData* UMClassSynthesisUI::GetSynthesisDataFromPawn(const int InTID) const
{
	if (const FMPawnData* PawnData = MDATAMGR->GetPawnData(InTID))
	{
		for (const TPair<int, const FMSynthesisData*> Pair : MDATAMGR->GetSynthesisMap())
		{
			if (Pair.Value->PawnType == PawnData->PawnType &&
				Pair.Value->Grade == PawnData->Grade)
			{
				return Pair.Value;
			}
		}
	}

	return nullptr;
}

const FMSynthesisData* UMClassSynthesisUI::GetCurrentSynthesisData() const
{
	if (SynthesisTID > 0)
	{
		return MDATAMGR->GetSynthesisData(SynthesisTID);
	}
	return nullptr;
}

void UMClassSynthesisUI::SetSynthesisData(const int InTID)
{
	if (const FMSynthesisData* Data = MDATAMGR->GetSynthesisData(InTID))
	{
		SynthesisTID = InTID;

		int i;
		// 합성 데이터가 정해지면 해당 등급 탭으로 강제이동
		for (i = 1; i < GradeListView->GetNumItems(); i++)
		{
			if (UMCategoryTabEntryData* Entry = Cast<UMCategoryTabEntryData>(GradeListView->GetItemAt(i)))
			{
				EMGrade Grade = static_cast<EMGrade>(Entry->Value);
				if (Grade == Data->Grade &&
					GradeListView->GetSelectedItem<UMCategoryTabEntryData>() != Entry)
				{
					break;
				}
			}
		}

		if (i < GradeListView->GetNumItems())
		{
			GradeListView->SetSelectedIndex(i);
		}
	}
	else
	{
		SynthesisTID = 0;
	}

	SettingSlotCount();
	UpdateProbability();
	UpdateSynthesisButton();
}

void UMClassSynthesisUI::RecvCombineAck(FCombineAckT* InPacket)
{
	if (InPacket == nullptr)
	{
		return;
	}

	if (InPacket->result != EResultID::R_SUCCESS)
	{
		return;
	}

	int PrevTID = SynthesisTID;

	ClearIngredient();

	EMRewardItemType RewardType = EMRewardItemType::None;
	TMap<int32, int> RewardMap;

	for (std::shared_ptr<MRewardItemT> Reward : InPacket->rewards)
	{
		if (Reward == nullptr)
		{
			continue;
		}

		int& Count = RewardMap.FindOrAdd(Reward->itemtid);
		Count += Reward->value;
	}

	if (PawnType == EMPawnType::Hero)
	{
		RewardType = EMRewardItemType::Hero;
	}
	else if (PawnType == EMPawnType::Pet)
	{
		RewardType = EMRewardItemType::Pet;
	}

	if (RewardType != EMRewardItemType::None)
	{
		MUIMGR->GachaUIOpen(EMGachaType::Synthesis, RewardType, RewardMap, 0);
		if (UMGachaUI* UI = Cast<UMGachaUI>(MUIMGR->GetOpenUI(TEXT("GachaUI"))))
		{
			UI->SetTryAgainCount(GetPossibleSynthesisCount(PrevTID));
		}
	}

	if (UMClassTabUI* Menubar = Slot->GetTypedOuter<UMClassTabUI>())
	{
		Menubar->UpdateRedDot();
	}
}

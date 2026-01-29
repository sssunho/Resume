/***********************************************************************************
 *                                                                                 *
 * Copyright. (C) 2023 Mobirix.                                                    *
 * Website: https://mobirix.com													   *
 *                                                                                 *
 ***********************************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "UI/Common/MBaseUIWidget.h"
#include "MClassSynthesisUI.generated.h"

enum class EMCommonBtnType : uint8;
class UMCommonBtn;
class UImage;
enum class EMPawnType : uint8;
class UListView;
class UMCharacterListUI;
struct FMSynthesisData;
struct FCombineAckT;
class UButton;
class UTextBlock;
class UMPawnIconUI;

USTRUCT()
struct FMClassSynthesisSlot
{
	GENERATED_BODY()

	int Index;

	TArray<int> Ingredients;

};

UCLASS()
class MRPG_API UMClassSynthesisUI : public UMBaseUIWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;;

	virtual void ReOpenUI(ESlateVisibility InVisibility) override;

private:
	UFUNCTION()
	void OnClickedAutoButton(EMCommonBtnType ButtonType);

	UFUNCTION()
	void OnClockedClearButton(EMCommonBtnType ButtonType);

	UFUNCTION()
	void OnClickedSynthesisButton(EMCommonBtnType ButtonType);

	UFUNCTION()
	void OnClickedCharacterListItem(int InTID);

	UFUNCTION()
	void OnClickedGrade(UObject* InObject);

	// 버튼 배열에 바인드할 마땅한 방법이 없다.
	UFUNCTION()
	void OnClickedSlot1();

	UFUNCTION()
	void OnClickedSlot2();

	UFUNCTION()
	void OnClickedSlot3();

	UFUNCTION()
	void OnClickedSlot4();

	UFUNCTION()
	void OnClickedPopupGachaAgainButton();

	void CreateGradeTab();

	void PushIngredient(int InIndex, const int InTID);

	bool AutoPushIngredient();

	int PopIngredient(int InIndex);

	void ClearIngredient();

	bool CanPushIngredient(int InIndex, const int InTID);

	int GetPossibleSynthesisCount(const int InSynthesisTID) const;

	bool IsSlotFull(int InIndex) const;

	int GetIngredientCountOfTID(const int InTID) const;

	int GetIngredientCount(int InIndex) const;

	int GetSlotCount() const;

	int FindPossibleLeastCountSlotIndex() const;

	int GetCurrentSynthesisCount() const;

	void SettingSlotCount();

	void UpdateSlot();

	void UpdateCharacterCountAll();

	void UpdateCharacterCountOfTID(const int InTID);

	void UpdateSynthesisCount();

	void UpdateNoIngredientText();

	void UpdateProbability();

	void UpdateSynthesisButton();

	void UpdateCanSynthesisImage();

	bool CanSynthesis();

	const FMSynthesisData* GetSynthesisDataFromPawn(const int InTID) const;
	const FMSynthesisData* GetCurrentSynthesisData() const;
	void SetSynthesisData(const int InTID);

	void RecvCombineAck(FCombineAckT* InPacket);


protected:

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UMCharacterListUI> CharacterList;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UListView> GradeListView;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UMPawnIconUI> SynthesisSlot1;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UMPawnIconUI> SynthesisSlot2;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UMPawnIconUI> SynthesisSlot3;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UMPawnIconUI> SynthesisSlot4;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ButtonSlot1;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ButtonSlot2;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ButtonSlot3;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ButtonSlot4;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProbabilityText;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NoIngredientText;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UMCommonBtn> ButtonAuto;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UMCommonBtn> ButtonClear;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UMCommonBtn> ButtonSynthesis;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SynthesisButtonCover;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UImage> ImageCanSynthesis;

	UPROPERTY(Category = UI, BlueprintReadWrite, EditAnywhere, meta = (BindWidgetOptional))
	TObjectPtr<UImage> ImageCanNotSynthesis;

private:
	UPROPERTY()
	TArray<FMClassSynthesisSlot> IngredientSlots;

	UPROPERTY()
	TMap<int, int> CountMap;

	UPROPERTY()
	TArray<UMPawnIconUI*> SynthesisSlots;

	UPROPERTY()
	TArray<UButton*> SlotButtons;

	int SynthesisTID;

	EMPawnType PawnType;
};

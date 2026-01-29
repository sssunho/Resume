using System;
using System.Linq;
using System.Numerics;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using TMPro;

[Serializable]
public class UI_Costume_ComposePart
{
    public GameObject MainPanel;
    public GameObject ListPanel;
    public UI_Costume_IngredientList List;
    public UI_Costume_Compose_Slot[] Slots;

    public TextMeshProUGUI ComposeCountText;
    public TextMeshProUGUI ResultRateText;
    public TextMeshProUGUI ResultCountText;
    public Image ResultGradeImage;

    public void SetActive(bool value)
    {
        MainPanel.SetActive(value);
        ListPanel.SetActive(value);
    }
}

public class UI_Costume_Compose : UIMenuBarBase
{
    enum ButtonID
    {
        None, Normal, Special, NormalSlot, SpecialSlot, Compose, Cancel, Auto, Prev, Next
    }

    [SerializeField] UI_Costume_ComposePart[] _composeUIs = new UI_Costume_ComposePart[3];
    [SerializeField] UI_Button _button_Compose;
    [SerializeField] GameObject _go_AutoButton;
    [SerializeField] UI_TabGroup _tabs;
    [SerializeField] GameObject[] _go_composeEffecs;
    [SerializeField] Sprite[] _composeGradeSprites;
    [SerializeField] Sprite[] _composeGradeTextSprites;

    [Header("Special Compose")]
    [SerializeField] GameObject _specialSlotBackground;

    [SerializeField] TextMeshProUGUI _text_ComposeLevel;
    [SerializeField] TextMeshProUGUI _text_SpecialIngredientName;
    [SerializeField] TextMeshProUGUI _text_SpecialIngredientCount;
    [SerializeField] GameObject _go_SpecialSlotInfoPanel;

    bool _isResultPlaying;

    EComposeType _composeType = EComposeType.Normal;
    UI_Costume_ComposePart CurrentUI => _composeUIs[(int)_composeType];

    CostumeComposer Composer => PlayerCostumeManager.Instance.GetComposer(_composeType);

    public override void InitUI()
    {
        base.InitUI();
        _composeUIs[(int)EComposeType.Normal].List.OnUpdateElement -= UpdateNormalElement;
        _composeUIs[(int)EComposeType.Normal].List.OnUpdateElement += UpdateNormalElement;
        _composeUIs[(int)EComposeType.Normal].List.OnClickElement -= OnClickNormalItemCell;
        _composeUIs[(int)EComposeType.Normal].List.OnClickElement += OnClickNormalItemCell;
        _composeUIs[(int)EComposeType.Special].List.OnUpdateElement -= UpdateSpecialElement;
        _composeUIs[(int)EComposeType.Special].List.OnUpdateElement += UpdateSpecialElement;
        _composeUIs[(int)EComposeType.Special].List.OnClickElement -= OnClickSpecialItemCell;
        _composeUIs[(int)EComposeType.Special].List.OnClickElement += OnClickSpecialItemCell;
        _tabs.Init();
        _tabs.SetSelectButton(0);
        _tabs.Buttons.ForEach(p =>
        {
            var id = EnumHelper.Parse(p.ID, ButtonID.None);
            switch (id)
            {
                case ButtonID.Normal:
                    p.GetComponentInChildren<RedDot_CostumeCollectionNewTab>().Setting(PlayerCostumeManager.Instance.GetComposer(EComposeType.Normal));
                    break;
                case ButtonID.Special:
                    p.GetComponentInChildren<RedDot_CostumeCollectionNewTab>().Setting(PlayerCostumeManager.Instance.GetComposer(EComposeType.Special));
                    break;
            }
        });

        int index = 0;
        Array.ForEach(_composeUIs[(int)EComposeType.Normal].Slots, p => p.SetSlot(index++));
        _composeUIs[(int)EComposeType.Special].Slots[0].SetSlot(0);
    }

    public override void OutUI(bool isDirect = false)
    {
        PlayerCostumeManager.Instance.GetComposer(EComposeType.Normal).Clear();
        PlayerCostumeManager.Instance.GetComposer(EComposeType.Special).Clear();
        base.OutUI(isDirect);
    }

    public override bool OnBackKey()
    {
        if (_isResultPlaying) return false;
        return base.OnBackKey();
    }

    #region Button events

    public override void EventButton(UIButtonInfo buttonInfo)
    {
        if (buttonInfo.State == UI_ButtonState.Click)
        {
            OnClickButton(buttonInfo);
        }
    }

    void OnClickButton(UIButtonInfo buttoninfo)
    {
        var id = EnumHelper.Parse(buttoninfo.ID, ButtonID.None);
        switch(id)
        {
            case ButtonID.Normal:
                OnClickNormalTab();
                break;

            case ButtonID.Special:
                OnClickSpecialTab();
                break;

            case ButtonID.NormalSlot:
                OnClickNormalSlot(buttoninfo);
                break;

            case ButtonID.SpecialSlot:
                OnClickSpecialSlot();
                break;

            case ButtonID.Compose:
                OnClickCompose();
                break;

            case ButtonID.Cancel:
                OnClickCancel();
                break;

            case ButtonID.Auto:
                OnClickAuto();
                break;

            case ButtonID.Next:
                OnClickNext();
                break;

            case ButtonID.Prev:
                OnClickPrev();
                break;
        }
    }

    void OnClickNormalTab()
    {
        Composer.Clear();
        ChangeComposeType(EComposeType.Normal);
    }

    void OnClickSpecialTab()
    {
        Composer.Clear();
        ChangeComposeType(EComposeType.Special);
    }
    
    void OnClickNormalSlot(UIButtonInfo buttoninfo)
    {
        string str = (buttoninfo as MonoBehaviour).name.Split('_')[1];
        if (int.TryParse(str, out int id) == false) return;

        id -= 1;
        UnsetSlot(id);
        CurrentUI.List.Refresh();
    }

    void OnClickSpecialSlot()
    {
        Composer.PopSlot(0);
        UnsetSlot(0);
        OnRefreshUI();
    }

    void OnClickCompose()
    {
        Compose();
    }

    void OnClickCancel()
    {
        Composer.Clear();
        OnRefreshUI();
    }

    void OnClickPrev()
    {
        var scomposer = Composer as CostumeComposer_Special;
        int level = scomposer.GetComposeLevel();
        scomposer.SetComposeLevel(level - 1);
        OnRefreshUI();
    }

    void OnClickNext()
    {
        var scomposer = Composer as CostumeComposer_Special;
        int level = scomposer.GetComposeLevel();
        scomposer.SetComposeLevel(level + 1);
        OnRefreshUI();
    }

    void OnClickSpecialItemCell(ItemIcon element)
    {
        SetSlot(element.ItemInfo, 0);
        OnRefreshUI();
    }

    void OnClickNormalItemCell(ItemIcon element)
    {
        var min = Enumerable.Range(0, Composer.SlotCount).Min(i => Composer.GetSlotSize(i));

        for (int i = 0; i < Composer.SlotCount; i++)
        {
            if (Composer.GetSlotSize(i) > min) continue;
            SetSlot(element.ItemInfo, i);
            return;
        }
    }

    void OnClickAuto()
    {
        Composer.AutoFillSlots();
        OnRefreshUI();
    }

    #endregion

    void ChangeComposeType(EComposeType mode)
    {
        if (_composeType != EComposeType.NONE)
        {
            Composer.Clear();
        }
        _composeType = mode;
        OnRefreshUI();
    }

    protected override void OnPrvOpenWork(Action<bool> resultCallback)
    {
        resultCallback?.Invoke(true);
    }

    protected override void RefreshUI(LanguageType langCode)
    {
        UpdateMode();
        UpdateSlotUIs();
        UpdateCanCompose();
        UpdateResultSlot();
        UpdateCountBackground();
        UpdateSpecialComposeSlot();
        UpdateComposeLevel();
    }

    void UpdateMode()
    {
        for(int i = 1; i < _composeUIs.Length; i++)
        {
            _composeUIs[i].SetActive(i == (int)_composeType);
        }
        _go_AutoButton.SetActive(_composeType == EComposeType.Normal);

        InitItemList();
    }

    void UpdateSlotUIs()
    {
        for(int i = 0; i < Composer.SlotCount; i++)
        {
            UpdateSlotUI(i);
        }
    }

    void UpdateSlotUI(int index)
    {
        if (index >= Composer.SlotCount) return;
        CurrentUI.Slots[index].Refresh();
    }

    void UpdateCanCompose()
    {
        _button_Compose.interactable = Composer.CanCompose();
        if(_composeType == EComposeType.Special)
        {
            _specialSlotBackground.SetActive(_button_Compose.interactable);            
        }
    }

    void InitItemList()
    {
        var ui = _composeUIs[(int)_composeType];
        var list = _composeType == EComposeType.Normal  ? PlayerCostumeManager.Instance.Items.Select(p => p as BaseItem).Where(i => DataManager.Instance.DicCostumeNormalCompose.ContainsKey(i.itemData.Grade) && i.Count > 1).ToList():
                   _composeType == EComposeType.Special ? DataManager.Instance.DicCostumeSpecialCompose.Keys.Select(p => InventoryManager.Instance.GetItem(p)).Where(i => i != null).ToList() : null;
        if(null != list) ui.List.SetLists(list, false);
    }

    void SetSlot(BaseItem item, int slot)
    {
        Composer.PushSlot(slot, item.Id);
        UpdateSlotUIs();
        UpdateCanCompose();
        UpdateResultSlot();
        UpdateCountBackground();
    }

    void UnsetSlot(int slot)
    {
        Composer.PopSlot(slot);
        UpdateSlotUIs();
        UpdateCanCompose();
        UpdateResultSlot();
        UpdateCountBackground();
    }

    int _composeCount = 0;
    void Compose()
    {
        if (!Composer.CanCompose())
        {
            if (Composer is CostumeComposer_Normal)
            {
                UIManager.ShowToastMessage(LocalizeManager.GetText("Msg_Btn_Shortage_Costume"));
            }
            else
            {
                UIManager.ShowToastMessage(LocalizeManager.GetText("Msg_Btn_ItemShortage"));
            }
            return;
        }

        var result = Composer.Compose();
        var res = OpenRewardBox(result);
        var nextGrade = result.Costs.First().Item.itemData.Grade + 1;
        if (CostumeConfirmManager.Instance.CanAddConfirmInfo(nextGrade) && CostumeConfirmManager.Instance.IsMaxCount)
        {
            UIManager.ShowToastMessage(LocalizeManager.GetText("Msg_CostumeConfirmNotAddList"));
            return;
        }

        foreach (var reward in res)
        {
            if (CostumeConfirmManager.Instance.NewConfirmInfo(reward.ItemID, (int)reward.ItemCount) == false)
            {
                InventoryManager.Instance.GetItem(reward.ItemID).Count += (BigInteger)reward.ItemCount;
            }
        }
        foreach (var cost in result.Costs)
        {
            cost.Item.Count -= cost.Count;
        }
        result.Rewards = res;

        _composeCount++;
        if (_composeCount % 12 == 0)
        {
            UIRoot.Instance.LoadingNetwork.Set("CostumeCompose");
            UserDataManager.Save_ServerData("CostumeCompose", null, null, error =>
              {
                  OnComposeResultSyncCompleted(result);
                  UIRoot.Instance.LoadingNetwork.Set("CostumeCompose", false);
              });
        }
        else
        {
            UserDataManager.Save_LocalData();
            OnComposeResultSyncCompleted(result);
        }
    }

    List<RewardItemInfo> OpenRewardBox(CostumeComposeResult result)
    {
        var res = new List<RewardItemInfo>();
        foreach (var reward in result.Rewards)
        {
            res.AddRange(reward.GetRewardItemsInBox());
        }
        res.RemoveAll(p => p.ItemID == 0);
        return res;
    }

    void UpdateNormalElement(ItemIcon element)
    {
        var count = (Composer as CostumeComposer_Normal).GetIngredientCount(element.ItemInfo.Id);
        count = element.ItemInfo.Count - count - 1;
        if (count < 0) count = 0;
        element.Setting(new RewardItemInfo { ItemID = element.ItemInfo.Id, ItemCount = count });
        element.SetActiveCover(Composer.ValidateIngredient(element.ItemInfo.Id) == false);
    }

    void UpdateSpecialElement(ItemIcon element)
    {
        element.SetActiveParts(ItemIcon.PartFlag.Stars | ItemIcon.PartFlag.Grade, false);
        element.SetActiveParts(ItemIcon.PartFlag.Cover, element.ItemInfo.Count <= 0);
        element.SetActiveParts(ItemIcon.PartFlag.Frame, element.ItemInfo == PlayerCostumeManager.Instance.GetComposer(EComposeType.Special).PeekSlot(0));
        element.GetComponent<UI_Button>().interactable = element.ItemInfo.Count > 0;
    }

    void UpdateComposeLevel()
    {
        if (_composeType != EComposeType.Special) return;
        var scomposer = Composer as CostumeComposer_Special;
        _text_ComposeLevel.text = (scomposer.GetComposeLevel() + 1).ToString();
        UpdateCanCompose();
    }

    void UpdateResultSlot()
    {
        var grade = Composer.GetGrade();
        var rate = Composer.GetRate();
        CurrentUI.ResultRateText.text = rate > 0 ? (rate / (float)100).ToString() + '%' : "? %";
        CurrentUI.ResultGradeImage.sprite = _composeGradeTextSprites[(int)grade];
        CurrentUI.ResultGradeImage.enabled = _composeGradeTextSprites[(int)grade] != null;
        CurrentUI.ComposeCountText.text = Composer.GetComposeCount().ToString();
    }

    void UpdateSpecialComposeSlot()
    {
        _go_SpecialSlotInfoPanel.SetActive(PlayerCostumeManager.Instance.GetComposer(EComposeType.Special).PeekSlot(0) != null);
        if (_composeType != EComposeType.Special) return;
        var slot = Composer.PeekSlot(0);
        if (null == slot) return;
        _text_SpecialIngredientName.text = LocalizeManager.GetText(slot.itemData.Nameid);
        _text_SpecialIngredientCount.text = $"{slot.Count}/{Composer.GetCurrentComposeData().Needcount}";
    }

    void OnComposeResultSyncCompleted(CostumeComposeResult result)
    {
        UIManager.Instance.OpenUI(EUIName.UI_Gacha_GetCostume, new()
        {
            prvOpenUIWork = (ui, callback)=>
            {
                (ui as UI_Gacha_GetItem).Setting(UI_Gacha_GetItem.EUIType.CostumeCompose, result.Rewards);
                callback?.Invoke(true);
            }                
        });                        

        OnRefreshUI();
    }

    void UpdateCountBackground()
    {
        if (_composeType == EComposeType.Special)
        {
            //_go_specialComposeEffect.SetActive(Composer.GetSlot(0).Item != null);
        }
        else
        {
            int count = 0;
            for (int i = 0; i < Composer.SlotCount; i++)
                if (Composer.PeekSlot(i) != null) count++;
            for (int i = 0; i < _go_composeEffecs.Length; i++)
            {
                var image = _go_composeEffecs[i];
                if (image != null) image.SetActive(i < count);
            }
        }
    }

}

// MQ2GroupInfo.cpp : Defines the entry point for the DLL application.
//

// TODO:  Break the specific hotbuttons out into commands that can be used independently

#include <mq/Plugin.h>
#include "resource.h"

PreSetup("MQ2GroupInfo");
PLUGIN_VERSION(0.1);

enum TI_MenuCommands
{
	TIMC_MakeMeLeader = 54,
	TIMC_NavToMe = 55,
	TIMC_Runto = 56,
	TIMC_PickupGroundItem = 57,
	TIMC_ClickNearestDoor = 58,
	TIMC_SwitchTo = 59,
	TIMC_FollowMe = 60,
	//show/hide
	TIMC_ComeToMeButton = 61,
	TIMC_MimicMeButton = 62,
	TIMC_FollowMeButton = 63,
	TIMC_HotButtons = 64,
	TIMC_Distance = 65,
};

enum class eINIOptions
{
	WriteOnly,
	ReadAndWrite,
	ReadOnly
};

CButtonWnd* FollowMeButton = nullptr;
CButtonWnd* MimicMeButton = nullptr;
CButtonWnd* ComeToMeButton = nullptr;

CHotButton* GroupHotButton[3] = { nullptr, nullptr, nullptr };

bool Initialized = false;
bool gBUsePerCharSettings = false;
bool gBShowDistance = true;
bool gbFollowme = false;
bool gbMimicMe = false;
bool gBShowMimicMeButton = true;
bool gBShowComeToMeButton = true;
bool gBShowFollowMeButton = true;
bool gBShowHotButtons = true;
bool bDisablePluginDueToBadUI = false;
bool gbDisableNetCommandChecks = false;

int separatorId = 0;
int separatorid2 = 0;

// All of the below are also defaulted in the ini
std::string DistanceLabelToolTip = "Member Distance";
std::string ComeToMeLabel = "Come To Me";
std::string ComeToMeCommand = "/dgge /multiline ; /afollow off;/nav stop;/timed 5 /nav id ${Me.ID}";
std::string ComeToMeToolTip = ComeToMeCommand;
std::string NavStopCommand = "/dgge /nav stop";
std::string FollowMeLabel = "Follow Me";
std::string FollowMeCommand = "/dgge /multiline ; /afollow off;/nav stop;/timed 5 /afollow spawn ${Me.ID}";
std::string FollowMeToolTip = FollowMeCommand;
std::string FollowStopCommand = "/dgge /multiline ; /afollow off;/nav stop";
std::string MimicMeLabel = "Mimic Me";
std::string MimicMeToolTip = "Everyone do what I do: targeting, hailing, etc.";
std::string MimicMeSayCommand = "/dgge /say";
std::string MimicMeHailCommand = "/dgge /keypress HAIL";
std::string TargetCommand = "/dgge /target id";

// TODO: This should probably be a map of UI settings
/* Also defaulted in HandleINI() for INI reads and in the .ini resource */
bool gbDynamicUI = true; // see note above
bool gbUseGroupLayoutBox = false; // see note above
int gGroupDistanceFontSize = 2; // see note above
int gGroupDistanceOffset = 2; // see note above
std::string BaseLabelName = "Player_ManaLabel"; // see note above
std::string GroupDistanceElementPrefix = "Needs to be set when UseGroupLayoutBox is enabled"; // see note above
std::string GroupDistanceLoc = "0,-20,70,0"; // see note above
std::string ComeToMeLoc = "61,27,6,46"; // see note above
std::string FollowMeLoc = "61,27,48,88"; // see note above
std::string MimicMeLoc = "61,27,90,130"; // see note above
std::string HotButton0Loc = "97,64,6,46"; // see note above
std::string HotButton1Loc = "97,64,49,89"; // see note above
std::string HotButton2Loc = "97,64,92,132"; // see note above
/* End multiple location defaults */

int gRightClickIndex = -1;

int mmlmenuid = 0;
int navmenuid = 0;
int groundmenuid = 0;
int doormenuid = 0;
int switchtomenuid = 0;
int followmenuid = 0;
int gotomenuid = 0;

int cometomeoptionmenuid = 0;
int mimicmeoptionmenuid = 0;
int followmeoptionmenuid = 0;
int hotoptionmenuid = 0;
int distanceoptionmenuid = 0;

std::map<DWORD, bool> FollowMeMap;

constexpr int NUM_GROUPWND_CONTROLS = 5;
CGaugeWnd* GW_Gauges[NUM_GROUPWND_CONTROLS] = { nullptr };
CLabelWnd* GroupDistLabels[NUM_GROUPWND_CONTROLS] = { nullptr };

DWORD LastTargetID = 0;
DWORD orgwstyle = 0;

char szMMainTip[128] = { "MQ2GroupInfo is Active: Type /groupinfo help or rightclick this window to see a menu" };

CLabelWnd* CreateDistLabel(CXWnd* parent, CControlTemplate* DistLabelTemplate, const CXStr& label,
	int font, const CXRect& rect, bool bAlignRight, bool bShow)
{
	uint32_t oldfont = DistLabelTemplate->nFont;
	uint32_t oldstyle = DistLabelTemplate->uStyleBits;
	CXStr oldName = DistLabelTemplate->strName;
	CXStr oldScreenId = DistLabelTemplate->strScreenId;

	DistLabelTemplate->nFont = font;
	DistLabelTemplate->uStyleBits = WSF_AUTOSTRETCHH | WSF_AUTOSTRETCHV | WSF_RELATIVERECT;
	DistLabelTemplate->strName = label;
	DistLabelTemplate->strScreenId = label;

	CLabelWnd* pLabel = (CLabelWnd*)pSidlMgr->CreateXWndFromTemplate(parent, DistLabelTemplate);
	if (pLabel)
	{
		pLabel->SetTopOffset(rect.top);
		pLabel->SetBottomOffset(rect.bottom);
		pLabel->SetLeftOffset(rect.left);
		pLabel->SetRightOffset(rect.right);
		pLabel->SetCRNormal(MQColor(0, 255, 0)); // green
		pLabel->SetBGColor(MQColor(255, 255, 255));
		pLabel->SetTooltip(DistanceLabelToolTip.c_str());
		pLabel->SetVisible(bShow);
		pLabel->bNoWrap = true;
		pLabel->SetLeftAnchoredToLeft(true);
		pLabel->bAlignRight = bAlignRight;
		pLabel->bAlignCenter = false;
	}

	DistLabelTemplate->uStyleBits = oldstyle;
	DistLabelTemplate->nFont = oldfont;
	DistLabelTemplate->strName = oldName;
	DistLabelTemplate->strScreenId = oldScreenId;

	return pLabel;
}

CButtonWnd* CreateAButton(CGroupWnd* pGwnd, CControlTemplate* Template,
	const char* label, const char* labelscreen, int fontsize, const CXRect& rect,
	COLORREF color, COLORREF bgcolor, const char* tooltip, const char* text, bool bShow)
{
	uint32_t oldFont = Template->nFont;
	CXStr oldName = Template->strName;
	CXStr oldScreenID = Template->strScreenId;

	Template->nFont = 1;
	Template->strName = label;
	Template->strScreenId = labelscreen;

	CButtonWnd* pButton = (CButtonWnd*)pSidlMgr->CreateXWndFromTemplate(pGwnd, Template);
	if (pButton)
	{
		pButton->SetVisible(true);
		pButton->SetTopOffset(rect.top);
		pButton->SetBottomOffset(rect.bottom);
		pButton->SetLeftOffset(rect.left);
		pButton->SetRightOffset(rect.right);
		pButton->SetCRNormal(color);
		pButton->SetBGColor(bgcolor);
		pButton->SetWindowText(text);
		pButton->SetTooltip(tooltip);
		pButton->SetVisible(bShow);
	}

	Template->nFont = oldFont;
	Template->strName = oldName;
	Template->strScreenId = oldScreenID;

	return pButton;
}

CHotButton* CreateGroupHotButton(CGroupWnd* pGwnd, CControlTemplate* Template, const CXStr& name,
	const CXRect& rect, int buttonIndex)
{
	CXStr oldName = Template->strName;
	CXStr oldScreenId = Template->strScreenId;

	Template->strName = name;
	Template->strScreenId = name;

	CHotButton* pButton = (CHotButton*)pSidlMgr->CreateHotButtonWnd(pGwnd, Template);
	pButton->BarIndex = 9;
	pButton->ButtonIndex = buttonIndex;
	pButton->SetButtonSize(100, true);
	pButton->SetUseInLayoutVertical(true);

	pButton->SetWindowStyle(WSF_AUTOSTRETCHH | WSF_TRANSPARENT | WSF_AUTOSTRETCHV | WSF_RELATIVERECT);
	pButton->SetClipToParent(true);
	pButton->SetUseInLayoutHorizontal(true);
	pButton->SetLeftAnchoredToLeft(true);
	pButton->SetRightAnchoredToLeft(true);
	pButton->SetTopAnchoredToTop(false);
	pButton->SetBottomAnchoredToTop(false);

	pButton->SetTopOffset(rect.top);
	pButton->SetBottomOffset(rect.bottom);
	pButton->SetLeftOffset(rect.left);
	pButton->SetRightOffset(rect.right);

	pButton->SetCRNormal(MQColor(0, 255, 255));
	pButton->SetBGColor(MQColor(255, 255, 255));
	pButton->SetVisible(gBShowHotButtons);

	Template->strName = oldName;
	Template->strScreenId = oldScreenId;

	return pButton;
}

void RemoveOurMenu(CGroupWnd* pGwnd)
{
	if (pGwnd->GroupContextMenu)
	{
		pContextMenuManager->Flush();

		if (pGwnd->RoleSelectMenuID)
		{
			pGwnd->GroupContextMenu->RemoveMenuItem(pGwnd->RoleSelectMenuID);
			pGwnd->GroupContextMenu->RemoveMenuItem(pGwnd->RoleSeparatorID);
			pGwnd->RoleSelectMenuID = 0;
			pGwnd->RoleSeparatorID = 0;
		}

		if (separatorid2)
		{
			pGwnd->GroupContextMenu->RemoveMenuItem(followmenuid);
			pGwnd->GroupContextMenu->RemoveMenuItem(switchtomenuid);
			pGwnd->GroupContextMenu->RemoveMenuItem(doormenuid);
			pGwnd->GroupContextMenu->RemoveMenuItem(groundmenuid);
			pGwnd->GroupContextMenu->RemoveMenuItem(gotomenuid);
			pGwnd->GroupContextMenu->RemoveMenuItem(navmenuid);
			pGwnd->GroupContextMenu->RemoveMenuItem(mmlmenuid);
			pGwnd->GroupContextMenu->RemoveMenuItem(separatorid2);
			followmenuid = 0;
			switchtomenuid = 0;
			doormenuid = 0;
			groundmenuid = 0;
			gotomenuid = 0;
			navmenuid = 0;
			mmlmenuid = 0;
			separatorid2 = 0;
		}

		if (separatorId)
		{
			pGwnd->GroupContextMenu->RemoveMenuItem(distanceoptionmenuid);
			pGwnd->GroupContextMenu->RemoveMenuItem(hotoptionmenuid);
			pGwnd->GroupContextMenu->RemoveMenuItem(followmeoptionmenuid);
			pGwnd->GroupContextMenu->RemoveMenuItem(mimicmeoptionmenuid);
			pGwnd->GroupContextMenu->RemoveMenuItem(cometomeoptionmenuid);

			pGwnd->GroupContextMenu->RemoveMenuItem(separatorId);

			distanceoptionmenuid = 0;
			hotoptionmenuid = 0;
			followmeoptionmenuid = 0;
			mimicmeoptionmenuid = 0;
			cometomeoptionmenuid = 0;
			separatorId = 0;
		}
	}
}

void AddOurMenu(CGroupWnd* pGwnd, bool bMemberClicked, int index)
{
	if (pGwnd->GroupContextMenu)
	{
		RemoveOurMenu(pGwnd);
		separatorId = pGwnd->GroupContextMenu->AddSeparator();
		cometomeoptionmenuid = pGwnd->GroupContextMenu->AddMenuItem("Show Come to Me Button", TIMC_ComeToMeButton, gBShowComeToMeButton);
		mimicmeoptionmenuid = pGwnd->GroupContextMenu->AddMenuItem("Show Mimic Me Button", TIMC_MimicMeButton, gBShowMimicMeButton);
		followmeoptionmenuid = pGwnd->GroupContextMenu->AddMenuItem("Show Follow Button", TIMC_FollowMeButton, gBShowFollowMeButton);
		hotoptionmenuid = pGwnd->GroupContextMenu->AddMenuItem("Show Hot Buttons", TIMC_HotButtons, gBShowHotButtons);
		distanceoptionmenuid = pGwnd->GroupContextMenu->AddMenuItem("Show Distance", TIMC_Distance, gBShowDistance);

		if (bMemberClicked)
		{
			separatorid2 = pGwnd->GroupContextMenu->AddSeparator();
			mmlmenuid = pGwnd->GroupContextMenu->AddMenuItem("Make Me Leader", TIMC_MakeMeLeader);
			navmenuid = pGwnd->GroupContextMenu->AddMenuItem("Run To Me", TIMC_NavToMe);
			gotomenuid = pGwnd->GroupContextMenu->AddMenuItem("Run To...", TIMC_Runto);
			groundmenuid = pGwnd->GroupContextMenu->AddMenuItem("Pick Up Nearest Ground Item", TIMC_PickupGroundItem);
			doormenuid = pGwnd->GroupContextMenu->AddMenuItem("Click Nearest Door", TIMC_ClickNearestDoor);
			switchtomenuid = pGwnd->GroupContextMenu->AddMenuItem("Switch to...", TIMC_SwitchTo);

			if (FollowMeMap.find(index) != FollowMeMap.end())
			{
				if (FollowMeMap[index] == true)
				{
					followmenuid = pGwnd->GroupContextMenu->AddMenuItem("Stop Following Me", TIMC_FollowMe);
				}
				else
				{
					followmenuid = pGwnd->GroupContextMenu->AddMenuItem("Follow Me", TIMC_FollowMe);
				}
			}
			else
			{
				followmenuid = pGwnd->GroupContextMenu->AddMenuItem("Follow Me", TIMC_FollowMe);
			}
		}
		pContextMenuManager->Flush();
	}
}

bool CheckNetCommand(const std::string& theCommand)
{
	if (!gbDisableNetCommandChecks)
	{
		if (ci_find_substr(theCommand, "/bc") != -1)
		{
			bool bConnectedtoEqBCs = false;
			if (HMODULE hMod = GetModuleHandle("mq2eqbc"))
			{
				unsigned short(*fisConnected)();
				if (fisConnected = (unsigned short(*)())GetProcAddress(hMod, "isConnected"))
				{
					if (fisConnected())
					{
						bConnectedtoEqBCs = true;
					}
				}
			}
			if (!bConnectedtoEqBCs)
			{
				WriteChatf("[MQ2GroupInfo] \ar%s\ax only works if mq2eqbc is loaded and eqbcs is started, Please run /plugin mq2eqbc and then /bccmd connect", theCommand.c_str());
				return false;
			}
		}
		else if (ci_find_substr(theCommand, "/dg") != -1)
		{
			if (!GetModuleHandle("mq2dannet"))
			{
				WriteChatf("[MQ2GroupInfo] \ar%s\ax only works if mq2dannet is loaded, Please run /plugin mq2dannet", theCommand.c_str());
				return false;
			}
		}
		else
		{
			WriteChatf("[MQ2GroupInfo] No module check known for: %s", theCommand.c_str());
		}
	}
	return true;
}

// FIXME:  This function can probably be removed in favor of calling the command the user set directly
void StopMovement(bool bChange = true, bool bStopNav = false)
{
	if (bChange)
	{
		CheckNetCommand(FollowStopCommand);
		DoCommand(pLocalPlayer, FollowStopCommand.c_str());

		if (bStopNav)
		{
			CheckNetCommand(NavStopCommand);
			DoCommand(pLocalPlayer, NavStopCommand.c_str());
		}

		FollowMeButton->bChecked = false;
		gbFollowme = false;
	}
}

void DoFollowMe(bool bOnOff)
{
	if (!CheckNetCommand(FollowMeCommand))
		return;

	if (gbFollowme != bOnOff)
	{
		gbFollowme = !gbFollowme;
		FollowMeButton->bChecked = gbFollowme;
		WriteChatf("\ay[MQ2GroupInfo]\ax : Telling group to %s following you.", gbFollowme ? "\agSTART\ax" : "\arSTOP\ax");
		if (!gbFollowme)
		{
			StopMovement();
		}
		else
		{
			DoCommand(pLocalPlayer, FollowMeCommand.c_str());
		}
	}
	else
	{
		WriteChatf("\ay[MQ2GroupInfo]\ax : Your group is already set %s follow you.", gbFollowme ? "\agTO\ax" : "\arTO NOT\ax");
	}
}

void UnpackIni()
{
	HMODULE hMe = nullptr;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR)UnpackIni, &hMe);

	if (HRSRC hRes = FindResource(hMe, MAKEINTRESOURCE(IDR_INI1), "INI"))
	{
		if (const HGLOBAL bin = LoadResource(hMe, hRes))
		{
			if (void* pMyBinaryData = LockResource(bin))
			{
				std::size_t ressize = SizeofResource(hMe, hRes);
				FILE* File = nullptr;
				const errno_t err = fopen_s(&File, INIFileName, "wb");
				if (!err && File != nullptr)
				{
					fwrite(pMyBinaryData, ressize, 1, File);
					fclose(File);
				}
			}
		}
	}
}

void HandleINI(eINIOptions Operation)
{
	std::error_code ec;
	// If the INI doesn't exist, create it and switch our operation to read.
	if (!std::filesystem::exists(INIFileName, ec))
	{
		UnpackIni();
		Operation = eINIOptions::ReadOnly;
	}

	char szSettingINISection[MAX_STRING] = "Default";

	if (!Initialized)
	{
		gBUsePerCharSettings = GetPrivateProfileBool("Default", "UsePerCharSettings", gBUsePerCharSettings, INIFileName);
	}

	if (gBUsePerCharSettings && pLocalPlayer && EQADDR_SERVERNAME[0] != '\0')
	{
		sprintf_s(szSettingINISection, "%s_%s", EQADDR_SERVERNAME, pLocalPlayer->Name);
	}

	std::string strUISection = "UI_";
	strUISection.append(gUISkin);

	if (Operation == eINIOptions::ReadOnly || Operation == eINIOptions::ReadAndWrite)
	{
		gbDisableNetCommandChecks = GetPrivateProfileBool(szSettingINISection, "DisableNetCommandChecks", gbDisableNetCommandChecks, INIFileName);
		gBShowDistance = GetPrivateProfileBool(szSettingINISection, "ShowDistance", gBShowDistance, INIFileName);
		DistanceLabelToolTip = GetPrivateProfileString(szSettingINISection, "DistanceLabelToolTip", DistanceLabelToolTip, INIFileName);
		gBShowMimicMeButton = GetPrivateProfileBool(szSettingINISection, "ShowMimicMeButton", gBShowMimicMeButton, INIFileName);
		gBShowComeToMeButton = GetPrivateProfileBool(szSettingINISection, "ShowComeToMeButton", gBShowComeToMeButton, INIFileName);
		gBShowFollowMeButton = GetPrivateProfileBool(szSettingINISection, "ShowFollowMeButton", gBShowFollowMeButton, INIFileName);
		gBShowHotButtons = GetPrivateProfileBool(szSettingINISection, "ShowHotButtons", gBShowHotButtons, INIFileName);
		ComeToMeLabel = GetPrivateProfileString(szSettingINISection, "ComeToMeText", ComeToMeLabel, INIFileName);
		ComeToMeCommand = GetPrivateProfileString(szSettingINISection, "ComeToMeCommand", ComeToMeCommand, INIFileName);
		ComeToMeToolTip = GetPrivateProfileString(szSettingINISection, "ComeToMeToolTip", ComeToMeToolTip, INIFileName);
		NavStopCommand = GetPrivateProfileString(szSettingINISection, "NavStopCommand", NavStopCommand, INIFileName);
		FollowMeLabel = GetPrivateProfileString(szSettingINISection, "FollowMeText", FollowMeLabel, INIFileName);
		FollowMeCommand = GetPrivateProfileString(szSettingINISection, "FollowMeCommand", FollowMeCommand, INIFileName);
		FollowMeToolTip = GetPrivateProfileString(szSettingINISection, "FollowMeToolTip", FollowMeToolTip, INIFileName);
		FollowStopCommand = GetPrivateProfileString(szSettingINISection, "FollowStopCommand", FollowStopCommand, INIFileName);
		MimicMeLabel = GetPrivateProfileString(szSettingINISection, "MimicMeText", MimicMeLabel, INIFileName);
		MimicMeSayCommand = GetPrivateProfileString(szSettingINISection, "MimicMeSayCommand", MimicMeSayCommand, INIFileName);
		MimicMeHailCommand = GetPrivateProfileString(szSettingINISection, "MimicMeHailCommand", MimicMeHailCommand, INIFileName);
		MimicMeToolTip = GetPrivateProfileString(szSettingINISection, "MimicMeToolTip", MimicMeToolTip, INIFileName);
		TargetCommand = GetPrivateProfileString(szSettingINISection, "TargetCommand", TargetCommand, INIFileName);

		/* Also defaulted on the global and in the .ini resource */
		gbDynamicUI = GetPrivateProfileBool(strUISection, "DynamicUI", true, INIFileName); // see note above
		gbUseGroupLayoutBox = GetPrivateProfileBool(strUISection, "UseGroupLayoutBox", false, INIFileName); // see note above
		BaseLabelName = GetPrivateProfileString(strUISection, "LabelBaseGW", "Player_ManaLabel", INIFileName); // see note above
		gGroupDistanceFontSize = GetPrivateProfileInt(strUISection, "GroupDistanceFontSize", 2, INIFileName); // see note above
		gGroupDistanceOffset = GetPrivateProfileInt(strUISection, "GroupDistanceOffset", 2, INIFileName); // see note above
		GroupDistanceLoc = GetPrivateProfileString(strUISection, "GroupDistanceLoc", "0,-20,70,0", INIFileName); // see note above
		GroupDistanceElementPrefix = GetPrivateProfileString(strUISection, "GroupDistanceElementPrefix", "Needs to be set when UseGroupLayoutBox is enabled", INIFileName); // see note above
		ComeToMeLoc = GetPrivateProfileString(strUISection, "ComeToMeLoc", "61,27,6,46", INIFileName); // see note above
		FollowMeLoc = GetPrivateProfileString(strUISection, "FollowMeLoc", "61,27,48,88", INIFileName); // see note above
		MimicMeLoc = GetPrivateProfileString(strUISection, "MimicMeLoc", "61,27,90,130", INIFileName); // see note above
		HotButton0Loc = GetPrivateProfileString(strUISection, "HotButton0Loc", "97,64,6,46", INIFileName); // see note above
		HotButton1Loc = GetPrivateProfileString(strUISection, "HotButton1Loc", "97,64,49,89", INIFileName); // see note above
		HotButton2Loc = GetPrivateProfileString(strUISection, "HotButton2Loc", "97,64,92,132", INIFileName); // see note above
		/* End multiple location defaults */
	}
	if (Operation == eINIOptions::WriteOnly || Operation == eINIOptions::ReadAndWrite)
	{
		WritePrivateProfileBool("Default", "UsePerCharSettings", gBUsePerCharSettings, INIFileName);

		WritePrivateProfileBool(szSettingINISection, "DisableNetCommandChecks", gbDisableNetCommandChecks, INIFileName);
		WritePrivateProfileBool(szSettingINISection, "ShowDistance", gBShowDistance, INIFileName);
		WritePrivateProfileString(szSettingINISection, "DistanceLabelToolTip", DistanceLabelToolTip, INIFileName);
		WritePrivateProfileBool(szSettingINISection, "ShowMimicMeButton", gBShowMimicMeButton, INIFileName);
		WritePrivateProfileBool(szSettingINISection, "ShowComeToMeButton", gBShowComeToMeButton, INIFileName);
		WritePrivateProfileBool(szSettingINISection, "ShowFollowMeButton", gBShowFollowMeButton, INIFileName);
		WritePrivateProfileBool(szSettingINISection, "ShowHotButtons", gBShowHotButtons, INIFileName);
		WritePrivateProfileString(szSettingINISection, "ComeToMeText", ComeToMeLabel, INIFileName);
		WritePrivateProfileString(szSettingINISection, "ComeToMeCommand", ComeToMeCommand, INIFileName);
		WritePrivateProfileString(szSettingINISection, "ComeToMeToolTip", ComeToMeToolTip, INIFileName);
		WritePrivateProfileString(szSettingINISection, "NavStopCommand", NavStopCommand, INIFileName);
		WritePrivateProfileString(szSettingINISection, "FollowMeText", FollowMeLabel, INIFileName);
		WritePrivateProfileString(szSettingINISection, "FollowMeCommand", FollowMeCommand, INIFileName);
		WritePrivateProfileString(szSettingINISection, "FollowMeToolTip", FollowMeToolTip, INIFileName);
		WritePrivateProfileString(szSettingINISection, "FollowStopCommand", FollowStopCommand, INIFileName);
		WritePrivateProfileString(szSettingINISection, "MimicMeText", MimicMeLabel, INIFileName);
		WritePrivateProfileString(szSettingINISection, "MimicMeSayCommand", MimicMeSayCommand, INIFileName);
		WritePrivateProfileString(szSettingINISection, "MimicMeHailCommand", MimicMeHailCommand, INIFileName);
		WritePrivateProfileString(szSettingINISection, "MimicMeToolTip", MimicMeToolTip, INIFileName);
		WritePrivateProfileString(szSettingINISection, "TargetCommand", TargetCommand, INIFileName);

		WritePrivateProfileBool(strUISection, "DynamicUI", gbDynamicUI, INIFileName);
		WritePrivateProfileBool(strUISection, "UseGroupLayoutBox", gbUseGroupLayoutBox, INIFileName);
		WritePrivateProfileString(strUISection, "LabelBaseGW", BaseLabelName, INIFileName);
		WritePrivateProfileInt(strUISection, "GroupDistanceFontSize", gGroupDistanceFontSize, INIFileName);
		WritePrivateProfileInt(strUISection, "GroupDistanceOffset", gGroupDistanceOffset, INIFileName);
		WritePrivateProfileString(strUISection, "GroupDistanceLoc", GroupDistanceLoc, INIFileName);
		WritePrivateProfileString(strUISection, "GroupDistanceElementPrefix", GroupDistanceElementPrefix, INIFileName);
		WritePrivateProfileString(strUISection, "ComeToMeLoc", ComeToMeLoc, INIFileName);
		WritePrivateProfileString(strUISection, "FollowMeLoc", FollowMeLoc, INIFileName);
		WritePrivateProfileString(strUISection, "MimicMeLoc", MimicMeLoc, INIFileName);
		WritePrivateProfileString(strUISection, "HotButton0Loc", HotButton0Loc, INIFileName);
		WritePrivateProfileString(strUISection, "HotButton1Loc", HotButton1Loc, INIFileName);
		WritePrivateProfileString(strUISection, "HotButton2Loc", HotButton2Loc, INIFileName);

	}
}

class CGroupWnd_Detours
{
public:
	int GetSelectedGroupIndex(CXWnd* pWnd)
	{
		int index = -1;

		if (pCharData && pCharData->Group)
		{
			// index = group->GroupSelectID;
			for (int i = 1; i < MAX_GROUP_SIZE; i++)
			{
				CGroupMember* pMember = pCharData->Group->GetGroupMember(i);

				if (pMember && pMember->GetPlayer() && pMember->GetPlayer()->Type == EQP_PC
					&& (pWnd == reinterpret_cast<CGroupWnd*>(this)->HPGauge[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->PetGauge[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->ManaGauge[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->EnduranceGauge[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->HPLabel[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->HPPercLabel[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->ManaLabel[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->ManaPercLabel[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->EnduranceLabel[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->EndurancePercLabel[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->EnduranceLabel[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->GroupTankButton[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->GroupAssistButton[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->GroupPullerButton[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->GroupMarkNPCButton[i]
					|| pWnd == reinterpret_cast<CGroupWnd*>(this)->AggroPercLabel[i]))
				{
					return i;
				}
			}
		}
		return index;
	}

	SPAWNINFO* GetSpawnFromRightClickIndex()
	{
		if (pCharData && pCharData->Group)
		{
			CGroupMember* pMember = pCharData->Group->GetGroupMember(gRightClickIndex);
			if (pMember && pMember->GetPlayer() && pMember->GetPlayer()->Type != EQP_NPC)
				return (SPAWNINFO*)pMember->GetPlayer();
		}

		return nullptr;
	}

	bool UpdateOurMenu(int index)
	{
		if (CGroupWnd* pWnd = reinterpret_cast<CGroupWnd*>(this))
		{
			if (pWnd->GroupContextMenu)
			{
				AddOurMenu(reinterpret_cast<CGroupWnd*>(this), index != -1, index);
				return true;
			}
		}

		return false;
	}

	DETOUR_TRAMPOLINE_DEF(int, WndNotification_Trampoline, (CXWnd*, uint32_t, void*));
	int WndNotification_Detour(CXWnd* pWnd, uint32_t Message, void* pData)
	{
		if (Message == XWM_OUTPUT_TEXT)
		{
			if (pWnd && (
				pWnd->GetParentWindow() == GroupHotButton[0]
				|| pWnd->GetParentWindow() == GroupHotButton[1]
				|| pWnd->GetParentWindow() == GroupHotButton[2]
				|| pWnd == GroupHotButton[0]
				|| pWnd == GroupHotButton[1]
				|| pWnd == GroupHotButton[2]
				|| pWnd == ComeToMeButton
				|| pWnd == MimicMeButton
				|| pWnd == FollowMeButton))
			{
				// we dont want to show the menu here.
				pContextMenuManager->Flush();
			}
		}
		else if (Message == XWM_RCLICK || Message == XWM_RSELITEM_DOWN)
		{
			if (pWnd && (pWnd->GetParentWindow() == GroupHotButton[0]
				|| pWnd->GetParentWindow() == GroupHotButton[1]
				|| pWnd->GetParentWindow() == GroupHotButton[2]
				|| pWnd == GroupHotButton[0]
				|| pWnd == GroupHotButton[1]
				|| pWnd == GroupHotButton[2]
				|| pWnd == ComeToMeButton
				|| pWnd == MimicMeButton
				|| pWnd == FollowMeButton))
			{
				// we dont want to show the menu here.
				pContextMenuManager->Flush();
				return 1;
			}

			gRightClickIndex = this->GetSelectedGroupIndex(pWnd);
			UpdateOurMenu(gRightClickIndex);
			// dont return here or group roles wont be filled in //return 1;
		}
		else if (Message == XWM_LCLICK)
		{
			if (pWnd == MimicMeButton)
			{
				gbMimicMe = !gbMimicMe;
				MimicMeButton->bChecked = gbMimicMe;
				return 1;
			}

			if (pWnd == ComeToMeButton)
			{
				if (!CheckNetCommand(ComeToMeCommand))
					return 1;

				DoCommand(pLocalPlayer, ComeToMeCommand.c_str());
				return 1;
			}

			if (pWnd == FollowMeButton)
			{
				DoFollowMe(!gbFollowme);
				return 1;
			}

			if (pWnd
				&& (pWnd->GetParentWindow() == GroupHotButton[0]
					|| pWnd->GetParentWindow() == GroupHotButton[1]
					|| pWnd->GetParentWindow() == GroupHotButton[2]
					|| pWnd == GroupHotButton[0]
					|| pWnd == GroupHotButton[1]
					|| pWnd == GroupHotButton[2]))
			{
				return 1; // catches the hotbuttons
			}
		}
		else if (Message == XWM_MENUSELECT)
		{
			int reqId = (int)(uintptr_t)pData;
			// FIXME:  These should be ini set rather than guessed at.
			switch (reqId)
			{
			case TIMC_MakeMeLeader:
			{
				SPAWNINFO* pSpawn = GetSpawnFromRightClickIndex();
				if (pSpawn)
				{
					if (GetModuleHandle("mq2dannet"))
						DoCommandf("/dex %s /makeleader %s", pSpawn->Name, ((SPAWNINFO*)pLocalPlayer)->Name);
					else if (GetModuleHandle("mq2eqbc"))
						DoCommandf("/bct %s //makeleader %s", pSpawn->Name, ((SPAWNINFO*)pLocalPlayer)->Name);
				}
				return 1;
			}

			case TIMC_NavToMe: // our nav menu id
			{
				SPAWNINFO* pSpawn = GetSpawnFromRightClickIndex();
				if (pSpawn)
				{
					StopMovement(gbFollowme);
					if (GetModuleHandle("mq2dannet"))
						DoCommandf("/dex %s /nav id %d", pSpawn->Name, ((SPAWNINFO*)pLocalPlayer)->SpawnID);
					else if (GetModuleHandle("mq2eqbc"))
						DoCommandf("/bct %s //nav id %d", pSpawn->Name, ((SPAWNINFO*)pLocalPlayer)->SpawnID);
				}
				return 1; // we dont need to call the tramp, its our message...
			}

			case TIMC_Runto: // our Run To menu id
			{
				SPAWNINFO* pSpawn = GetSpawnFromRightClickIndex();
				if (pSpawn)
				{
					StopMovement(gbFollowme);
					DoCommandf("/nav id %d", pSpawn->SpawnID);
				}
				return 1; // we dont need to call the tramp, its our message...
			}

			case TIMC_PickupGroundItem: // ground
			{
				SPAWNINFO* pSpawn = GetSpawnFromRightClickIndex();
				if (pSpawn)
				{
					if (GetModuleHandle("mq2dannet"))
					{
						DoCommandf("/dex %s /itemtarget", pSpawn->Name);
						DoCommandf("/dex %s /click left item", pSpawn->Name);
					}
					else if (GetModuleHandle("mq2eqbc"))
					{
						DoCommandf("/bct %s //itemtarget", pSpawn->Name);
						DoCommandf("/bct %s //click left item", pSpawn->Name);
					}
				}
				return 1;
			}

			case TIMC_ClickNearestDoor: // door
			{
				SPAWNINFO* pSpawn = GetSpawnFromRightClickIndex();
				if (pSpawn)
				{
					if (GetModuleHandle("mq2dannet"))
					{
						DoCommandf("/dex %s /doortarget", pSpawn->Name);
						DoCommandf("/dex %s /click left door", pSpawn->Name);
					}
					else if (GetModuleHandle("mq2eqbc"))
					{
						DoCommandf("/bct %s //doortarget", pSpawn->Name);
						DoCommandf("/bct %s //click left door", pSpawn->Name);
					}
				}
				return 1;
			}

			case TIMC_SwitchTo: // switchto
			{
				SPAWNINFO* pSpawn = GetSpawnFromRightClickIndex();
				if (pSpawn)
				{
					if (GetModuleHandle("mq2dannet"))
					{
						DoCommandf("/dex %s /foreground", pSpawn->Name);
					}
					else if (GetModuleHandle("mq2eqbc"))
					{
						DoCommandf("/bct %s //foreground", pSpawn->Name);
					}
				}
				return 1;
			}

			case TIMC_FollowMe: // follow me
			{
				SPAWNINFO* pSpawn = GetSpawnFromRightClickIndex();
				if (pSpawn)
				{
					if (FollowMeMap.find(gRightClickIndex) != FollowMeMap.end())
					{
						if (FollowMeMap[gRightClickIndex])
						{
							if (GetModuleHandle("mq2dannet"))
								DoCommandf("/dex %s /afollow off", pSpawn->Name);
							else if (GetModuleHandle("mq2eqbc"))
								DoCommandf("/bct %s //afollow off", pSpawn->Name);
							FollowMeMap[gRightClickIndex] = false;
						}
						else
						{
							if (GetModuleHandle("mq2dannet"))
								DoCommandf("/dex %s /afollow spawn %d", pSpawn->Name, ((SPAWNINFO*)pLocalPlayer)->SpawnID);
							else if (GetModuleHandle("mq2eqbc"))
								DoCommandf("/bct %s //afollow spawn %d", pSpawn->Name, ((SPAWNINFO*)pLocalPlayer)->SpawnID);
							FollowMeMap[gRightClickIndex] = true;
						}
					}
					else
					{
						if (GetModuleHandle("mq2dannet"))
							DoCommandf("/dex %s /afollow spawn %d", pSpawn->Name, ((SPAWNINFO*)pLocalPlayer)->SpawnID);
						else if (GetModuleHandle("mq2eqbc"))
							DoCommandf("/bct %s //afollow spawn %d", pSpawn->Name, ((SPAWNINFO*)pLocalPlayer)->SpawnID);
						FollowMeMap[gRightClickIndex] = true;
					}
				}
				return 1;
			}

			case TIMC_ComeToMeButton: // gBShowComeToMeButton
			{
				CContextMenu* pContextMenu = (CContextMenu*)pWnd;

				int iItemID = pContextMenu->GetItemAtPoint(pWndMgr->MousePoint);
				gBShowComeToMeButton = !gBShowComeToMeButton;

				pContextMenu->CheckMenuItem(iItemID, gBShowComeToMeButton);
				ComeToMeButton->SetVisible(gBShowComeToMeButton);

				HandleINI(eINIOptions::WriteOnly);

				return 1;
			}

			case TIMC_MimicMeButton: // gBShowMimicMeButton
			{
				CContextMenu* pContextMenu = (CContextMenu*)pWnd;

				int iItemID = ((CListWnd*)pContextMenu)->GetItemAtPoint(pWndMgr->MousePoint);
				gBShowMimicMeButton = !gBShowMimicMeButton;
				pContextMenu->CheckMenuItem(iItemID, gBShowMimicMeButton);
				MimicMeButton->SetVisible(gBShowMimicMeButton);

				HandleINI(eINIOptions::WriteOnly);

				return 1;
			}

			case TIMC_FollowMeButton: // gBShowFollowMeButton
			{
				CContextMenu* pContextMenu = (CContextMenu*)pWnd;

				int iItemID = ((CListWnd*)pContextMenu)->GetItemAtPoint(pWndMgr->MousePoint);
				gBShowFollowMeButton = !gBShowFollowMeButton ;
				pContextMenu->CheckMenuItem(iItemID, gBShowFollowMeButton);
				FollowMeButton->SetVisible(gBShowFollowMeButton);

				HandleINI(eINIOptions::WriteOnly);

				return 1;
			}

			case TIMC_HotButtons: // gBShowHotButtons
			{
				CContextMenu* pContextMenu = (CContextMenu*)pWnd;

				int iItemID = ((CListWnd*)pContextMenu)->GetItemAtPoint(pWndMgr->MousePoint);
				gBShowHotButtons = !gBShowHotButtons;
				pContextMenu->CheckMenuItem(iItemID, gBShowHotButtons);
				GroupHotButton[0]->SetVisible(gBShowHotButtons);
				GroupHotButton[1]->SetVisible(gBShowHotButtons);
				GroupHotButton[2]->SetVisible(gBShowHotButtons);

				HandleINI(eINIOptions::WriteOnly);

				return 1;
			}

			case TIMC_Distance: // gBShowDistance
			{
				CContextMenu* pContextMenu = (CContextMenu*)pWnd;

				int iItemID = ((CListWnd*)pContextMenu)->GetItemAtPoint(pWndMgr->MousePoint);
				gBShowDistance = !gBShowDistance;
				pContextMenu->CheckMenuItem(iItemID, gBShowDistance);

				for (auto& GroupDistLabel : GroupDistLabels)
					GroupDistLabel->SetVisible(gBShowDistance);

				HandleINI(eINIOptions::WriteOnly);

				return 1;
			}

			}
		}
		return WndNotification_Trampoline(pWnd, Message, pData);
	}
};

void HandleTargetChange()
{
	if (pTarget && pTarget->SpawnID != LastTargetID)
	{
		LastTargetID = pTarget->SpawnID;
		if (CheckNetCommand(TargetCommand))
		{
			WriteChatf("\ay[MQ2GroupInfo] :\ax Letting group know target changed");
			const std::string myCommand = fmt::format("{} {:d}", TargetCommand, LastTargetID);
			DoCommandf(myCommand.c_str());
		}
	}
}

CXRect GetCXRectTBLRFromString(const std::string& Input, int defaultTop, int defaultBottom, int defaultLeft, int defaultRight)
{
	auto splitString = split(Input, ',');
	if (splitString.size() == 4)
	{
		defaultTop = GetIntFromString(splitString[0], defaultTop);
		defaultBottom = GetIntFromString(splitString[1], defaultBottom);
		defaultLeft = GetIntFromString(splitString[2], defaultLeft);
		defaultRight = GetIntFromString(splitString[3], defaultRight);
	}
	return CXRect(defaultLeft, defaultTop, defaultRight, defaultBottom);
}

void Initialize()
{
	if (Initialized)
		return;

	if (GetGameState() == GAMESTATE_INGAME && !bDisablePluginDueToBadUI && pGroupWnd)
	{
		HandleINI(eINIOptions::ReadAndWrite);
		orgwstyle = pGroupWnd->GetWindowStyle();
		if (orgwstyle & WSF_TITLEBAR)
		{
			pGroupWnd->AddStyle(WSF_SIZABLE | WSF_BORDER);
		}
		else
		{
			pGroupWnd->AddStyle(WSF_CLIENTMOVABLE | WSF_SIZABLE | WSF_BORDER);
		}

		pGroupWnd->SetTooltip(szMMainTip);

		if (pGroupWnd->GroupContextMenu)
		{
			pGroupWnd->GroupContextMenu->SetCRNormal(0xFF000000);
			pGroupWnd->GroupContextMenu->SetDisabledBackground(0xFF000000);
			pGroupWnd->GroupContextMenu->SetBGColor(0xFF000000);
		}

		// AddOurMenu(pGwnd);
		for (int i = 0; i < NUM_GROUPWND_CONTROLS; ++i)
		{
			char szName[32] = { 0 };
			sprintf_s(szName, "Gauge%d", i + 1);

			GW_Gauges[i] = (CGaugeWnd*)pGroupWnd->GetChildItem(szName);
		}

		CControlTemplate* DistLabelTemplate = (CControlTemplate*)pSidlMgr->FindScreenPieceTemplate(BaseLabelName.c_str());
		CControlTemplate* ComeToMeButtonTemplate = (CControlTemplate*)pSidlMgr->FindScreenPieceTemplate("GW_InviteButton"); // borrowing this...
		CControlTemplate* HBButtonTemplate1 = (CControlTemplate*)pSidlMgr->FindScreenPieceTemplate("HB_Button1");
		CControlTemplate* HBButtonTemplate2 = (CControlTemplate*)pSidlMgr->FindScreenPieceTemplate("HB_Button2");
		CControlTemplate* HBButtonTemplate3 = (CControlTemplate*)pSidlMgr->FindScreenPieceTemplate("HB_Button3");

		if (GW_Gauges[0] && DistLabelTemplate)
		{
			const std::string OldBaseLabelController = DistLabelTemplate->strController.c_str();

			DistLabelTemplate->strController = "0";

			CXRect tPos = GetCXRectTBLRFromString(GroupDistanceLoc, 0, -20, 70, 0);

			if (gbUseGroupLayoutBox) // they have a weird UI like sars that uses a layout box these UI's don't have any locations we can read
			{
				tPos.top += gGroupDistanceOffset;

				for (int i = 0; i < NUM_GROUPWND_CONTROLS; ++i)
				{
					const std::string strDistPrefix = GroupDistanceElementPrefix + std::to_string(i + 1);
					if (CXWnd* wnd = pGroupWnd->GetChildItem(strDistPrefix.c_str()))
					{
						char labelName[20] = { 0 };
						sprintf_s(labelName, "Group_DistLabel%d", i + 1);

						GroupDistLabels[i] = CreateDistLabel(wnd, DistLabelTemplate, labelName, gGroupDistanceFontSize,
							wnd->GetLocation() + tPos, true, gBShowDistance);
					}
				}
			}
			else
			{
				if (gbDynamicUI)
				{
					for (int i = 0; i < NUM_GROUPWND_CONTROLS; ++i)
					{
						char labelName[20] = { 0 };
						sprintf_s(labelName, "Group_DistLabel%d", i + 1);

						CXRect rect(GW_Gauges[i]->GetLeftOffset(), GW_Gauges[i]->GetTopOffset(), GW_Gauges[i]->GetRightOffset(), GW_Gauges[i]->GetBottomOffset());

						GroupDistLabels[i] = CreateDistLabel(pGroupWnd, DistLabelTemplate, "Group_DistLabel1", gGroupDistanceFontSize, rect + tPos, true, gBShowDistance);
					}
				}
				else
				{
					for (int i = 0; i < NUM_GROUPWND_CONTROLS; ++i)
					{
						char labelName[20] = { 0 };
						sprintf_s(labelName, "Group_DistLabel%d", i + 1);

						CXRect rect = GW_Gauges[i]->GetLocation() + tPos;
						GroupDistLabels[i] = CreateDistLabel(pGroupWnd, DistLabelTemplate, "Group_DistLabel1", gGroupDistanceFontSize, rect, true, gBShowDistance);
					}
				}
			}

			// create Nav All to Me Button
			if (ComeToMeButtonTemplate)
			{
				int oldfont = ComeToMeButtonTemplate->nFont;
				bool oldbRelativePosition = ComeToMeButtonTemplate->bRelativePosition;
				bool oldbAutoStretchVertical = ComeToMeButtonTemplate->bAutoStretchVertical;
				bool oldbAutoStretchHorizontal = ComeToMeButtonTemplate->bAutoStretchHorizontal;
				bool oldbTopAnchorToTop = ComeToMeButtonTemplate->bTopAnchorToTop;
				bool oldbBottomAnchorToTop = ComeToMeButtonTemplate->bBottomAnchorToTop;
				bool oldbLeftAnchorToLeft = ComeToMeButtonTemplate->bLeftAnchorToLeft;
				bool oldbRightAnchorToLeft = ComeToMeButtonTemplate->bRightAnchorToLeft;
				DWORD oldStyleBits = ComeToMeButtonTemplate->uStyleBits;

				// setup our template the way we want it:
				ComeToMeButtonTemplate->uStyleBits = WSF_AUTOSTRETCHH | WSF_AUTOSTRETCHV | WSF_RELATIVERECT;
				ComeToMeButtonTemplate->bRightAnchorToLeft = true;
				ComeToMeButtonTemplate->bLeftAnchorToLeft = true;
				ComeToMeButtonTemplate->bBottomAnchorToTop = false;
				ComeToMeButtonTemplate->bTopAnchorToTop = false;
				ComeToMeButtonTemplate->bAutoStretchHorizontal = true;
				ComeToMeButtonTemplate->bAutoStretchVertical = true;
				ComeToMeButtonTemplate->bRelativePosition = true;

				CButtonWnd* Butt = (CButtonWnd*)pGroupWnd->GetChildItem("GW_InviteButton");

				// Come To Me button
				ComeToMeButton = CreateAButton(pGroupWnd, ComeToMeButtonTemplate, "GW_ComeToMeButton", "ComeToMeButton", 1, GetCXRectTBLRFromString(ComeToMeLoc, 61, 27, 6, 46), 0xFF00FFFF, 0xFFFFFFFF, ComeToMeToolTip.c_str(), ComeToMeLabel.c_str(), gBShowComeToMeButton);

				FollowMeButton = CreateAButton(pGroupWnd, ComeToMeButtonTemplate, "GW_FollowMeButton", "FollowMeButton", 1, GetCXRectTBLRFromString(FollowMeLoc, 61, 27, 48, 88), 0xFF00FFFF, 0xFFFFFFFF, FollowMeToolTip.c_str(), FollowMeLabel.c_str(), gBShowFollowMeButton);

				// Mimic Me button
				MimicMeButton = CreateAButton(pGroupWnd, ComeToMeButtonTemplate, "GW_MimicMeButton", "MimicMeButton", 1, GetCXRectTBLRFromString(MimicMeLoc, 61, 27, 90, 130), 0xFF00FFFF, 0xFFFFFFFF, MimicMeToolTip.c_str(), MimicMeLabel.c_str(), gBShowMimicMeButton);

				// Hotbutton0
				GroupHotButton[0] = CreateGroupHotButton(pGroupWnd, HBButtonTemplate1, "GW_HotButton1", GetCXRectTBLRFromString(HotButton0Loc, 97, 64, 6, 46), 0);

				// Hotbutton1
				GroupHotButton[1] = CreateGroupHotButton(pGroupWnd, HBButtonTemplate2, "GW_HotButton2", GetCXRectTBLRFromString(HotButton1Loc, 97, 64, 49, 89), 1);

				// Hotbutton2
				GroupHotButton[2] = CreateGroupHotButton(pGroupWnd, HBButtonTemplate3, "GW_HotButton3", GetCXRectTBLRFromString(HotButton2Loc, 97, 64, 92, 132), 2);

				// now set the template values back
				ComeToMeButtonTemplate->strName = "GW_InviteButton";
				ComeToMeButtonTemplate->strScreenId = "InviteButton";
				ComeToMeButtonTemplate->bRelativePosition = oldbRelativePosition;
				ComeToMeButtonTemplate->bAutoStretchVertical = oldbAutoStretchVertical;
				ComeToMeButtonTemplate->bAutoStretchHorizontal = oldbAutoStretchHorizontal;
				ComeToMeButtonTemplate->bTopAnchorToTop = oldbTopAnchorToTop;
				ComeToMeButtonTemplate->bBottomAnchorToTop = oldbBottomAnchorToTop;
				ComeToMeButtonTemplate->bLeftAnchorToLeft = oldbLeftAnchorToLeft;
				ComeToMeButtonTemplate->bRightAnchorToLeft = oldbRightAnchorToLeft;
				ComeToMeButtonTemplate->uStyleBits = oldStyleBits;
				ComeToMeButtonTemplate->nFont = oldfont;
			}

			// now set the template values back
			DistLabelTemplate->strController = OldBaseLabelController;
		}
		else
		{
			bDisablePluginDueToBadUI = true;
			WriteChatf("MQ2GroupInfo has been disabled due to an incompatible UI, let a developer know.");
			return;
		}

		Initialized = true;
	}
}

void UpdateGroupDist(int index)
{
	if (index < 1 || index > NUM_GROUPWND_CONTROLS)
		return;

	if (CLabelWnd* pWnd = GroupDistLabels[index - 1])
	{
		CGroupMember* pMember = pCharData->Group->GetGroupMember(index);
		if (pMember && pMember->GetPlayer())
		{
			char szTargetDist[EQ_MAX_NAME] = { 0 };

			float dist = Distance3DToSpawn(pLocalPlayer, pMember->pSpawn);
			sprintf_s(szTargetDist, "%.2f", dist);

			if (dist < 250)
			{
				pWnd->SetCRNormal(MQColor(0, 255, 0)); // green
			}
			else
			{
				pWnd->SetCRNormal(MQColor(255, 0, 0)); // red
			}

			pWnd->SetWindowText(szTargetDist);
			pWnd->SetVisible(true);
		}
		else
		{
			pWnd->SetVisible(false);
		}
	}
}

void CleanUp()
{
	bDisablePluginDueToBadUI = false;

	if (pGroupWnd)
	{
		if (orgwstyle)
		{
			orgwstyle = 0;
		}

		if (pGroupWnd->GroupContextMenu && separatorId)
		{
			RemoveOurMenu(pGroupWnd);
		}
	}

	for (auto& label : GroupDistLabels)
	{
		if (label)
		{
			label->Destroy();
			label = nullptr;
		}
	}

	if (ComeToMeButton)
	{
		ComeToMeButton->Destroy();
		ComeToMeButton = nullptr;
	}

	if (FollowMeButton)
	{
		FollowMeButton->Destroy();
		FollowMeButton = nullptr;
	}

	if (MimicMeButton)
	{
		MimicMeButton->Destroy();
		MimicMeButton = nullptr;
	}

	for (auto& button : GroupHotButton)
	{
		if (button)
		{
			button->Destroy();
			button = nullptr;
		}
	}
}

void ShowHelp()
{
	WriteChatf("\ayMQ2GroupInfo Usage (green indicates your current setting):");
	WriteChatf("     \ay/groupinfo perchar [%sOn\ay|%sOff\ay]\aw will toggle splitting settings by character.", gBUsePerCharSettings ? "\ag" : "", gBUsePerCharSettings ? "" : "\ag");
	WriteChatf("     \ay/groupinfo cometome\aw imitates pressing the %s button.", ComeToMeLabel.c_str());
	WriteChatf("     \ay/groupinfo followme [%sOn\ay|%sOff\ay]\aw will toggle group follow (imitates %s button).", gbFollowme ? "\ag" : "", gbFollowme ? "" : "\ag", FollowMeLabel.c_str());
	WriteChatf("     \ay/groupinfo mimicme [%sOn\ay|%sOff\ay]\aw will toggle group mimic (imitates %s button).", gbMimicMe ? "\ag" : "", gbMimicMe ? "" : "\ag", MimicMeLabel.c_str());
	WriteChatf("     \ay/groupinfo show cometome [%sOn\ay|%sOff\ay]\aw will toggle showing the %s button.", gBShowComeToMeButton ? "\ag" : "", gBShowComeToMeButton ? "" : "\ag", ComeToMeLabel.c_str());
	WriteChatf("     \ay/groupinfo show distance [%sOn\ay|%sOff\ay]\aw will toggle showing distance to group.", gBShowDistance ? "\ag" : "", gBShowDistance ? "" : "\ag");
	WriteChatf("     \ay/groupinfo show followme [%sOn\ay|%sOff\ay]\aw will toggle showing the %s button.", gBShowFollowMeButton ? "\ag" : "", gBShowFollowMeButton ? "" : "\ag", FollowMeLabel.c_str());
	WriteChatf("     \ay/groupinfo show hot [%sOn\ay|%sOff\ay]\aw will toggle showing hotbuttons.", gBShowHotButtons ? "\ag" : "", gBShowHotButtons ? "" : "\ag");
	WriteChatf("     \ay/groupinfo show mimicme [%sOn\ay|%sOff\ay]\aw will toggle showing the %s button.", gBShowMimicMeButton ? "\ag" : "", gBShowMimicMeButton ? "" : "\ag", MimicMeLabel.c_str());
	WriteChatf("     \ay/groupinfo disablenetcheck [%sOn\ay|%sOff\ay]\aw will toggle checking EQBC/DanNet commands before issuing them.", gbDisableNetCommandChecks ? "\ag" : "", gbDisableNetCommandChecks ? "\ar" : "\ag");
	WriteChatf("     \ay/groupinfo reset\aw will reset all settings to default.");
	WriteChatf("     \ay/groupinfo reload\aw will reload all settings.");
}

void CMD_GroupInfo(SPAWNINFO* pPlayer, char* szLine)
{
	char szArg1[MAX_STRING] = { 0 };
	GetArg(szArg1, szLine, 1);

	bool WriteIni = false;

	if (ci_equals(szArg1, "perchar"))
	{
		GetArg(szArg1, szLine, 2);
		gBUsePerCharSettings = GetBoolFromString(szArg1, !gBUsePerCharSettings);
		WriteIni = true;
	}
	else if (ci_equals(szArg1, "disablenetcheck"))
	{
		GetArg(szArg1, szLine, 2);
		gbDisableNetCommandChecks = GetBoolFromString(szArg1, !gbDisableNetCommandChecks);
		WriteIni = true;
	}
	else if (!_stricmp(szArg1, "show"))
	{
		GetArg(szArg1, szLine, 2);
		char szArg2[MAX_STRING] = { 0 };
		GetArg(szArg2, szLine, 3);

		WriteIni = true;
		if (!_stricmp(szArg1, "cometome"))
		{
			gBShowComeToMeButton = GetBoolFromString(szArg2, !gBShowComeToMeButton);
		}
		else if (!_stricmp(szArg1, "distance"))
		{
			gBShowDistance = GetBoolFromString(szArg2, !gBShowDistance);
		}
		else if (!_stricmp(szArg1, "followme"))
		{
			gBShowFollowMeButton = GetBoolFromString(szArg2, !gBShowFollowMeButton);
		}
		else if (!_stricmp(szArg1, "hot"))
		{
			gBShowHotButtons = GetBoolFromString(szArg2, !gBShowHotButtons);
		}
		else if (!_stricmp(szArg1, "mimicme"))
		{
			gBShowMimicMeButton = GetBoolFromString(szArg2, !gBShowMimicMeButton);
		}
		else
		{
			WriteIni = false;
			ShowHelp();
		}
	}
	else if (!_stricmp(szArg1, "mimicme"))
	{
		char szArg2[MAX_STRING] = { 0 };
		GetArg(szArg2, szLine, 2);

		bool show = GetBoolFromString(szArg2, !gbMimicMe);
		gbMimicMe = show;
		MimicMeButton->bChecked = show;
	}
	else if (!_stricmp(szArg1, "followme"))
	{
		char szArg2[MAX_STRING] = { 0 };
		GetArg(szArg2, szLine, 2);
		DoFollowMe(GetBoolFromString(szArg2, !gbFollowme));
	}
	else if (!_stricmp(szArg1, "cometome"))
	{
		if (!CheckNetCommand(ComeToMeCommand))
			return;

		DoCommand(pLocalPlayer, ComeToMeCommand.c_str());
	}
	else if (ci_equals(szArg1, "reset"))
	{
		UnpackIni();
		CleanUp();
		Initialized=false;
	}
	else if (ci_equals(szArg1, "reload"))
	{
		CleanUp();
		Initialized=false;
	}
	else
	{
		ShowHelp();
	}

	if (WriteIni)
	{
		HandleINI(eINIOptions::WriteOnly);
	}
}

PLUGIN_API void InitializePlugin()
{
	AddCommand("/groupinfo", CMD_GroupInfo);

	EzDetour(CGroupWnd__WndNotification,
		&CGroupWnd_Detours::WndNotification_Detour,
		&CGroupWnd_Detours::WndNotification_Trampoline);

	Initialize();
}

PLUGIN_API void ShutdownPlugin()
{
	CleanUp();
	RemoveCommand("/groupinfo");
	RemoveDetour(CGroupWnd__WndNotification);
}

PLUGIN_API void OnCleanUI()
{
	CleanUp();
}

PLUGIN_API void OnReloadUI()
{
	Initialized = false;
}

PLUGIN_API void OnPulse()
{
	if (GetGameState() != GAMESTATE_INGAME || !pCharData)
		return;

	static uint64_t lastPulseUpdate = MQGetTickCount64();
	uint64_t currentTime = MQGetTickCount64();

	if (currentTime - lastPulseUpdate > 500) // 500ms
	{
		lastPulseUpdate = currentTime;
		Initialize();

		if (gbMimicMe)
		{
			HandleTargetChange();
		}

		if (pGroupWnd)
		{
			if (pContextMenuManager->NumVisibleMenus == 0 && separatorId)
			{
				RemoveOurMenu(pGroupWnd);
			}

			if (pCharData->Group)
			{
				if (gBShowDistance)
				{
					for (int i = 0; i < NUM_GROUPWND_CONTROLS; ++i)
						UpdateGroupDist(i + 1);
				}
			}
			else
			{
				if (GroupDistLabels[0] && GroupDistLabels[0]->IsVisible())
				{
					for (auto& label : GroupDistLabels)
					{
						if (label) label->SetVisible(false);
					}
				}
			}
			if (MimicMeButton && MimicMeButton->IsVisible() != gBShowMimicMeButton)
			{
				MimicMeButton->SetVisible(gBShowMimicMeButton);
			}
			if (FollowMeButton && FollowMeButton->IsVisible() != gBShowFollowMeButton)
			{
				FollowMeButton->SetVisible(gBShowFollowMeButton);
			}
			if (ComeToMeButton && ComeToMeButton->IsVisible() != gBShowComeToMeButton)
			{
				ComeToMeButton->SetVisible(gBShowComeToMeButton);
			}
			if (GroupHotButton[0] && GroupHotButton[0]->IsVisible() != gBShowHotButtons)
			{
				for (auto& button : GroupHotButton)
				{
					if (button)
					{
						button->SetVisible(gBShowHotButtons);
					}
				}
			}
		}
	}
}

PLUGIN_API void OnWriteChatColor(const char* Line, int Color, int Filter)
{
	if (gbMimicMe)
	{
		//MQ2EasyFind: Going to (Group) -> Annera
		int linelen = (int)strlen(Line);
		char* szLine = (char*)LocalAlloc(LPTR, linelen + 32);
		char* szLineOrg = szLine;
		strcpy_s(szLine, linelen + 32, Line);

		if (!_strnicmp(szLine, "MQ2EasyFind: Going to ", 22))
		{
			//szLine += 22;
			if (char* pDest = strstr(szLine, "-> "))
			{
				szLine = pDest;
				szLine += 3;
			}
			if (char* pDest = strchr(szLine, '('))
			{
				pDest--;
				pDest[0] = '\0';

			}
			StopMovement();

			if (GetModuleHandle("mq2dannet"))
				DoCommandf("/dgge /easyfind \"%s\"", szLine);
			else if (GetModuleHandle("mq2eqbc"))
				DoCommandf("/bcg //easyfind \"%s\"", szLine);
			else
				WriteChatf("Either mq2eqbc or mq2dannet needs to be loaded to go to %s", szLine);
		}

		LocalFree(szLineOrg);
	}
}

PLUGIN_API bool OnIncomingChat(const char* Line, DWORD Color)
{
	if (gbMimicMe)
	{
		size_t linelen = strlen(Line);
		char* szLine = (char*)LocalAlloc(LPTR, linelen + 32);
		char* szLineOrg = szLine;
		strcpy_s(szLine, linelen + 32, Line);

		if (!_strnicmp(szLine, "You say, '", 10))
		{
			szLine += 10;
			if (char* pDest = strrchr(szLine, '\''))
			{
				pDest[0] = '\0';
			}

			if (!_strnicmp(szLine, "Hail, ", 6))
			{
				DoCommandf(MimicMeHailCommand.c_str());
			}
			else
			{
				DoCommandf("%s %s", MimicMeSayCommand.c_str(), szLine);
			}
		}

		LocalFree(szLineOrg);
	}
	return false;
}

PLUGIN_API void OnZoned()
{
	gbMimicMe = false;

	if (MimicMeButton)
		MimicMeButton->bChecked = false;
	gbFollowme = false;

	if (FollowMeButton)
		FollowMeButton->bChecked = false;
}

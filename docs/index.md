---
tags:
  - plugin
resource_link: "https://www.redguides.com/community/resources/mq2groupinfo.2246/"
support_link: "https://www.redguides.com/community/threads/mq2groupinfo.79138/"
repository: "https://github.com/RedGuides/MQ2GroupInfo"
config: "MQ2GroupInfo.ini"
authors: "eqmule, Knightly, Sic"
tagline: "Extends the group window with buttons for multiboxing."
acknowledgements: "eqmule and MQ2Targetinfo"
---

# MQ2GroupInfo

<!--desc-start-->
New buttons for your group window. Mimic me, come to me and follow me to move your group around, distance to each member in the group window, and 3 handy equipment buttons you can set to whatever slot you wish.
<!--desc-end-->

## Commands

<a href="cmd-groupinfo/">
{% 
  include-markdown "projects/mq2groupinfo/cmd-groupinfo.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2groupinfo/cmd-groupinfo.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2groupinfo/cmd-groupinfo.md') }}

## Requirements

- "Follow Me" requires mq2advpath loaded on everyone
- "Come to Me" requires mq2nav with nav meshes loaded on everyone
- "Mimic Me" and general plugin usage is defaulted to use mq2dannet, so that must be configured and working correctly.

## Settings

Example MQ2GroupInfo.ini,

```ini
[Default]
; Given the number of options, setting this per character will be a lot to maintain
UsePerCharSettings=0

; Disable checks that EQBC or DanNet are loaded before running commands
DisableNetCommandChecks=0

ShowDistance=1
DistanceLabelToolTip=Member Distance

ShowMimicMeButton=1
ShowComeToMeButton=1
ShowFollowMeButton=1
ShowHotButtons=1
```

Customize commands, texts and tooltips are displayed for the "Follow Me" and "Come to Me" buttons.
Default entries are:

```ini
ComeToMeText=Come To Me
ComeToMeCommand=/dgge /multiline ; /afollow off;/nav stop;/timed 5 /nav id ${Me.ID}
;ComeToMeCommand=/bcg //multiline ; /afollow off;/nav stop;/timed 5 /nav id ${Me.ID}
ComeToMeToolTip=/dgge /nav id ${Me.ID}
NavStopCommand=/dgge /nav stop
;NavStopCommand=/bcg //nav stop

FollowMeText=Follow Me
FollowMeCommand=/dgge /multiline ; /afollow off;/nav stop;/timed 5 /afollow spawn ${Me.ID}
;FollowMeCommand=/bcg //multiline ; /afollow off;/nav stop;/timed 5 /afollow spawn ${Me.ID}
FollowMeToolTip=/dgge /afollow spawn ${Me.ID}
FollowStopCommand=/dgge /multiline ; /afollow off;/nav stop
;FollowStopCommand=/bcg //multiline ; /afollow off;/nav stop

MimicMeText=Mimic Me
MimicMeSayCommand=/dgge /say
;MimicMeSayCommand=/bcg //say
MimicMeHailCommand=/dgge /keypress HAIL
;MimicMeHailCommand=/bcg //keypress HAIL
MimicMeToolTip=Everyone do what I do: targeting, hailing, etc.

; This command will have the spawn ID appended to it
TargetCommand=/dgge /target id
;TargetCommand=/bcg //target id
```

Non-default UI's are compatible. If you have a custom UI it will save button locations in its own section of the .ini file. Example:

```ini
[UI_default]
UseGroupLayoutBox=0
LabelBaseGW=Player_ManaLabel

GroupDistanceFontSize=2
GroupDistanceOffset=2
GroupDistanceLoc=0,-20,70,0
; The gauge to write the distance text to: GW_Gauge<x> etc... x will be automatically filled by the plugin
GroupDistanceElementPrefix=Needs to be set when UseGroupLayoutBox is enabled

ComeToMeLoc=61,27,6,46
FollowMeLoc=61,27,48,88
MimicMeLoc=61,27,90,130

HotButton0Loc=97,64,6,46
HotButton1Loc=97,64,49,89
HotButton2Loc=97,64,92,132
DynamicUI=1

[UI_zliz]
; Custom settings built into plugin
DynamicUI=0
GroupDistanceLoc=0,-8,60,0
ComeToMeLoc=41,1,1,41
FollowMeLoc=41,1,42,82
MimicMeLoc=41,1,83,123
HotButton0Loc=76,36,2,42
HotButton1Loc=76,36,44,84
HotButton2Loc=76,36,86,126
; End Built-In Settings

[UI_Melee]
; Custom settings built into plugin
DynamicUI=0
GroupDistanceLoc=-3,-15,120,-70
ComeToMeLoc=36,2,4,44
FollowMeLoc=36,2,46,86
MimicMeLoc=36,2,88,128
HotButton0Loc=36,0,134,174
HotButton1Loc=36,0,174,214
HotButton2Loc=36,0,214,254
; End Built-In Settings

[UI_sars]
; Custom settings built into plugin
DynamicUI=0
UseGroupLayoutBox=1
GroupDistanceOffset=0
GroupDistanceLoc=0,12,70,-5
GroupDistanceElementPrefix=GW_Gauge
ComeToMeLoc=33,3,0,30
FollowMeLoc=33,3,32,62
MimicMeLoc=33,3,64,94
HotButton0Loc=69,39,2,32
HotButton1Loc=69,39,34,64
HotButton2Loc=69,39,66,96
; End Built-In Settings

[UI_Simple_SticeGroup]
; Custom settings built into plugin
GroupDistanceFontSize=1
GroupDistanceLoc=4,-10,70,0
ComeToMeLoc=51,17,6,46
FollowMeLoc=51,17,48,88
MimicMeLoc=51,17,90,130
HotButton0Loc=87,54,6,46
HotButton1Loc=87,54,49,89
HotButton2Loc=87,54,92,132
; End Built-In Settings

[UI_Drakah]
; Custom settings built into plugin
DynamicUI=0
GroupDistanceLoc=-3,-15,80,-30
ComeToMeLoc=36,2,4,44
FollowMeLoc=36,2,46,86
MimicMeLoc=36,2,88,128
HotButton0Loc=36,0,154,194
HotButton1Loc=36,0,194,234
HotButton2Loc=36,0,234,274
; End Built-In Settings

[UI_Freq_SteelDragon]
; Custom settings built into plugin
DynamicUI=0
LabelBaseGW=PW_ManaLabel
GroupDistanceLoc=0,-12,0,30
ComeToMeLoc=41,1,6,46
FollowMeLoc=41,1,48,88
MimicMeLoc=41,1,90,130
HotButton0Loc=77,44,6,46
HotButton1Loc=77,44,49,89
HotButton2Loc=77,44,92,132
; End Built-In Settings

[UI_Sparxx]
; Custom settings built into plugin
DynamicUI=0
GroupDistanceLoc=13,5,100,2
ComeToMeLoc=34,1,2,42
FollowMeLoc=34,1,44,84
MimicMeLoc=34,1,86,126
HotButton0Loc=34,1,128,168
HotButton1Loc=34,1,170,210
HotButton2Loc=34,1,212,252
; End Built-In Settings
```

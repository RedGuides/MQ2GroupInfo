---
tags:
  - command
---

# /groupinfo

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/groupinfo [option] [on|off]
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Triggers the buttons, controls the display of specific buttons, and adjusts other .ini settings.
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `perchar [on|off]` | Toggle splitting settings by character. |
| `cometome` | Imitates pressing the cometome button, which by default sends a command (via mq2eqbc or mq2dannet) for all characters in your group to come to you using `/nav`. |
| `followme [on|off]` | Toggles the followme button, which uses `/afollow` to find you. |
| `mimicme [on|off]` | Toggles the mimicme button, which causes all characters to copy your targets and repeat your words in /say (handy for quests). |
| `show/hide [cometome|followme|mimicme|hot|distance]` | Toggles showing the specified button.<br>- **hot** toggles hotbuttons<br>- **distance** toggles showing the distance to group<br>- **cometome**, **followme**, **mimicme** toggle their respective buttons |
| `disablenetcheck` | Toggles checking EQBC/DanNet commands before issuing them. |
| `reset` | Resets all settings to default. |
| `reload` | Reloads all settings. |

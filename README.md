# CornerCullingSourceEngine

### Introduction
This is the latest in a long line of occlusion culling / anti-wallhack systems.  
There are a few features that make this implementation great for competitive Counter-Strike:
- Open source
- Good performance (1-2% of frame time for 10 v 10 128-tick Dust2)
- Strict culling with ray casts
- Guaranteed to be optimistic (no popping) for players under the latency threshold set in culling.cfg

The main caveat is that occluders are placed manually, so we do not automatically support community maps.  
I suspect that there are still a few placement errors, which cause players to be invisible, so please submit an issue with a video.  
All feedback is welcome!

### Installation
- Install SourceMod  
- Drag the contents of "InstallThis" into csgo-ds/csgo  

### Next Steps
- Polish lookahead logic  
- Find and fill "missed spots" in maps  
- Calculate lookahead with velocity instead of speed  
- Update automatically (perhaps https://forums.alliedmods.net/showthread.php?t=169095)  
- Join occluders, preventing "leaks" through thin corners  
- Anti-anti-flash  
- Smoke occlusion  

### Special Thanks
Paul "arkem" Chamberlain  
Garrett Weinzierl at PlayWin  
DJPlaya, lekobyroxa, and the AlliedModders community  
Challengermode esports platform  
TURF! community servers  

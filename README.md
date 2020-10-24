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

### Adding Occluders to Custom Maps
- Create a file for your custum map, csgo/maps/culling_<MAPNAME>.txt
- Compile and install culling_editor.sp
- To prevent your CS:GO client from crashing, you may have to unload culling_editor until after your client joins
- Get the coordinates of a point by looking at it and attacking. You must be client #1
- An axis-aligned bounding box is declared by "AABB" and defined by the coordinates of two opposite vertices
- A cuboid is declared by "cuboid" and defined by
  - offset
  - scale
  - rotation
  - 8 vertices in the order below
- A cuboid is usually best defined with 8 raw vertex coordinates, "0 0 0" offset, "1 1 1" scale, and "0 0 0" rotation
- The user must ensure that the vertices of a cuboid's faces are coplanar. Failure will cause undefined behavior
- You can loosely check your work with "r_drawothermodels 2" but beware that it is not as rigorous as testing with a real wallhack

```  
   .1------0
 .' |    .'|
2---+--3'  |
|   |  |   |
|  .5--+---4
|.'    | .'
6------7'
```

### Known Issues
- Mirage sandwich

### Future Work
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

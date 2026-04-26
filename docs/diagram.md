flowchart TD

subgraph group_core["Emulation core"]
  node_common["Common runtime<br/>shared infra"]
  node_schedule["Scheduler<br/>timing<br/>[schedule.cpp]"]
  node_buffers["Audio buffers<br/>[soundbuf.cpp]"]
  node_devices["Device emulators<br/>cpu/chips"]
  node_z80["Z80 CPU<br/>cpu core<br/>[Z80_x86.cpp]"]
  node_chips["Sound chips<br/>fm/psg<br/>[opna.cpp]"]
  node_pc88["PC-88 model<br/>machine model<br/>[pc88.cpp]"]
  node_pc88memio["Memory and I/O<br/>bus logic<br/>[memory.cpp]"]
  node_pc88video["Video system<br/>display logic<br/>[crtc.cpp]"]
  node_pc88media["Media control<br/>storage/tape<br/>[diskmgr.cpp]"]
  node_pc88input["Input devices<br/>keyboard/mouse/joy<br/>[joypad.cpp]"]
  node_pc88sound["Sound subsystem<br/>audio mix<br/>[sound.cpp]"]
end

subgraph group_host["Win32 host"]
  node_win32["Win32 app<br/>desktop shell<br/>[main.cpp]"]
  node_ui["UI and dialogs<br/>menus/dialogs<br/>[ui.cpp]"]
  node_render["Draw backends<br/>rendering<br/>[DrawGDI.cpp]"]
  node_audio["Audio drivers<br/>host audio<br/>[winsound.cpp]"]
  node_input["Input bridge<br/>host input<br/>[WinKeyIF.cpp]"]
  node_monitors["Monitors<br/>diagnostics<br/>[winmon.cpp]"]
end

subgraph group_ext["Extensions"]
  node_cdif["CD interface<br/>cd-rom layer<br/>[cdif.cpp]"]
  node_diskdrv["Disk driver<br/>low-level disk<br/>[diskio.cpp]"]
  node_romeo["FM control<br/>external hw<br/>[piccolo_romeo.cpp]"]
  node_iface["Interface headers<br/>API boundary<br/>[ifpc88.h]"]
  node_samples["Sample modules<br/>plugin examples<br/>[moduleif.cpp]"]
end

subgraph group_vendor["Vendored code"]
  node_zlib["zlib<br/>compression lib<br/>[zlib.h]"]
end

node_win32 -->|"controls"| node_pc88
node_pc88 -->|"uses"| node_common
node_pc88 -->|"executes"| node_devices
node_devices -->|"CPU cores"| node_z80
node_devices -->|"chips"| node_chips
node_common -->|"timing"| node_schedule
node_common -->|"buffers"| node_buffers
node_pc88 -->|"bus"| node_pc88memio
node_pc88 -->|"video"| node_pc88video
node_pc88 -->|"media"| node_pc88media
node_pc88 -->|"input"| node_pc88input
node_pc88 -->|"audio"| node_pc88sound
node_win32 -->|"presents"| node_ui
node_win32 -->|"renders via"| node_render
node_win32 -->|"plays via"| node_audio
node_win32 -->|"captures"| node_input
node_win32 -->|"diagnostics"| node_monitors
node_render -->|"consumes"| node_pc88video
node_audio -->|"consumes"| node_buffers
node_input -->|"injects"| node_pc88input
node_cdif -->|"feeds"| node_pc88media
node_diskdrv -->|"feeds"| node_pc88media
node_romeo -->|"extends"| node_pc88sound
node_iface -->|"contracts"| node_win32
node_iface -->|"used by"| node_samples
node_zlib -->|"supports"| node_common

click node_common "https://github.com/rururutan/m88/tree/master/src/common"
click node_schedule "https://github.com/rururutan/m88/blob/master/src/common/schedule.cpp"
click node_buffers "https://github.com/rururutan/m88/blob/master/src/common/soundbuf.cpp"
click node_devices "https://github.com/rururutan/m88/tree/master/src/devices"
click node_z80 "https://github.com/rururutan/m88/blob/master/src/devices/Z80_x86.cpp"
click node_chips "https://github.com/rururutan/m88/blob/master/src/devices/opna.cpp"
click node_pc88 "https://github.com/rururutan/m88/blob/master/src/pc88/pc88.cpp"
click node_pc88memio "https://github.com/rururutan/m88/blob/master/src/pc88/memory.cpp"
click node_pc88video "https://github.com/rururutan/m88/blob/master/src/pc88/crtc.cpp"
click node_pc88media "https://github.com/rururutan/m88/blob/master/src/pc88/diskmgr.cpp"
click node_pc88input "https://github.com/rururutan/m88/blob/master/src/pc88/joypad.cpp"
click node_pc88sound "https://github.com/rururutan/m88/blob/master/src/pc88/sound.cpp"
click node_win32 "https://github.com/rururutan/m88/blob/master/src/win32/main.cpp"
click node_ui "https://github.com/rururutan/m88/blob/master/src/win32/ui.cpp"
click node_render "https://github.com/rururutan/m88/blob/master/src/win32/DrawGDI.cpp"
click node_audio "https://github.com/rururutan/m88/blob/master/src/win32/winsound.cpp"
click node_input "https://github.com/rururutan/m88/blob/master/src/win32/WinKeyIF.cpp"
click node_monitors "https://github.com/rururutan/m88/blob/master/src/win32/winmon.cpp"
click node_cdif "https://github.com/rururutan/m88/blob/master/cdif/src/cdif.cpp"
click node_diskdrv "https://github.com/rururutan/m88/blob/master/diskdrv/src/diskio.cpp"
click node_romeo "https://github.com/rururutan/m88/blob/master/src/win32/romeo/piccolo_romeo.cpp"
click node_iface "https://github.com/rururutan/m88/blob/master/src/if/ifpc88.h"
click node_samples "https://github.com/rururutan/m88/blob/master/sample1/src/moduleif.cpp"
click node_zlib "https://github.com/rururutan/m88/blob/master/src/zlib/zlib.h"

classDef toneNeutral fill:#f8fafc,stroke:#334155,stroke-width:1.5px,color:#0f172a
classDef toneBlue fill:#dbeafe,stroke:#2563eb,stroke-width:1.5px,color:#172554
classDef toneAmber fill:#fef3c7,stroke:#d97706,stroke-width:1.5px,color:#78350f
classDef toneMint fill:#dcfce7,stroke:#16a34a,stroke-width:1.5px,color:#14532d
classDef toneRose fill:#ffe4e6,stroke:#e11d48,stroke-width:1.5px,color:#881337
classDef toneIndigo fill:#e0e7ff,stroke:#4f46e5,stroke-width:1.5px,color:#312e81
classDef toneTeal fill:#ccfbf1,stroke:#0f766e,stroke-width:1.5px,color:#134e4a
class node_common,node_schedule,node_buffers,node_devices,node_z80,node_chips,node_pc88,node_pc88memio,node_pc88video,node_pc88media,node_pc88input,node_pc88sound toneBlue
class node_win32,node_ui,node_render,node_audio,node_input,node_monitors toneAmber
class node_cdif,node_diskdrv,node_romeo,node_iface,node_samples toneMint
class node_zlib toneRose

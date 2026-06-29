# Reversa BO3Enhanced Training Pass

- Generated: `2026-06-28T23:59:32.8484711-05:00`
- BO3Enhanced root: `\\wsl.localhost\Ubuntu\home\richtofen\.android\repositories\gaming\BO3Enhanced`
- BO3Enhanced commit: `3d54e5c5545fa5eb7091c4ea6b531faf75c10764`
- BO3-Transformed root: `\\wsl.localhost\Ubuntu\home\richtofen\.android\repositories\gaming\BO3-Transformed`
- Required runtime: `BO3Enhanced full runtime lane`
- Compatibility rule: Partner menu compatibility is valid only after Reversa detects the BO3Enhanced lane or an explicitly selected BO3Enhanced test profile.

## Training Passes

- `pass1` root: `\\wsl.localhost\Ubuntu\home\richtofen\.android\repositories\gaming\BO3Enhanced`
  commit: `3d54e5c5545fa5eb7091c4ea6b531faf75c10764`
  source files scanned: `31`
- `pass2` root: `\\wsl.localhost\Ubuntu\home\richtofen\.android\repositories\gaming\upstream-bo3\shiversoftdev-bo3enhanced`
  commit: `3d54e5c5545fa5eb7091c4ea6b531faf75c10764`
  source files scanned: `31`

## Safety Boundary

Train on BO3Enhanced runtime/menu/stability contracts. Port only behavior that is required, understood, testable, and compatible with BO3-Transformed.

## Lessons

### friend_menu_required_runtime

- Lesson: The partner menu depends on BO3Enhanced as a full runtime contract, not just a few copied offsets.
- Transformed action: Detect the BO3Enhanced lane explicitly and keep menu compatibility features behind that detected contract.
- Evidence hits: `28`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `14`
  - `pass2` hits: `14`
  - `pass1:dllmain.cpp:1468` MDT_Define_FASTCALL(REBASE(0x1F009E0), Live_SystemInfo_Hook, bool, (int controllerIndex, int infoType, char* outputString, const int outputLen))
  - `pass1:dllmain.cpp:1472` return MDT_ORIGINAL(Live_SystemInfo_Hook, (controllerIndex, infoType, outputString, outputLen));
  - `pass1:dllmain.cpp:1476` snprintf(outputString, outputLen, "%s (Wine)", VERSION_STRING);
  - `pass1:dllmain.cpp:1478` strcpy_s(outputString, outputLen, VERSION_STRING);
  - `pass1:dllmain.cpp:1550` MDT_Activate(Live_SystemInfo_Hook);
  - `pass2:dllmain.cpp:1802` ifs.open(WSTORE_MOD_NAME, std::ifstream::in | std::ifstream::binary);
  - `pass2:dllmain.cpp:1806` ALOG("ERROR: COULD NOT FIND OR OPEN %s", WSTORE_MOD_NAME);
  - `pass2:framework.h:4` #define WSINTERNAL_MOD_NAME "T7InternalWS.dll"
  - `pass2:framework.h:5` #define WSTORE_MOD_NAME "WSBlackOps3.exe"
  - `pass2:framework.h:9` #define VERSION_STRING "BO3Enhanced v1.16"
  - `pass2:steamugc.cpp:195` ifT7InternalWS.open(WSINTERNAL_MOD_NAME, std::ios::binary);
  - `pass2:steamugc.cpp:199` ALOG("FAILED to open '%s', cannot check for updates.", WSINTERNAL_MOD_NAME);
  - `pass1:T7WSBootstrapper\dllmain.cpp:11` if (std::filesystem::copy_file(WSTORE_UPDATER_DEST_FILENAME, WSINTERNAL_MOD_NAME, std::filesystem::copy_options::overwrite_existing))
  - `pass2:T7WSBootstrapper\dllmain.cpp:27` LoadLibraryA(WSINTERNAL_MOD_NAME);

### friend_menu_loader_contract

- Lesson: BO3Enhanced enters through a bootstrapper/import boundary that loads T7InternalWS and handles update handoff.
- Transformed action: Train Reversa to treat T7WSBootstrapper/T7InternalWS/WSBlackOps3 as one lane instead of independent optional files.
- Evidence hits: `32`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `16`
  - `pass2` hits: `16`
  - `pass2:dllmain.cpp:1986` EXPORT void dummy_proc() {}
  - `pass2:framework.h:4` #define WSINTERNAL_MOD_NAME "T7InternalWS.dll"
  - `pass2:MemoryModule.h:141` * Default implementation of CustomLoadLibraryFunc that calls LoadLibraryA
  - `pass1:README.md:41` 6. You can use the provided `WindowsCodecs.dll` from the [releases tab](https://github.com/shiversoftdev/BO3Enhanced/releases/tag/Current), or if you wish you can use [CFF Explorer](https://ntcore.com/explorer-suite-iii-cff-explorer-vii/) to add an import to your system32/WindowsCodecs.dll -- Import should be by name from `T7WSBootstrapper.dll`, function name `dummy_proc`.
  - `pass1:steamugc.cpp:195` ifT7InternalWS.open(WSINTERNAL_MOD_NAME, std::ios::binary);
  - `pass2:steamugc.cpp:199` ALOG("FAILED to open '%s', cannot check for updates.", WSINTERNAL_MOD_NAME);
  - `pass1:T7WSBootstrapper\dllmain.cpp:5` extern "C" __declspec(dllexport) void dummy_proc() { }
  - `pass1:T7WSBootstrapper\dllmain.cpp:7` void check_update_moves()
  - `pass2:T7WSBootstrapper\dllmain.cpp:11` if (std::filesystem::copy_file(WSTORE_UPDATER_DEST_FILENAME, WSINTERNAL_MOD_NAME, std::filesystem::copy_options::overwrite_existing))
  - `pass1:T7WSBootstrapper\dllmain.cpp:26` check_update_moves();
  - `pass1:T7WSBootstrapper\dllmain.cpp:27` LoadLibraryA(WSINTERNAL_MOD_NAME);
  - `pass1:T7WSBootstrapper\T7WSBootstrapper.vcxproj:25` <RootNamespace>T7WSBootstrapper</RootNamespace>
  - `pass1:T7WSBootstrapper\T7WSBootstrapper.vcxproj:80` <PreprocessorDefinitions>WIN32;_DEBUG;T7WSBOOTSTRAPPER_EXPORTS;_WINDOWS;_USRDLL;%(PreprocessorDefinitions)</PreprocessorDefinitions>
  - `pass1:T7WSBootstrapper\T7WSBootstrapper.vcxproj:97` <PreprocessorDefinitions>WIN32;NDEBUG;T7WSBOOTSTRAPPER_EXPORTS;_WINDOWS;_USRDLL;%(PreprocessorDefinitions)</PreprocessorDefinitions>
  - `pass1:T7WSBootstrapper\T7WSBootstrapper.vcxproj:114` <PreprocessorDefinitions>_DEBUG;T7WSBOOTSTRAPPER_EXPORTS;_WINDOWS;_USRDLL;%(PreprocessorDefinitions)</PreprocessorDefinitions>
  - `pass2:T7WSBootstrapper\T7WSBootstrapper.vcxproj:131` <PreprocessorDefinitions>NDEBUG;T7WSBOOTSTRAPPER_EXPORTS;_WINDOWS;_USRDLL;%(PreprocessorDefinitions)</PreprocessorDefinitions>

### friend_menu_config_contract

- Lesson: The runtime contract includes file-backed config for player name, friends-only, and network password behavior.
- Transformed action: Prefer file-backed runtime state and do not rely on Steam launch environment inheritance for menu-critical switches.
- Evidence hits: `68`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `34`
  - `pass2` hits: `34`
  - `pass1:bdAuthXB1toSteam.cpp:59` strncpy((char *)requestData + 0x18, gSteamPlayername, 0x40);
  - `pass2:security.cpp:8` sec_config security::conf{};
  - `pass1:security.cpp:155` if (security::conf.update_watcher_time(PATCH_CONFIG_LOCATION))
  - `pass1:security.cpp:157` security::conf.loadfrom(PATCH_CONFIG_LOCATION);
  - `pass1:security.cpp:166` if (!security::conf.fs_exists(PATCH_CONFIG_LOCATION))
  - `pass1:security.cpp:168` security::conf.saveto(PATCH_CONFIG_LOCATION);
  - `pass1:security.cpp:172` security::conf.loadfrom(PATCH_CONFIG_LOCATION);
  - `pass2:security.cpp:240` bool sec_config::fs_exists(const char* filename)
  - `pass1:security.cpp:253` bool sec_config::update_watcher_time(const char* path)
  - `pass1:security.cpp:271` void sec_config::loadfrom(const char* path)
  - `pass1:security.cpp:300` case FNV32("playername"):
  - `pass2:security.cpp:306` std::strcpy(playername, val.data());
  - `pass1:security.cpp:307` SALOG("Read playername from config: %s", playername);
  - `pass1:security.cpp:310` case FNV32("isfriendsonly"):
  - `pass1:security.cpp:324` SALOG("Read isfriendsonly from config: %u", is_friends_only);
  - `pass1:security.cpp:327` case FNV32("networkpassword"):

### friend_menu_stability_contract

- Lesson: BO3Enhanced carries crash-hardening and private-session behavior that the menu expects to coexist with.
- Transformed action: Do not run the partner menu as if plain Steam BO3 and BO3Enhanced have the same safety/runtime surface.
- Evidence hits: `26`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `13`
  - `pass2` hits: `13`
  - `pass2:README.md:22` - This project includes support for the T7Patch linux config file. After first boot you can edit the config for a network password and will be able to play with other T7Patch players if you wish. The name editor requires a reboot.
  - `pass2:README.md:23` - This version of the game is still vulnerable to all the remote crashes from the Steam version, but the easy RCEs are not an issue (accidental by Treyarch). I recommend using network password and friends only.
  - `pass2:security.cpp:34` MDT_Define_FASTCALL(REBASE(0x14F6710), dwInstantDispatchMessage_Internal_hook, bool, (XUID senderID, const uint32_t controllerIndex, void* message, uint32_t messageSize))
  - `pass2:security.cpp:91` // friends only is only supported for the steam version of the game at this time
  - `pass2:security.cpp:116` return MDT_ORIGINAL(dwInstantDispatchMessage_Internal_hook, (senderID, controllerIndex, message, messageSize));
  - `pass2:security.cpp:119` MDT_Define_FASTCALL(REBASE(0x2246240), Sys_Checksum_hook, uint16_t, (const unsigned char* msg, uint32_t size))
  - `pass1:security.cpp:121` auto res = MDT_ORIGINAL(Sys_Checksum_hook, (msg, size));
  - `pass1:security.cpp:176` MDT_Define_FASTCALL(REBASE(0x1FF8E70), LobbyMsgRW_PrepWriteMsg_Hook, bool, (msg_t* lobbyMsg, __int64 data, int length, int msgType))
  - `pass1:security.cpp:178` if (MDT_ORIGINAL(LobbyMsgRW_PrepWriteMsg_Hook, (lobbyMsg, data, length, msgType)))
  - `pass1:security.cpp:206` MDT_Activate(dwInstantDispatchMessage_Internal_hook);
  - `pass1:security.cpp:207` MDT_Activate(Sys_Checksum_hook);
  - `pass1:security.cpp:209` MDT_Activate(LobbyMsgRW_PrepWriteMsg_Hook);
  - `pass2:security.cpp:341` SALOG("Read network password from config!");

### runtime_lane

- Lesson: Treat BO3Enhanced as the required partner-menu runtime lane when that menu is in use.
- Transformed action: Keep Steam/T7/Reversa and BO3Enhanced manifests separated, but allow Reversa to train on and report the BO3Enhanced lane.
- Evidence hits: `50`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `25`
  - `pass2` hits: `25`
  - `pass2:dllmain.cpp:1276` ALOG("Registering BO3Enhanced lua functions...");
  - `pass2:dllmain.cpp:1278` MDT_ORIGINAL(hksI_openlib_hook, (s, "BO3Enhanced", lib, 0, 1));
  - `pass2:dllmain.cpp:1476` snprintf(outputString, outputLen, "%s (Wine)", VERSION_STRING);
  - `pass1:dllmain.cpp:1478` strcpy_s(outputString, outputLen, VERSION_STRING);
  - `pass1:dllmain.cpp:1851` // copy the tls data from windows store binary into our current thread tls memory region
  - `pass2:dllmain.cpp:1855` // copy the data from windows store into our dll's tls region
  - `pass1:dllmain.cpp:1894` // invoke TLS of windows store bo3
  - `pass1:dllmain.cpp:1961` return TRUE; // this is not the windows store executable we expect
  - `pass2:framework.h:5` #define WSTORE_MOD_NAME "WSBlackOps3.exe"
  - `pass2:framework.h:9` #define VERSION_STRING "BO3Enhanced v1.16"
  - `pass1:README.md:1` # BO3Enhanced
  - `pass1:README.md:3` [BO3Enhanced](https://github.com/shiversoftdev/BO3Enhanced/releases/tag/Current) is a mod for the Windows Store edition of Black Ops 3 that allows you to crossplay with Steam and use the Steam game files.
  - `pass2:README.md:9` Download: https://github.com/shiversoftdev/BO3Enhanced/releases/tag/Current
  - `pass2:README.md:14` 2. Acquire the [Windows Store BO3 Executable and Dependencies](https://www.youtube.com/watch?v=rBZZTcSJ9_s)
  - `pass2:README.md:15` 3. Place the Windows Store BO3 Executable and Dependencies into the Steam game installation folder (Steam Library->Right Click Black Ops III->Properties->Installed Files->Browse) (Replace all if it asks)
  - `pass1:README.md:16` 4. Download the [Released Files](https://github.com/shiversoftdev/BO3Enhanced/releases/tag/Current) and place them into the Steam game installation folder (Replace all if it asks)

### avoid_hot_path_platform_work

- Lesson: Cache platform/friends work and avoid refreshing it during gameplay hot paths.
- Transformed action: Keep overlay, Steam, display, and process telemetry idle unless visible or explicitly capturing.
- Evidence hits: `76`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `38`
  - `pass2` hits: `38`
  - `pass2:dllmain.cpp:19` // FUTURE MAYBE: friends list
  - `pass1:security.cpp:11` uint64_t next_update_friendslist_time = 0;
  - `pass2:security.cpp:13` bool IsFriendByXUIDUncached(XUID xuid) // ok I say its "uncached" but thats because I don't want this running a billion times per second and I think it might hitch with huge friends lists.
  - `pass1:security.cpp:15` if (GetTickCount64() < next_update_friendslist_time || !s_runningUILevel) // we will just not update the friends list in game because I really think this will hitch. STEAMAPI SUCKS
  - `pass2:security.cpp:20` int num_friends = SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
  - `pass2:security.cpp:25` CSteamID out_friend = SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
  - `pass2:security.cpp:29` next_update_friendslist_time = GetTickCount64() + 30 * 1000; // once every 30 seconds
  - `pass1:steam.cpp:85` gSteamPlayername = SteamFriends()->GetPersonaName();
  - `pass1:steam.cpp:539` SteamFriends()->ActivateGameOverlayToUser("friendadd", CSteamID(xuid));
  - `pass2:steam.cpp:542` SteamFriends()->ActivateGameOverlayToUser("steamid", CSteamID(xuid));
  - `pass1:steam.cpp:694` validate_and_copy_name(SteamFriends()->GetFriendPersonaName(steamID), info->name);
  - `pass1:steam.cpp:700` if (SteamFriends()->GetFriendPersonaState(steamID) == k_EPersonaStateOffline)
  - `pass1:steam.cpp:711` if (!SteamFriends()->GetFriendGamePlayed(steamID, &currentGame) ||
  - `pass2:steam.cpp:724` const char* state_str = SteamFriends()->GetFriendRichPresence(steamID, "state");
  - `pass1:steam.cpp:725` const char *tActivity_str = SteamFriends()->GetFriendRichPresence(steamID, "tActivity");
  - `pass1:steam.cpp:726` const char* tCtx_str = SteamFriends()->GetFriendRichPresence(steamID, "tCtx");

### callback_threading

- Lesson: Move platform callbacks to a controlled background cadence rather than doing everything in-frame.
- Transformed action: Keep Reversa telemetry publication non-blocking and avoid waiting on locks in Present paths.
- Evidence hits: `10`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `5`
  - `pass2` hits: `5`
  - `pass2:steam.cpp:479` DWORD steam_callbacks_thread(void *arg)
  - `pass2:steam.cpp:483` SteamAPI_RunCallbacks();
  - `pass1:steam.cpp:494` SteamNetworking()->ReadP2PPacket(incomingPacketBuf, sizeof(incomingPacketBuf), &incomingPacketSize, &incomingPacketFrom);
  - `pass1:steam.cpp:498` SteamNetworking()->ReadP2PPacket(incomingPacketBuf, sizeof(incomingPacketBuf), &incomingPacketSize, &incomingPacketFrom);
  - `pass2:steam.cpp:890` CreateThread(NULL, NULL, steam_callbacks_thread, NULL, NULL, NULL);

### frame_dispatcher_budget

- Lesson: Frame-event systems need hard budget discipline; every-frame hooks should schedule rare work out of band.
- Transformed action: Use delayed/idle timers for overlay refresh and reserve frame-linked logic for lightweight counters only.
- Evidence hits: `22`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `11`
  - `pass2` hits: `11`
  - `pass1:dllmain.cpp:435` void dispatch_events()
  - `pass1:dllmain.cpp:462` void register_frame_event(std::function<void()> lambda)
  - `pass2:dllmain.cpp:467` void set_delay(uint32_t delayMS, uint32_t eventID, std::function<void()> lambda)
  - `pass2:dllmain.cpp:1520` dispatch_events();
  - `pass2:dllmain.cpp:1681` register_frame_event([]()
  - `pass1:dllmain.cpp:1692` set_delay(1000, DELAYED_EVENT_PROCESSORAFFINITY, []()
  - `pass2:framework.h:1075` void register_frame_event(std::function<void()> lambda);
  - `pass1:framework.h:1076` void set_delay(uint32_t delayMS, uint32_t eventID, std::function<void()> lambda);
  - `pass1:steam.cpp:115` void steam_dispatch_every_frame()
  - `pass1:steam.cpp:892` register_frame_event([]()
  - `pass2:steam.cpp:894` steam_dispatch_every_frame();

### config_watcher

- Lesson: Config changes should be file-backed and watched outside the render path.
- Transformed action: Prefer local config/marker files for Reversa test switches over Steam environment inheritance.
- Evidence hits: `32`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `16`
  - `pass2` hits: `16`
  - `pass2:security.cpp:151` DWORD WINAPI watch_settings_updates(_In_ LPVOID lpParameter)
  - `pass1:security.cpp:155` if (security::conf.update_watcher_time(PATCH_CONFIG_LOCATION))
  - `pass2:security.cpp:157` security::conf.loadfrom(PATCH_CONFIG_LOCATION);
  - `pass1:security.cpp:168` security::conf.saveto(PATCH_CONFIG_LOCATION);
  - `pass1:security.cpp:172` security::conf.loadfrom(PATCH_CONFIG_LOCATION);
  - `pass2:security.cpp:220` CreateThread(NULL, NULL, watch_settings_updates, NULL, NULL, NULL);
  - `pass2:security.cpp:253` bool sec_config::update_watcher_time(const char* path)
  - `pass1:security.cpp:271` void sec_config::loadfrom(const char* path)
  - `pass1:security.cpp:278` update_watcher_time(path);
  - `pass2:security.cpp:362` update_watcher_time(path);
  - `pass1:security.cpp:365` void sec_config::saveto(const char* path)
  - `pass1:security.cpp:372` update_watcher_time(path);
  - `pass1:security.cpp:380` update_watcher_time(path);
  - `pass1:security.h:19` bool update_watcher_time(const char* path);
  - `pass1:security.h:20` void loadfrom(const char* path);
  - `pass2:security.h:21` void saveto(const char* path);

### wine_detection

- Lesson: Detect Wine/Linux explicitly and isolate compatibility hooks from Windows-native paths.
- Transformed action: Keep Vulkan/RM11Pro profile switches separate from Windows-native D3D11/T7 behavior.
- Evidence hits: `28`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `14`
  - `pass2` hits: `14`
  - `pass2:dllmain.cpp:1475` if (is_on_linux())
  - `pass1:linux_helper.cpp:1` // TODO(Emma): remove these as WineGDK becomes more mature.
  - `pass1:linux_helper.cpp:31` void do_linux_hooks()
  - `pass2:linux_helper.cpp:38` static const char* (*wine_get_version)(void) = NULL;
  - `pass1:linux_helper.cpp:43` bool is_on_linux()
  - `pass2:linux_helper.cpp:47` return (wine_get_version != NULL);
  - `pass2:linux_helper.cpp:54` wine_get_version = (const char *(*)(void))GetProcAddress(ntdll, "wine_get_version");
  - `pass1:linux_helper.cpp:55` if (wine_get_version == NULL)
  - `pass1:linux_helper.cpp:61` const char* version = wine_get_version();
  - `pass1:linux_helper.h:5` void do_linux_hooks();
  - `pass2:linux_helper.h:6` bool is_on_linux();
  - `pass1:steam.cpp:838` if (is_on_linux())
  - `pass1:steam.cpp:862` if (is_on_linux()) {
  - `pass2:steam.cpp:864` do_linux_hooks();

### loader_manifest

- Lesson: Use a small loader/manifest boundary for update and DLL handoff behavior.
- Transformed action: Keep reversa-wrapper-stack.json and deploy hash checks as the source of truth for test state.
- Evidence hits: `20`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `10`
  - `pass2` hits: `10`
  - `pass2:MemoryModule.h:141` * Default implementation of CustomLoadLibraryFunc that calls LoadLibraryA
  - `pass1:README.md:41` 6. You can use the provided `WindowsCodecs.dll` from the [releases tab](https://github.com/shiversoftdev/BO3Enhanced/releases/tag/Current), or if you wish you can use [CFF Explorer](https://ntcore.com/explorer-suite-iii-cff-explorer-vii/) to add an import to your system32/WindowsCodecs.dll -- Import should be by name from `T7WSBootstrapper.dll`, function name `dummy_proc`.
  - `pass2:T7WSBootstrapper\dllmain.cpp:7` void check_update_moves()
  - `pass2:T7WSBootstrapper\dllmain.cpp:26` check_update_moves();
  - `pass1:T7WSBootstrapper\dllmain.cpp:27` LoadLibraryA(WSINTERNAL_MOD_NAME);
  - `pass1:T7WSBootstrapper\T7WSBootstrapper.vcxproj:25` <RootNamespace>T7WSBootstrapper</RootNamespace>
  - `pass1:T7WSBootstrapper\T7WSBootstrapper.vcxproj:80` <PreprocessorDefinitions>WIN32;_DEBUG;T7WSBOOTSTRAPPER_EXPORTS;_WINDOWS;_USRDLL;%(PreprocessorDefinitions)</PreprocessorDefinitions>
  - `pass1:T7WSBootstrapper\T7WSBootstrapper.vcxproj:97` <PreprocessorDefinitions>WIN32;NDEBUG;T7WSBOOTSTRAPPER_EXPORTS;_WINDOWS;_USRDLL;%(PreprocessorDefinitions)</PreprocessorDefinitions>
  - `pass1:T7WSBootstrapper\T7WSBootstrapper.vcxproj:114` <PreprocessorDefinitions>_DEBUG;T7WSBOOTSTRAPPER_EXPORTS;_WINDOWS;_USRDLL;%(PreprocessorDefinitions)</PreprocessorDefinitions>
  - `pass2:T7WSBootstrapper\T7WSBootstrapper.vcxproj:131` <PreprocessorDefinitions>NDEBUG;T7WSBOOTSTRAPPER_EXPORTS;_WINDOWS;_USRDLL;%(PreprocessorDefinitions)</PreprocessorDefinitions>

### affinity_experiment

- Lesson: CPU affinity can change pacing, but it is hardware-sensitive and needs A/B captures before promotion.
- Transformed action: Add affinity as an opt-in experiment only after overlay-idle and BO3Enhanced executable baselines.
- Evidence hits: `10`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `5`
  - `pass2` hits: `5`
  - `pass1:dllmain.cpp:1692` set_delay(1000, DELAYED_EVENT_PROCESSORAFFINITY, []()
  - `pass2:dllmain.cpp:1699` if (!GetProcessAffinityMask(GetCurrentProcess(), &processAffinity, &systemAffinity))
  - `pass2:dllmain.cpp:1718` if (!SetProcessAffinityMask(GetCurrentProcess(), affinity))
  - `pass1:dllmain.cpp:1730` if (!SetProcessAffinityMask(GetCurrentProcess(), processAffinity))
  - `pass1:framework.h:1121` #define DELAYED_EVENT_PROCESSORAFFINITY 1

### compatibility_boundary

- Lesson: Separate menu-required stability/runtime features from unrelated debugger, DRM, or stealth internals.
- Transformed action: Train on the BO3Enhanced contract and port only compatibility-facing behavior that is needed, understood, and testable.
- Evidence hits: `122`
- Confidence: `confirmed_by_all_passes` (2/2 passes)
  - `pass1` hits: `61`
  - `pass2` hits: `61`
  - `pass2:bdAuthXB1toSteam.cpp:120` MDT_Define_FASTCALL(REBASE(0x29a81e0), bdAntiCheat_reportExtendedAuthInfo_Hook, void*, (void* a, void* b, int gameMode, int gameVersion, uint64_t checksum, uint8_t* macaddr, uint64_t internaladdr, uint64_t externaladdr, void* authinfo))
  - `pass1:bdAuthXB1toSteam.cpp:122` return MDT_ORIGINAL(bdAntiCheat_reportExtendedAuthInfo_Hook, (a, b, gameMode, BD_VERSION_LARP, checksum, macaddr, internaladdr, externaladdr, authinfo));
  - `pass1:bdAuthXB1toSteam.cpp:125` MDT_Define_FASTCALL(REBASE(0x29a7db0), bdAntiCheat_reportConsoleDetails_Hook, void*, (void* a, void* b, int gameMode, int gameVersion, uint64_t checksum, uint8_t* macaddr, uint64_t internaladdr, uint64_t externaladdr, void* consoleid))
  - `pass1:bdAuthXB1toSteam.cpp:127` return MDT_ORIGINAL(bdAntiCheat_reportConsoleDetails_Hook, (a, b, gameMode, BD_VERSION_LARP, checksum, macaddr, internaladdr, externaladdr, consoleid));
  - `pass1:bdAuthXB1toSteam.cpp:160` // fix the version number in bdAntiCheat
  - `pass2:bdAuthXB1toSteam.cpp:161` MDT_Activate(bdAntiCheat_reportConsoleDetails_Hook);
  - `pass2:bdAuthXB1toSteam.cpp:162` MDT_Activate(bdAntiCheat_reportExtendedAuthInfo_Hook);
  - `pass1:dllmain.cpp:1055` MDT_Define_FASTCALL(REBASE(0x23C0110), anticheat_sus_peepo_hook, void, ())
  - `pass2:dllmain.cpp:1057` SPOOFED_CALL(((void(__fastcall*)())MDT_ORIGINAL_PTR(anticheat_sus_peepo_hook)));
  - `pass2:dllmain.cpp:1060` MDT_Define_FASTCALL(REBASE(0x22649C0), anticheat_sus_peepo2_hook, void, ()) // probably anticheat pump
  - `pass2:dllmain.cpp:1062` SPOOFED_CALL(((void(__fastcall*)())MDT_ORIGINAL_PTR(anticheat_sus_peepo2_hook)));
  - `pass2:dllmain.cpp:1065` MDT_Define_FASTCALL(REBASE(0x22C0140), anticheat_unk3_hook, uint64_t, (uint64_t a1, uint64_t a2))
  - `pass2:dllmain.cpp:1068` auto res = SPOOFED_CALL(((uint64_t(__fastcall*)(uint64_t, uint64_t))MDT_ORIGINAL_PTR(anticheat_unk3_hook)), a1, a2);
  - `pass1:dllmain.cpp:1073` MDT_Define_FASTCALL(REBASE(0x22BE810), anticheat_unk4_hook, uint64_t, (uint64_t a1))
  - `pass2:dllmain.cpp:1076` auto res = SPOOFED_CALL(((uint64_t(__fastcall*)(uint64_t))MDT_ORIGINAL_PTR(anticheat_unk4_hook)), a1);
  - `pass1:dllmain.cpp:1081` MDT_Define_FASTCALL(REBASE(0x22BD4C0), anticheat_unk5_hook, uint64_t, (uint64_t a1))

## ConsoleIX/Menu Contract

### consoleix_process_contract

- Lesson: ConsoleIX currently attaches to Steam BlackOps3.exe and T7InternalWS directly.
- Transformed action: Teach the menu bridge to resolve Steam BO3 and BO3Enhanced lanes independently before trusting offsets.
- Evidence hits: `4`
  - `ConsoleIX.cpp:1036` procId = GetProcId(L"BlackOps3.exe");
  - `ConsoleIX.cpp:1043` moduleBase = GetModuleBaseAddress(procId, L"BlackOps3.exe");
  - `ConsoleIX.cpp:1044` DllBase = GetModuleBaseAddress(procId, L"T7InternalWS.dll");
  - `ConsoleIX.cpp:1045` CaptionAddr = DllBase + 0x50000;

### consoleix_bank_guard_contract

- Lesson: Bank writes must remain behind map, weapon, and resolved-address guards.
- Transformed action: Keep bank/table pattern scanning ahead of signature fallback, and invalidate BankAddr on map or weapon changes.
- Evidence hits: `53`
  - `ConsoleIX.cpp:188` static uintptr_t BankAddr = 0; // move these to file/class scope if not already
  - `ConsoleIX.cpp:223` uintptr_t moduleBase = 0, DllBase = 0, HookLastByteAddr = 0, HookClanByte = 0, SigCamoAddr = 0, ModIdAddr = 0, TestAddr = 0, LeftPistolAddr = 0, AmmoBaseAddr = 0, NameAddr = 0, NameBaseAddr = 0, MapIdAddr = 0, ConnectionIDAddr = 0, WeaponInHandAddr = 0, TableBase = 0, SvCheatsAddr = 0, DeveloperAddr = 0, TWeaponAddr = 0, LocalPlayerOffset = 0, ZombiesCountAddr = 0, CamoAddr1 = 0, CamoAddr2 = 0, CamoAddr3 = 0, CamoAddr4 = 0, CamoAddr5 = 0, XCoordAddr = 0, YCoordAddr = 0, ZCoordAddr = 0, NoclipAddr = 0, GoldTU8EHealthAddr = 0, GoldTU8EBaseAddr = 0, GoldTU8ENameAddr = 0, GodBaseAddr = 0, GodAddr = 0, MWeaponAddr = 0, QWeaponAddr = 0, WWeaponAddr = 0, EWeaponAddr = 0, ScoreAddr = 0, ScoreBaseAddr = 0, ClassCamoBaseAddr = 0, ClassCamoAddr = 0;
  - `ConsoleIX.cpp:231` MapIdAddr = ConnectionIDAddr = WeaponInHandAddr = TableBase = 0;
  - `ConsoleIX.cpp:239` ClipIdAddr = SpiritTimerAddr = BankAddr = 0;
  - `ConsoleIX.cpp:475` DWORD WINAPI ScanThread(LPVOID lpParam)
  - `ConsoleIX.cpp:481` uintptr_t found = FindPatternEx(
  - `ConsoleIX.cpp:487` printf("[SCAN] FindPatternEx result: 0x%llX\n", found);
  - `ConsoleIX.cpp:491` BankAddr = found - 0x28;
  - `ConsoleIX.cpp:493` printf("[SCAN] BankAddr set: 0x%llX\n", BankAddr);
  - `ConsoleIX.cpp:504` DWORD WINAPI SpiritScanThread(LPVOID lpParam)
  - `ConsoleIX.cpp:510` uintptr_t found = FindPatternEx(
  - `ConsoleIX.cpp:516` printf("[SPIRIT SCAN] FindPatternEx result: 0x%llX\n", found);

### consoleix_independent_visual_contract

- Lesson: Clan/caption/camo visual features have their own target validation and should not be blocked by bank weapon/map guards.
- Transformed action: Bridge hookClanByte and camoByte through independent address validation, not the bank-specific guard chain.
- Evidence hits: `124`
  - `ConsoleIX.cpp:54` //BO3Enhanced Rainbow
  - `ConsoleIX.cpp:175` bool bHealth = true, bAmmo = false, bRewardGiven = false, bCamoFlag = false, bName = false, bCamo = false, bNameToggle = false, bSvCheats = false, bDebug = false, bInstant = false, bNoclip = false, bToggleMW = false, bToggleQW = false, bToggleWW = false, bToggleEW = false, bToggleScore = false, bToggleAbilityDamage = false, bToggleSpeed = false, bToggleJump = false, bRainbow = false, bRainbow2 = false, bRainbow3 = false, bRainbowBypass = false, bRainbowCycle = false, bTest = false, bTest2 = false, bTest3 = false, bReviveGoldTU8E = false, bGoldTU8EHealth = false, bUnlimitedGrenades = false, bGoldTU8EFlag = true, bRapid = false;
  - `ConsoleIX.cpp:176` bool bClan = false, isPatched = false, bBank = false, bSpirit = false;
  - `ConsoleIX.cpp:178` std::thread rainbowThread;
  - `ConsoleIX.cpp:180` std::thread rainbowThread2;
  - `ConsoleIX.cpp:182` std::thread rainbowThread3;
  - `ConsoleIX.cpp:223` uintptr_t moduleBase = 0, DllBase = 0, HookLastByteAddr = 0, HookClanByte = 0, SigCamoAddr = 0, ModIdAddr = 0, TestAddr = 0, LeftPistolAddr = 0, AmmoBaseAddr = 0, NameAddr = 0, NameBaseAddr = 0, MapIdAddr = 0, ConnectionIDAddr = 0, WeaponInHandAddr = 0, TableBase = 0, SvCheatsAddr = 0, DeveloperAddr = 0, TWeaponAddr = 0, LocalPlayerOffset = 0, ZombiesCountAddr = 0, CamoAddr1 = 0, CamoAddr2 = 0, CamoAddr3 = 0, CamoAddr4 = 0, CamoAddr5 = 0, XCoordAddr = 0, YCoordAddr = 0, ZCoordAddr = 0, NoclipAddr = 0, GoldTU8EHealthAddr = 0, GoldTU8EBaseAddr = 0, GoldTU8ENameAddr = 0, GodBaseAddr = 0, GodAddr = 0, MWeaponAddr = 0, QWeaponAddr = 0, WWeaponAddr = 0, EWeaponAddr = 0, ScoreAddr = 0, ScoreBaseAddr = 0, ClassCamoBaseAddr = 0, ClassCamoAddr = 0;
  - `ConsoleIX.cpp:224` uintptr_t TestAddr2 = 0, TableCamoAddr = 0, EntityList = 0, DistanceBetween = 0, HealthAddr = 0, ClipIdAddr = 0, SpiritTimerAddr = 0;
  - `ConsoleIX.cpp:225` uintptr_t CaptionAddr = 0;
  - `ConsoleIX.cpp:229` moduleBase = HookLastByteAddr = HookClanByte = SigCamoAddr = ModIdAddr = 0;
  - `ConsoleIX.cpp:233` ZombiesCountAddr = CamoAddr1 = CamoAddr2 = CamoAddr3 = CamoAddr4 = CamoAddr5 = 0;
  - `ConsoleIX.cpp:237` ScoreAddr = ScoreBaseAddr = ClassCamoBaseAddr = ClassCamoAddr = 0;

## Output

- JSON: `\\wsl.localhost\Ubuntu\home\richtofen\.android\repositories\gaming\BO3-Transformed\profiles\bo3-vulkan\logs\reversa-bo3enhanced-training-20260628-235916.json`
- Stable JSON: `\\wsl.localhost\Ubuntu\home\richtofen\.android\repositories\gaming\BO3-Transformed\profiles\bo3-vulkan\training\bo3enhanced-menu-contract.json`
- Stable Markdown: `\\wsl.localhost\Ubuntu\home\richtofen\.android\repositories\gaming\BO3-Transformed\profiles\bo3-vulkan\training\BO3ENHANCED_MENU_CONTRACT.md`

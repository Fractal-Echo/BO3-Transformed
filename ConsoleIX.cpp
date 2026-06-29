#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <GL/gl.h>
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <stdio.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdint>
#include "proc.h" 
#include "mem.h"
#include "Helpers.h"

//New Attempt
#include <vector>
#include "Signature.h"
#include <string>
#include <iomanip>    
#include <sstream>     
#include <algorithm>  
#include <iostream>
#include <cmath>

#ifdef _MSC_VER
// Portable build with no GLEW dependency.
#pragma comment(lib, "opengl32.lib")
#endif



static bool isCycling = false;    // Flag to control cycling
static int targetIndex = 0;       // Index for cycling through targets
static float cycleInterval = 0.1f;  // Time interval between each target change (in seconds)
static float lastCycleTime = 0.0f; // Tracks when the target last changed


static bool isCycling2 = false;
static int targetIndex2 = 0;
static float cycleInterval2 = 0.1f;
static float lastCycleTime2 = 0.0f;

static bool isCycling3 = false;
static int targetIndex3 = 0;
static float cycleInterval3 = 0.1f;
static float lastCycleTime3 = 0.0f;


//BO3Enhanced Rainbow
std::string targets3[] = {
    "^1BO3Enhanced v1.16", "^2BO3Enhanced v1.16", "^3BO3Enhanced v1.16", "^4BO3Enhanced v1.16",
    "^5BO3Enhanced v1.16", "^6BO3Enhanced v1.16", "^7BO3Enhanced v1.16", "^8BO3Enhanced v1.16", "^9BO3Enhanced v1.16"
};

// List of targets to cycle through

std::string targets2[] = {
    "^1GoldTU8E", "^2GoldTU8E", "^3GoldTU8E", "^4GoldTU8E",
    "^5GoldTU8E", "^6GoldTU8E", "^7GoldTU8E", "^8GoldTU8E", "^9GoldTU8E"
};
/*
std::string targets[] = {
    "^133", "^233", "^333", "^433",
    "^533", "^633", "^733", "^833", "^933"
};
*/

std::string targets[] = {
    "^1SyntaX-_-", "^2SyntaX-_-", "^3SyntaX-_-", "^4SyntaX",
    "^5SyntaX-_-", "^6SyntaX-_-", "^7SyntaX-_-", "^8SyntaX",
    "^9SyntaX-_-"
};

struct Weapon {
    const char* name;
    int id;
};

// List of weapons and their corresponding IDs
static std::vector<Weapon> weaponList = {
    {"Default_Pistol", 1},
    {"Apothicon_Upgraded", 138},
    {"Sliquifier", 282},
    {"Zeus Cannon", 168},
    {"XM53 Packed", 107},
    {"Ray_Gun_Mark_2", 280},
    {"Ray_Gun", 279},
    {"Agent_47", 196},
    {"Mustang & Sally",236}
};


//Tranzit List
static std::vector<Weapon> TranzitWeaponList_1 = {
    {"Default_Pistol", 1},
    {"Apothicon_Upgraded", 138},
    {"MR6", 96},
    {"Zeus Cannon", 168},
    {"Sliq", 325},
    {"Ray_Gun_Mark_2", 273},
    {"Ray_Gun", 271},
    {"Ray_Gun_Mark_3", 153},
    {"Barret M82", 202},
    {"LSAT", 226},
    {"SM1L3R", 266},
    {"Evo", 264},
    {"PDW", 246},
    {"MP5", 240}

};

//Mod List
static std::vector<Weapon> moddedWeaponList_1 = {
    {"Default_Pistol", 1},
    {"Apothicon_Upgraded", 145},
    {"31-79 JGb215", 159},
    {"Zeus Cannon", 178},
    {"Sliq", 325},
    {"Ray_Gun_Mark_2", 316},
    {"Ray_Gun", 314}
};



static int selectedWeaponIndex = 0;

static std::vector<Weapon>* currentWeaponList = &weaponList;


struct Camo {
    const char* name;
    int id;
};

static std::vector<Camo> CamoList = {
    {"Camo_0", 0},
    {"GoldTU8E_Camo", 15},
    {"Dark Aether", 17},
    {"Lava", 26},
    {"Storm", 38},
    {"Ice", 43},
    {"Icy Tempest", 83},
    {"Purple", 74   },
    {"Lightning Blue", 124},
    {"Galaxy", 125},
    {"Pink Plasma", 134},
    {"Gold Mix", 135 },
    {"Ice/White", 136},
    {"Insect Plasma", 137},
    {"Cool Cosmic Mix", 377}
};




static int selectedCamoIndex = 0;

static bool IsAddressValid(uintptr_t addr, HANDLE hProc = nullptr);

bool PatchWeaponID(uintptr_t addr, int id, HANDLE hProcess) {
    if (!IsAddressValid(addr, hProcess))
        return false;
    return mem::TryPatchEx((BYTE*)addr, reinterpret_cast<const BYTE*>(&id), sizeof(id), hProcess);
}

bool PatchCamoID(uintptr_t addr2, int id, HANDLE hProcess) {
    if (!IsAddressValid(addr2, hProcess))
        return false;
    return mem::TryPatchEx((BYTE*)addr2, reinterpret_cast<const BYTE*>(&id), sizeof(id), hProcess);
}
static bool PatchIntIfValid(uintptr_t addr, int value, HANDLE hProc)
{
    if (!IsAddressValid(addr, hProc))
        return false;
    return mem::TryPatchEx((BYTE*)addr, reinterpret_cast<const BYTE*>(&value), sizeof(value), hProc);
}

static bool PatchBytesIfValid(uintptr_t addr, const BYTE* bytes, size_t size, HANDLE hProc)
{
    if (!IsAddressValid(addr, hProc))
        return false;
    return mem::TryPatchEx((BYTE*)addr, bytes, size, hProc);
}

static bool WriteStringIfValid(uintptr_t addr, const char* value, HANDLE hProc)
{
    if (!value || !IsAddressValid(addr, hProc))
        return false;
    return mem::TryWriteStringEx((BYTE*)addr, value, hProc);
}



bool bHealth = false, bAmmo = false, bRewardGiven = false, bCamoFlag = false, bName = false, bCamo = false, bNameToggle = false, bSvCheats = false, bDebug = false, bInstant = false, bNoclip = false, bToggleMW = false, bToggleQW = false, bToggleWW = false, bToggleEW = false, bToggleScore = false, bToggleAbilityDamage = false, bToggleSpeed = false, bToggleJump = false, bRainbow = false, bRainbow2 = false, bRainbow3 = false, bRainbowBypass = false, bRainbowCycle = false, bTest = false, bTest2 = false, bTest3 = false, bReviveGoldTU8E = false, bGoldTU8EHealth = false, bUnlimitedGrenades = false, bGoldTU8EFlag = true, bRapid = false;
bool bClan = false, isPatched = false, bBank = false, bSpirit = false;
bool bHideCaption = false, bModSpoof = false;
std::thread rainbowThread;
std::atomic<bool> stopThread(false);
std::thread rainbowThread2;
std::atomic<bool> stopThread2(false);
std::thread rainbowThread3;
std::atomic<bool> stopThread3(false);

static std::atomic<bool> bScanInProgress(false);
static uint32_t LastMapId = 0;
static bool   bWaitingForMap = true;
static uintptr_t BankAddr = 0; // move these to file/class scope if not already

static std::atomic<bool> bSpiritScanInProgress(false);
static bool   bWaitingForSpirit = true;

static std::atomic<bool> bModScanInProgress(false);
static bool   bWaitingForMod = true;

// ── Address validation & auto-attach helpers ─────────────────────────────────

// Returns true if 'addr' is non-zero and points to committed, readable memory
// in the target process.  Pass hProcess = NULL to skip the VirtualQueryEx check
// (safe before the handle is open).
static bool IsAddressValid(uintptr_t addr, HANDLE hProc)
{
    if (addr == 0)
        return false;
    if (!hProc)
        return false;
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQueryEx(hProc, reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)))
        return false;
    // Must be committed and at least readable
    return (mbi.State == MEM_COMMIT) &&
        (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE |
            PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
            PAGE_EXECUTE_WRITECOPY | PAGE_WRITECOPY)) &&
        !(mbi.Protect & PAGE_GUARD);
}

// Resets every resolved address back to 0 so the attach block re-resolves them
// on the next frame.  Call this whenever the process goes away or an address
// 
DWORD procId;
HANDLE hProcess;
uintptr_t moduleBase = 0, DllBase = 0, HookLastByteAddr = 0, HookClanByte = 0, SigCamoAddr = 0, ModIdAddr = 0, TestAddr = 0, LeftPistolAddr = 0, AmmoBaseAddr = 0, NameAddr = 0, NameBaseAddr = 0, MapIdAddr = 0, ConnectionIDAddr = 0, WeaponInHandAddr = 0, TableBase = 0, SvCheatsAddr = 0, DeveloperAddr = 0, TWeaponAddr = 0, LocalPlayerOffset = 0, ZombiesCountAddr = 0, CamoAddr1 = 0, CamoAddr2 = 0, CamoAddr3 = 0, CamoAddr4 = 0, CamoAddr5 = 0, XCoordAddr = 0, YCoordAddr = 0, ZCoordAddr = 0, NoclipAddr = 0, GoldTU8EHealthAddr = 0, GoldTU8EBaseAddr = 0, GoldTU8ENameAddr = 0, GodBaseAddr = 0, GodAddr = 0, MWeaponAddr = 0, QWeaponAddr = 0, WWeaponAddr = 0, EWeaponAddr = 0, ScoreAddr = 0, ScoreBaseAddr = 0, ClassCamoBaseAddr = 0, ClassCamoAddr = 0;
uintptr_t TestAddr2 = 0, TableCamoAddr = 0, EntityList = 0, DistanceBetween = 0, HealthAddr = 0, ClipIdAddr = 0, SpiritTimerAddr = 0;
uintptr_t CaptionAddr = 0;

enum class BO3RuntimeLane {
    Unknown,
    SteamT7,
    BO3Enhanced
};

struct BO3RuntimeTarget {
    const wchar_t* processName;
    const char* label;
    BO3RuntimeLane lane;
};

static constexpr BO3RuntimeTarget kBO3RuntimeTargets[] = {
    { L"WSBlackOps3.exe", "BO3Enhanced", BO3RuntimeLane::BO3Enhanced },
    { L"BlackOps3.exe", "Steam/T7", BO3RuntimeLane::SteamT7 }
};

static const BO3RuntimeTarget* gRuntimeTarget = nullptr;
static BO3RuntimeLane gRuntimeLane = BO3RuntimeLane::Unknown;

static const char* CurrentRuntimeLabel()
{
    return gRuntimeTarget ? gRuntimeTarget->label : "Unknown";
}

static const wchar_t* CurrentRuntimeProcessName()
{
    return gRuntimeTarget ? gRuntimeTarget->processName : L"BlackOps3.exe";
}

static DWORD ResolveBO3RuntimeProcess(const BO3RuntimeTarget*& outTarget)
{
    for (const BO3RuntimeTarget& target : kBO3RuntimeTargets) {
        DWORD foundProcId = TryGetProcId(target.processName);
        if (foundProcId != 0) {
            outTarget = &target;
            return foundProcId;
        }
    }

    outTarget = nullptr;
    return 0;
}
// probe fails.
static void ResetAddresses()
{
    moduleBase = HookLastByteAddr = HookClanByte = SigCamoAddr = ModIdAddr = 0;
    TestAddr = LeftPistolAddr = AmmoBaseAddr = NameAddr = NameBaseAddr = 0;
    MapIdAddr = ConnectionIDAddr = WeaponInHandAddr = TableBase = 0;
    SvCheatsAddr = DeveloperAddr = TWeaponAddr = LocalPlayerOffset = 0;
    ZombiesCountAddr = CamoAddr1 = CamoAddr2 = CamoAddr3 = CamoAddr4 = CamoAddr5 = 0;
    XCoordAddr = YCoordAddr = ZCoordAddr = NoclipAddr = 0;
    GoldTU8EHealthAddr = GoldTU8EBaseAddr = GoldTU8ENameAddr = 0;
    GodBaseAddr = GodAddr = MWeaponAddr = QWeaponAddr = WWeaponAddr = EWeaponAddr = 0;
    ScoreAddr = ScoreBaseAddr = ClassCamoBaseAddr = ClassCamoAddr = 0;
    TestAddr2 = TableCamoAddr = EntityList = DistanceBetween = HealthAddr = 0;
    ClipIdAddr = SpiritTimerAddr = BankAddr = 0;
    LastMapId = 0;
    bWaitingForMap = true;
    bScanInProgress = false;
    bWaitingForSpirit = true;
    bSpiritScanInProgress = false;
    bWaitingForMod = true;
    bModScanInProgress = false;
    printf("[AUTO-ATTACH] All addresses reset.\n");
}

static uint32_t HashMapName(const char* mapName)
{
    uint32_t hash = 0;
    if (!mapName)
        return hash;

    for (int i = 0; mapName[i] != '\0'; ++i)
        hash = hash * 31u + static_cast<unsigned char>(mapName[i]);
    return hash;
}

static bool IsBankMapId(uint32_t mapId)
{
    constexpr uint32_t kDieRiseMapId = 3596349812u; // zm_die
    constexpr uint32_t kTranzitMapId = 3596365463u; // zm_tra
    return mapId == kDieRiseMapId || mapId == kTranzitMapId;
}

static bool IsBankWeaponId(int weaponId)
{
    return weaponId == 246 || weaponId == 241;
}

static bool TryReadBankContext(char (&mapName)[16], uint32_t& mapId, int& weaponId)
{
    mapName[0] = '\0';
    mapId = 0;
    weaponId = 0;

    if (!hProcess || !IsAddressValid(MapIdAddr, hProcess) || !IsAddressValid(MWeaponAddr, hProcess))
        return false;

    SIZE_T bytesRead = 0;
    if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(MapIdAddr), mapName, sizeof(mapName) - 1, &bytesRead) || bytesRead == 0)
        return false;
    mapName[sizeof(mapName) - 1] = '\0';
    mapId = HashMapName(mapName);

    bytesRead = 0;
    if (!ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(MWeaponAddr), &weaponId, sizeof(weaponId), &bytesRead) ||
        bytesRead != sizeof(weaponId))
        return false;

    return true;
}

static void LogBankGuardBlocked(const char* source, const char* reason)
{
    static ULONGLONG lastLogTick = 0;
    const ULONGLONG now = GetTickCount64();
    if (now - lastLogTick < 1500)
        return;

    lastLogTick = now;
    printf("[BANK GUARD] %s blocked: %s\n", source ? source : "bank write", reason ? reason : "unknown");
}

static bool TryPatchBankValue(int value, const char* source)
{
    char mapName[16]{};
    uint32_t mapId = 0;
    int weaponId = 0;

    if (!TryReadBankContext(mapName, mapId, weaponId)) {
        LogBankGuardBlocked(source, "map/weapon address is not readable");
        return false;
    }
    if (!IsBankMapId(mapId)) {
        LogBankGuardBlocked(source, "map is not Tranzit or Die Rise");
        return false;
    }
    if (!IsBankWeaponId(weaponId)) {
        LogBankGuardBlocked(source, "weapon is not valid for bank action");
        return false;
    }
    if (!IsAddressValid(BankAddr, hProcess)) {
        LogBankGuardBlocked(source, "BankAddr is not valid");
        return false;
    }

    return mem::TryPatchEx(reinterpret_cast<BYTE*>(BankAddr), reinterpret_cast<const BYTE*>(&value), sizeof(value), hProcess);
}

// ─────────────────────────────────────────────────────────────────────────────



ImVec4 RainbowColor(float frequency, float phase, float amplitude) {
    const float time = static_cast<float>(ImGui::GetTime());
    const float r = sinf(frequency * time + 0.0f + phase) * amplitude + 0.5f;
    const float g = sinf(frequency * time + 2.0f + phase) * amplitude + 0.5f;
    const float b = sinf(frequency * time + 4.0f + phase) * amplitude + 0.5f;
    return ImVec4(r, g, b, 1.0f);
}


BYTE CamoID = 0;

int MWeaponId = 0;
int QWeaponId = 0;
int WWeaponId = 0;
int EWeaponId = 0;
int ScoreId = 0;
int AbilityDamageId = 0;
int SpeedId = 0;
int JumpHeightId = 0;
int SoulChipsId = 0;
int BoltsId = 0;
int MemoriaId = 0;
int RainbowValue = 0;
int RainbowValue2 = 0;
int Aug1 = 0;
int JumpCount = 0;
int GoldTU8EHealth = 0;
int ON = 285587035;
int OFF = 694749996;
uint32_t D1 = 3302114845u;
int D2 = 617776669;
int A3 = 0;
int m1 = 256;
int m0 = 0;
int mx = 999;


float posX1 = -5406.315918f;
float posY1 = 76.51004028f;
float posZ1 = 3006.536133f;
float posX2 = 1024.260742f;
float posY2 = -62.875f;
float posZ2 = -594.0992432f;
float posX3 = -5827.96582f;
float posY3 = -34.76357269f;
float posZ3 = -6427.724121f;

int sp = 1132068864;
int sp0 = 1132068864;






static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void RainbowCycleLoop() {
    while (!stopThread.load()) {//87 = original valuev


        for (int RainbowValue = 1; RainbowValue <= 126; ++RainbowValue) {
            if (stopThread.load()) break;
            PatchIntIfValid(CamoAddr2, RainbowValue, hProcess);
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Rainbow Delay //100 MS original value
        }
    }
}

void RainbowCycleLoop2() {
    while (!stopThread2.load()) {
        for (int RainbowValue2 = 1; RainbowValue2 <= 126; ++RainbowValue2) {
            if (stopThread2.load()) break;
            PatchIntIfValid(TableCamoAddr, RainbowValue2, hProcess);
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Rainbow Delay //100 MS original value
        }
    }
}

void RainbowCycleLoop3() {
    while (!stopThread3.load()) {
        for (int RainbowValue3 = 1; RainbowValue3 <= 126; ++RainbowValue3) {
            if (stopThread3.load()) break;
            PatchIntIfValid(CamoAddr5, RainbowValue3, hProcess);
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Rainbow Delay //100 MS original value
        }
    }
}









void DrawDebugWatermark(HANDLE hProcess, uintptr_t MWeaponAddr)
{
    if (!bTest2)
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (!drawList)
        return;


    int weaponID = 0;
    if (MWeaponAddr && hProcess)
    {
        ReadProcessMemory(hProcess, (LPCVOID)MWeaponAddr, &weaponID, sizeof(weaponID), nullptr);
    }


    char buffer[64];
    snprintf(buffer, sizeof(buffer), "W-ID = %d", weaponID);


    ImVec2 textSize = ImGui::CalcTextSize(buffer);
    ImVec2 pos((io.DisplaySize.x - textSize.x) * 0.5f,
        (io.DisplaySize.y - textSize.y) * 0.5f);


    float time = static_cast<float>(ImGui::GetTime());
    float r = sinf(time * 1.5f + 0.0f) * 0.5f + 0.5f;
    float g = sinf(time * 1.5f + 2.0f) * 0.5f + 0.5f;
    float b = sinf(time * 1.5f + 4.0f) * 0.5f + 0.5f;
    ImU32 rainbowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1.0f));
    ImU32 shadowColor = IM_COL32(0, 0, 0, 150);


    drawList->AddText(ImVec2(pos.x + 1, pos.y + 1), shadowColor, buffer);
    drawList->AddText(pos, rainbowColor, buffer);
}

void DrawDebugWatermark2(HANDLE hProcess, uintptr_t CamoAddr2)
{
    if (!bTest3)
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (!drawList)
        return;


    int CamoID = 0;
    if (CamoAddr2 && hProcess)
    {
        ReadProcessMemory(hProcess, (LPCVOID)CamoAddr2, &CamoID, sizeof(CamoID), nullptr);
    }


    char buffer[64];
    snprintf(buffer, sizeof(buffer), "Camo-ID = %d", CamoID);


    ImVec2 textSize = ImGui::CalcTextSize(buffer);
    ImVec2 pos((io.DisplaySize.x - textSize.x) * 0.5f,
        (io.DisplaySize.y - textSize.y) * 0.5f);


    float time = static_cast<float>(ImGui::GetTime());
    float r = sinf(time * 1.5f + 0.0f) * 0.5f + 0.5f;
    float g = sinf(time * 1.5f + 2.0f) * 0.5f + 0.5f;
    float b = sinf(time * 1.5f + 4.0f) * 0.5f + 0.5f;
    ImU32 rainbowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1.0f));
    ImU32 shadowColor = IM_COL32(0, 0, 0, 150);


    drawList->AddText(ImVec2(pos.x + 1, pos.y + 1), shadowColor, buffer);
    drawList->AddText(pos, rainbowColor, buffer);
}





void DrawTextWatermark(const char* watermarkText)
{
    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    ImVec2 padding(10.0f, 10.0f);
    ImVec2 textSize = ImGui::CalcTextSize(watermarkText);


    // ImVec2 pos(io.DisplaySize.x - textSize.x - padding.x, padding.y);
    // ImVec2 pos(padding.x, padding.y);
    ImVec2 pos(io.DisplaySize.x - textSize.x - padding.x,
        io.DisplaySize.y - textSize.y - padding.y);




    float time = static_cast<float>(ImGui::GetTime());
    float r = sinf(time * 1.5f + 0.0f) * 0.5f + 0.5f;
    float g = sinf(time * 1.5f + 2.0f) * 0.5f + 0.5f;
    float b = sinf(time * 1.5f + 4.0f) * 0.5f + 0.5f;
    ImU32 rainbowColor = ImGui::ColorConvertFloat4ToU32(ImVec4(r, g, b, 1.0f));
    ImU32 shadowColor = IM_COL32(0, 0, 0, 150);


    drawList->AddText(ImVec2(pos.x + 1, pos.y + 1), shadowColor, watermarkText);

    drawList->AddText(pos, rainbowColor, watermarkText);
}



// Add this struct at the top of your file with your other globals
struct ScanParams
{
    HANDLE  hProc;
};
static ScanParams g_ScanParams;

struct BankScanResult
{
    uintptr_t address = 0;
    const char* source = "none";
};

static BankScanResult ResolveBankAddressByBankPattern(HANDLE hProc)
{
    uintptr_t found = FindPatternEx(
        hProc,
        "\x9C\x50\xE5\xCB\x00\x00\x00\x00",
        "xxxxxxxx"
    );

    printf("[BANK SCAN] bank pattern result: 0x%llX\n", found);
    if (found == 0)
        return {};

    return { found - 0x28, "bank-pattern" };
}

static BankScanResult ResolveBankAddressByTablePattern(HANDLE hProc)
{
    uintptr_t found = FindPatternEx(
        hProc,
        "\xCD\x2C\xA8\x44\xAC\x4C\xB2\x43\xBE\xFF\x74\x43",
        "xxxxxxxxxxxx"
    );

    printf("[BANK SCAN] table pattern probe result: 0x%llX\n", found);
    if (found != 0) {
        printf("[BANK SCAN] table pattern found, but no verified table-to-bank offset is configured yet.\n");
    }
    return {};
}

static BankScanResult ResolveBankAddressByFallbackSignature(HANDLE)
{
    printf("[BANK SCAN] signature fallback skipped; no verified bank fallback signature is configured yet.\n");
    return {};
}

static BankScanResult ResolveBankAddress(HANDLE hProc)
{
    BankScanResult result = ResolveBankAddressByBankPattern(hProc);
    if (result.address != 0)
        return result;

    result = ResolveBankAddressByTablePattern(hProc);
    if (result.address != 0)
        return result;

    return ResolveBankAddressByFallbackSignature(hProc);
}

// Add this plain function near your other thread functions (RainbowCycleLoop etc)
DWORD WINAPI ScanThread(LPVOID lpParam)
{
    ScanParams* p = (ScanParams*)lpParam;

    Sleep(2000);

    BankScanResult result = ResolveBankAddress(p->hProc);
    if (result.address != 0)
    {
        BankAddr = result.address;
        bWaitingForMap = false;
        printf("[BANK SCAN] BankAddr set from %s: 0x%llX\n", result.source, BankAddr);
    }
    else
    {
        printf("[BANK SCAN] BankAddr not resolved - will retry next eligible frame\n");
    }

    bScanInProgress = false;
    return 0;
}

DWORD WINAPI SpiritScanThread(LPVOID lpParam)
{
    ScanParams* p = (ScanParams*)lpParam;

    Sleep(2000);

    uintptr_t found = FindPatternEx(
        p->hProc,
        "\x00\x16\x88\x42\x55\x00\x00\x00\x00\x00\x00\x00\x00",
        "xxxxxxxxx????"
    );

    printf("[SPIRIT SCAN] FindPatternEx result: 0x%llX\n", found);

    if (found != 0)
    {
        SpiritTimerAddr = found -= 0x26;
        bWaitingForSpirit = false;
        printf("[SPIRIT SCAN] SpiritTimerAddr set: 0x%llX\n", SpiritTimerAddr);
    }
    else
    {
        printf("[SPIRIT SCAN] Pattern not found - will retry next frame\n");
    }

    bSpiritScanInProgress = false;
    return 0;
}

DWORD WINAPI ModScanThread(LPVOID lpParam)
{
    ScanParams* p = (ScanParams*)lpParam;

    Sleep(2000);

    uintptr_t found = FindPatternEx(
        p->hProc,
        "\xA5\x9B\x56\xDF\x00\x00\x00\x00\x87",
        "xxxxxxxxx"
    );

    printf("[MOD SCAN] FindPatternEx result: 0x%llX\n", found);

    if (found != 0)
    {
        ModIdAddr = found;
        bWaitingForMod = false;
        printf("[MOD SCAN] ModIdAddr set: 0x%llX\n", ModIdAddr);
    }
    else
    {
        printf("[MOD SCAN] Pattern not found - will retry next frame\n");
    }

    bModScanInProgress = false;
    return 0;
}



int main(int, char**) {
    system("color 4");

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    GLFWmonitor* Monitor = glfwGetPrimaryMonitor();
    if (!Monitor)
    {
        return 0;
    }

    int Width = glfwGetVideoMode(Monitor)->width;
    int Height = glfwGetVideoMode(Monitor)->height;

    glfwWindowHint(GLFW_FLOATING, true);
    glfwWindowHint(GLFW_RESIZABLE, false);
    glfwWindowHint(GLFW_MAXIMIZED, true);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, true);

    GLFWwindow* window = glfwCreateWindow(Width, Height, "X Menu", nullptr, nullptr);
    if (window == nullptr)
        return 1;

    glfwSetWindowAttrib(window, GLFW_DECORATED, false);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    bool bMenuVisible = true;

    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        DrawTextWatermark("X v0.1.0");
        DrawDebugWatermark(hProcess, MWeaponAddr);
        DrawDebugWatermark2(hProcess, CamoAddr2);



        if (GetAsyncKeyState(VK_F5))
        {
            PatchBytesIfValid(XCoordAddr, reinterpret_cast<const BYTE*>(&posX1), sizeof(posX1), hProcess);
            PatchBytesIfValid(YCoordAddr, reinterpret_cast<const BYTE*>(&posY1), sizeof(posY1), hProcess);
            PatchBytesIfValid(ZCoordAddr, reinterpret_cast<const BYTE*>(&posZ1), sizeof(posZ1), hProcess);
        }


        if (GetAsyncKeyState(VK_F6))
        {
            PatchBytesIfValid(XCoordAddr, reinterpret_cast<const BYTE*>(&posX2), sizeof(posX2), hProcess);
            PatchBytesIfValid(YCoordAddr, reinterpret_cast<const BYTE*>(&posY2), sizeof(posY2), hProcess);
            PatchBytesIfValid(ZCoordAddr, reinterpret_cast<const BYTE*>(&posZ2), sizeof(posZ2), hProcess);
        }

        if (GetAsyncKeyState(VK_F7))
        {
            PatchBytesIfValid(XCoordAddr, reinterpret_cast<const BYTE*>(&posX3), sizeof(posX3), hProcess);
            PatchBytesIfValid(YCoordAddr, reinterpret_cast<const BYTE*>(&posY3), sizeof(posY3), hProcess);
            PatchBytesIfValid(ZCoordAddr, reinterpret_cast<const BYTE*>(&posZ3), sizeof(posZ3), hProcess);
        }



        if (GetAsyncKeyState(VK_INSERT) & 1) {
            bMenuVisible = !bMenuVisible;
            if (bMenuVisible) {
                ShowMenu(window);
            }
            else {
                HideMenu(window);
            }
        }





        if (bMenuVisible) {
            ImGui::Begin("X V0.1.0");
            ImGui::Checkbox("Health", &bHealth);
            ImGui::Checkbox("Unlimited Ammo", &bAmmo);
            ImGui::Checkbox("Instant-Kill", &bInstant);
            ImGui::Checkbox("Rapid-Fire", &bRapid);
            ImGui::Checkbox("Fly-Hack", &bTest);
            ImGui::Checkbox("Weapon-Hotkeys", &bTest2);
            ImGui::Checkbox("Camo-Hotkeys", &bTest3);
            ImGui::Checkbox("GoldTU8E", &bGoldTU8EHealth);
            ImGui::Checkbox("Sv_Cheats", &bSvCheats);
            ImGui::Checkbox("Name", &bName);
            ImGui::Checkbox("Rainbow-Tag", &bClan);
            ImGui::Checkbox("Unlimited Spirit", &bSpirit);
            ImGui::Checkbox("bCaption", &bHideCaption);
            ImGui::Checkbox("bModSpoof", &bModSpoof);
            // ImGui::Checkbox("EntityList", &bClan);
            ImGui::Checkbox("Galaxy", &bCamo);
            ImGui::InputInt("Primary-Weapon", &MWeaponId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused()) {
                bToggleMW = true;
            }
            else
            {
                bToggleMW = false;
            }

            ImGui::InputInt("Secondary-Weapon", &QWeaponId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused()) {
                bToggleQW = true;
            }
            else
            {
                bToggleQW = false;
            }

            ImGui::InputInt("Third-Weapon", &WWeaponId);
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused()) {
                bToggleWW = true;
            }
            else
            {
                bToggleWW = false;
            }

            ImGui::InputInt("Score", &ScoreId);

            bToggleScore = (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::IsItemFocused());


            if (ImGui::Button("Max Bank"))
            {
                bBank = !bBank;
                TryPatchBankValue(bBank ? m1 : m0, "Max Bank toggle");
            }


            static bool spiritPatched = false;
            if (bSpirit && SpiritTimerAddr != 0)
            {
                spiritPatched = PatchIntIfValid(SpiritTimerAddr, sp, hProcess) || spiritPatched;
            }
            else if (spiritPatched)
            {
                if (PatchIntIfValid(SpiritTimerAddr, sp0, hProcess))
                    spiritPatched = false;
            }

            /*
            int level = 1;
            ImGui::SliderInt("Level", &level, 1, 55);

            int xp = 0;

            if (ImGui::SliderInt("XP", &xp, 0, 100000))
            {
                // Snap to nearest 1000
                xp = (xp / 1000) * 1000;
            }
            */


            /*
            int currentModId = 0;
            if (hProcess && ModIdAddr != 0) {
                ReadProcessMemory(hProcess, (LPCVOID)ModIdAddr, &currentModId, sizeof(currentModId), nullptr);

                switch (currentModId) {
                case 1634562661: // Usermaps
                    currentWeaponList = &weaponList;
                    break;
                case 909390899: // Wonder Weapon Pack Mod ID
                    currentWeaponList = &moddedWeaponList_1;
                    break;
                default:
                    currentWeaponList = &weaponList;
                    break;
                }
            }
            */






            int currentMapId = 0;
            if (hProcess && MapIdAddr != 0) {
                ReadProcessMemory(hProcess, (LPCVOID)MapIdAddr, &currentMapId, sizeof(currentMapId), nullptr);

                switch (currentMapId) {
                case 1683975546: // Die Rise
                    currentWeaponList = &weaponList;
                    break;
                case 1952411002: // Tranzit
                    currentWeaponList = &TranzitWeaponList_1;
                    break;
                default:
                    currentWeaponList = &weaponList;
                    break;
                }
            }

            if (currentWeaponList == nullptr || currentWeaponList->empty()) {
                currentWeaponList = &weaponList;
            }
            if (selectedWeaponIndex < 0 || selectedWeaponIndex >= static_cast<int>(currentWeaponList->size())) {
                selectedWeaponIndex = 0;
            }

            ImGui::Text("Weapon Selector");
            if (ImGui::BeginCombo("##WeaponSelector", (*currentWeaponList)[selectedWeaponIndex].name)) {
                for (int i = 0; i < static_cast<int>(currentWeaponList->size()); ++i) {
                    bool isSelected = (selectedWeaponIndex == i);
                    if (ImGui::Selectable((*currentWeaponList)[i].name, isSelected)) {
                        selectedWeaponIndex = i;

                        int weaponID = (*currentWeaponList)[i].id;
                        if (!PatchWeaponID(QWeaponAddr, weaponID, hProcess)) {
                            std::cerr << "Failed to patch weapon ID to process memory!" << std::endl;
                        }
                        else {
                            std::cout << "Patched weapon ID: " << weaponID
                                << " (" << (*currentWeaponList)[i].name << ")\n";
                        }
                    }

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }




            static int weaponID = 1;

            ImGui::Text("Camo Selector");
            ImGui::PushItemWidth(100.0f); // Make combo tiny

            if (ImGui::BeginCombo("##AATSelector", CamoList[selectedCamoIndex].name)) {
                for (int i = 0; i < static_cast<int>(CamoList.size()); ++i) {
                    bool isSelected = (selectedCamoIndex == i);
                    if (ImGui::Selectable(CamoList[i].name, isSelected)) {
                        selectedCamoIndex = i;

                        // Determine which address to patch based on weaponID
                        uintptr_t camoAddrToUse = 0;
                        switch (weaponID) {
                        case 1: camoAddrToUse = CamoAddr1; break;
                        case 2: camoAddrToUse = CamoAddr2; break;
                        case 3: camoAddrToUse = CamoAddr3; break;
                        case 4: camoAddrToUse = CamoAddr4; break;
                        case 5: camoAddrToUse = CamoAddr5; break;
                        default: camoAddrToUse = CamoAddr2; break;
                        }

                        int CamoID = CamoList[i].id;
                        if (!PatchWeaponID(camoAddrToUse, CamoID, hProcess)) {
                            std::cerr << "Failed to patch weapon ID to process memory!" << std::endl;
                        }
                        else {
                            std::cout << "Patched Camo ID: " << CamoID
                                << " (" << CamoList[i].name << ") to Weapon "
                                << weaponID << "\n";
                        }
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();






            // Weapon ID Selector (1–3)
            ImGui::SameLine();
            ImGui::Text("Weapon ID:");
            ImGui::SameLine();
            ImGui::PushItemWidth(50.0f);
            ImGui::SliderInt("##WeaponID", &weaponID, 1, 5);
            ImGui::PopItemWidth();



            if (ImGui::Button("Toggle BO3Enhanced Rainbow"))
            {
                isCycling3 = !isCycling3;
            }

            if (ImGui::Button("Toggle Name Rainbow")) {
                // Toggle the flag when the button is pressed
                isCycling = !isCycling;
                isPatched = !isPatched;



            }



            if (ImGui::Button("Toggle GoldTU8E Rainbow"))
            {
                isCycling2 = !isCycling2;
            }

            static int originalCamoValue = 0;  // holds the saved camo

            if (ImGui::Button("Toggle Rainbow Camo")) {
                bRainbow = !bRainbow;
                if (bRainbow) {

                    ReadProcessMemory(hProcess, (LPCVOID)CamoAddr2, &originalCamoValue, sizeof(originalCamoValue), nullptr);

                    stopThread.store(false);
                    rainbowThread = std::thread(RainbowCycleLoop);
                }
                else {
                    stopThread.store(true);
                    if (rainbowThread.joinable()) {
                        rainbowThread.join();
                    }


                    PatchIntIfValid(CamoAddr2, originalCamoValue, hProcess);
                }
            }


            if (ImGui::Button("Toggle Rainbow Table Camo")) {
                bRainbow2 = !bRainbow2;
                if (bRainbow2) {
                    stopThread2.store(false);
                    rainbowThread2 = std::thread(RainbowCycleLoop2);

                    //mem::PatchEx((BYTE*)(moduleBase + 0x427A75), (BYTE*)"\x75\x0D", 2, hProcess);
                }
                else {
                    stopThread2.store(true);
                    if (rainbowThread2.joinable()) {
                        rainbowThread2.join();
                    }
                    int staticColorValue = 0;
                    PatchIntIfValid(CamoAddr4, staticColorValue, hProcess);
                    //  mem::PatchEx((BYTE*)(moduleBase + 0x427A75), (BYTE*)"\x74\x0D", 2, hProcess);
                }
            }

            if (ImGui::Button("Toggle Rainbow Knife Camo")) {
                bRainbow3 = !bRainbow3;
                if (bRainbow3) {
                    stopThread3.store(false);
                    rainbowThread3 = std::thread(RainbowCycleLoop3);

                    //mem::PatchEx((BYTE*)(moduleBase + 0x427A75), (BYTE*)"\x75\x0D", 2, hProcess);
                }
                else {
                    stopThread3.store(true);
                    if (rainbowThread3.joinable()) {
                        rainbowThread3.join();
                    }
                    int staticColorValue = 0;
                    PatchIntIfValid(CamoAddr5, staticColorValue, hProcess);
                    //  mem::PatchEx((BYTE*)(moduleBase + 0x427A75), (BYTE*)"\x74\x0D", 2, hProcess);
                }
            }








            if (ImGui::Button("Exit")) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

            ImVec4 rainbowTextColor = RainbowColor(1.0f, 0.3f, 1.0f);

            ImGui::TextColored(rainbowTextColor, "*_* != GoldTU8E;");


            ImGui::TextColored(rainbowTextColor, R"(
                   __              __
                   \ `-._......_.-` /
                    `.  '.    .'  .'
                     //  _`\/`_  \\
                    ||  /\O||O/\  ||
                    |\  \_/||\_/  /|
                    \ '.   \/   .' /
                    / ^ `'~  ~'`   \
                   /  _-^_~ -^_ ~-  |
                   | / ^_ -^_- ~_^\ |
                   | |~_ ^- _-^_ -| |
                   | \  ^-~_ ~-_^ / |
                   \_/;-.,____,.-;\_/
            =jgs======(_(_(==)_)_)=========

           ==================================

)");


            ImGui::End();
        }



        // ── Auto re-attach: detect process death, then re-attach when it comes back ──
        {
            bool needReattach = false;

            if (procId != 0 && hProcess != nullptr)
            {
                // Check whether the process is still alive
                DWORD exitCode = STILL_ACTIVE;
                if (!GetExitCodeProcess(hProcess, &exitCode) || exitCode != STILL_ACTIVE)
                {
                    printf("[AUTO-ATTACH] %s closed (exit 0x%X). Waiting for relaunch...\n",
                        CurrentRuntimeLabel(), exitCode);
                    CloseHandle(hProcess);
                    hProcess = nullptr;
                    procId = 0;
                    gRuntimeTarget = nullptr;
                    gRuntimeLane = BO3RuntimeLane::Unknown;
                    ResetAddresses();
                    needReattach = true;
                }
                else
                {
                    // Process alive — spot-check one key resolved address each frame.
                    // If it has become invalid (e.g. map unloaded / ASLR shift) reset
                    // and let the block below re-resolve everything.
                    if (LocalPlayerOffset != 0 && !IsAddressValid(LocalPlayerOffset, hProcess))
                    {
                        printf("[AUTO-ATTACH] LocalPlayerOffset 0x%llX is no longer valid — re-resolving addresses.\n",
                            (unsigned long long)LocalPlayerOffset);
                        ResetAddresses();
                        needReattach = true;   // keep procId/hProcess; only addresses need refresh
                    }
                }
            }

            if (procId == 0) {
                const BO3RuntimeTarget* resolvedTarget = nullptr;
                procId = ResolveBO3RuntimeProcess(resolvedTarget);
                if (procId != 0 && resolvedTarget != nullptr) {
                    gRuntimeTarget = resolvedTarget;
                    gRuntimeLane = resolvedTarget->lane;
                    printf("[AUTO-ATTACH] %s detected as %ls (PID %lu).\n",
                        CurrentRuntimeLabel(), CurrentRuntimeProcessName(), procId);
                }
            }

            if (procId != 0 && (hProcess == nullptr || needReattach)) {
                if (hProcess == nullptr)
                    hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
                if (hProcess) {
                    moduleBase = GetModuleBaseAddress(procId, CurrentRuntimeProcessName());
                    DllBase = GetModuleBaseAddress(procId, L"T7InternalWS.dll");
                    if (moduleBase == 0) {
                        printf("[AUTO-ATTACH] %s module base was not resolved; attach aborted.\n",
                            CurrentRuntimeLabel());
                        CloseHandle(hProcess);
                        hProcess = nullptr;
                        procId = 0;
                        gRuntimeTarget = nullptr;
                        gRuntimeLane = BO3RuntimeLane::Unknown;
                        ResetAddresses();
                        goto AutoAttachDone;
                    }
                    if (DllBase == 0) {
                        printf("[AUTO-ATTACH] T7InternalWS.dll not found for %s; caption/rainbow bridge disabled until it loads.\n",
                            CurrentRuntimeLabel());
                    }
                    CaptionAddr = DllBase != 0 ? DllBase + 0x50000 : 0;
                    EntityList = moduleBase + 0x7DD48D8;
                    DistanceBetween = 0x3088;
                    WeaponInHandAddr = moduleBase + 0x7D953F0;
                    ScoreBaseAddr = moduleBase + 0x09059D58;
                    LocalPlayerOffset = moduleBase + 0x08FA0F78;
                    ZombiesCountAddr = moduleBase + 0xE41A0E0;
                    GodBaseAddr = moduleBase + 0x7DD1850;
                    SvCheatsAddr = moduleBase + 0x19A01090;
                    //EntityList = moduleBase + 0x0FBB8068; Distanced between = 0x8 //0x0 0x8 0x10
                    //BoltsAddr = moduleBase + 0x0;
                   // JumpAddr = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x15D8 });
                  //  ScoreAddr = FindDMAAddy(hProcess, ScoreBaseAddr, { 0x28C });
                    ScoreAddr = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x16E84 });
                    LeftPistolAddr = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x690 });
                    MWeaponAddr = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x3A8 });
                    QWeaponAddr = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x408 });
                    WWeaponAddr = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x3D8 });
                    //PrimaryCamoAddr = FindDMAAddy(hProcess, SecondaryCamoBaseAddr, {  });
                    NoclipAddr = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x16FE4 });
                    GodAddr = FindDMAAddy(hProcess, GodBaseAddr, { 0x2A8 });
                    GoldTU8EBaseAddr = moduleBase + 0x07DD48D8;
                    GoldTU8EHealthAddr = FindDMAAddy(hProcess, GoldTU8EBaseAddr, { 0x2C8 });
                    //  = moduleBase + 0x37C49E0;
                    GoldTU8ENameAddr = moduleBase + 0x37C49E0;
                    // GoldTU8ENameAddr = FindPattern(hProcess, procId, moduleBase, pattern3, mask3);


                    NameBaseAddr = moduleBase + 0x09059D58;
                    NameAddr = FindDMAAddy(hProcess, NameBaseAddr, { 0x2C });
                    ModIdAddr = moduleBase + 0x90FFFD7;//Useful for logic related to weapon id's etc
                    MapIdAddr = moduleBase + 0x90FC4D0;
                    ClipIdAddr = moduleBase + 0x18738BE8;
                    TableBase = moduleBase + 0x03BDCCC8;
                    // CoordBaseAddr = moduleBase + 0x08FA0F78;//Possible LocalPlayer Offset
                    XCoordAddr = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x30 });
                    YCoordAddr = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x38 });
                    ZCoordAddr = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x34 });
                    ClassCamoBaseAddr = moduleBase + 0x19A72BE8;
                    ClassCamoAddr = FindDMAAddy(hProcess, ClassCamoBaseAddr, { 0x128, 0x160, 0x75900 });
                    CamoAddr1 = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x3B0 });
                    CamoAddr2 = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x410 });
                    CamoAddr3 = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x470 });
                    CamoAddr4 = FindDMAAddy(hProcess, TableBase, { 0XAD8 });
                    CamoAddr5 = FindDMAAddy(hProcess, LocalPlayerOffset, { 0x440 });//Knife very nice -_-





                    /*
                    if (MapIdAddr != 0 && MWeaponAddr != 0)
                    {
                        int CurrentMapId2 = 0;
                        int CurrentWeapId2 = 0;

                        ReadProcessMemory(hProcess, (LPCVOID)MapIdAddr, &CurrentMapId2, sizeof(CurrentMapId2), nullptr);
                        ReadProcessMemory(hProcess, (LPCVOID)MWeaponAddr, &CurrentWeapId2, sizeof(CurrentWeapId2), nullptr);

                        printf("MapId: %d | WeapId: %d\n", CurrentMapId2, CurrentWeapId2);  // are these right?

                        if (
                            (CurrentMapId2 == 1683975546 || CurrentMapId2 == 1952411002) &&
                            (CurrentWeapId2 == 246 || CurrentWeapId2 == 241)
                            )
                        {
                            uintptr_t found = FindPatternEx(
                                hProcess,
                                "\x9C\x50\xE5\xCB\x00\x00\x00\x00",
                                "xxxxxxxx"
                            );

                            printf("BankAddr found: 0x%llX\n", found);  // is it 0?

                            if (found != 0)
                                BankAddr = found - 0x28;
                        }
                    }
                    */

                    // Pattern 1
                    /*
                    const BYTE pattern[] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xBD,0x78,0x50,0x54,0x00,0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x16 };
                    const char mask[] = "???????xxx????x????????x";


                    // Find the first byte of this pattern
                    HookLastByteAddr = FindPattern(hProcess, procId, moduleBase, pattern, mask);

                    */
                    /*
                        HookLastByteAddr = FindPattern(hProcess, procId, moduleBase,
                        "\x00\x00\x00\x00\x00\x00\x00\xBD\x78\x50\x54\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x16",
                        "???????xxx????x????????x");
                    */


                    //FindPatternEx(hProcess, "\x00\x00\x00\x00", "?? ?? ?? ?? ?? ?? ?? 00 16 88 42 55 00 00 00 00 ?? ?? ?? ??") //?? ?? ?? ?? ?? ?? ?? 00 16 88 42 55 00 00 00 00 ?? ?? ?? ??
                    /*
                    SpiritTimerAddr = FindPatternEx(
                        hProcess,
                        "\x00\x16\x88\x42\x55\x00\x00\x00\x00\x00\x00\x00\x00",
                        "xxxxxxxxx????"
                    );

                    if (SpiritTimerAddr != 0)
                    {

                        SpiritTimerAddr -= 0x26;
                    }
                    */



                    // HookClanByte = FindPatternEx(hProcess, "\xC4\x03\x00\x00\x00\x00\x74\xBA\x1E\x03", "xxxxxxxxxx");
                    // HookClanByte = FindPatternEx(hProcess, "\x5B\x33\x33\x5D\x5E", "xxxxx");




                     //placeholder = FindPattern(hProcess, procId, moduleBase, "\xC4\x03\x00\x00\x00\x00\x74\xBA\x1E\x03", "xxxxxxxxxx");
                     //HookClanByte = FindPatternEx(hProcess, "\x5E\x33\x33\x33\x5D\x5E", "xxxxxx");
                     //HookClanByte = FindPatternEx(hProcess, "\x5B\x33\x33\x5D\x5E", "xxxxx");

                     // If the pattern is found, add an offset to access the second byte (assuming offset is correct)


                     //11 00 00 00 00 00 00 00 57 98 68 4F 5E

                    /*
                    HookClanByte = FindPatternEx(hProcess, "\x5B\x33\x33\x5D\x5E", "xxxxx");

                    if (HookClanByte != 0) {
                        //  HookClanByte += 0x116;  // use hex offset

                        HookClanByte += 0x4;


                    }
                    */



                    // Debug - remove later



                    //int CurrentMapId2 = 0;
                   // int CurrentWeapId2 = 0;
/*
                    ReadProcessMemory(hProcess, (LPCVOID)MapIdAddr, &CurrentMapId2, sizeof(CurrentMapId2), nullptr);
                    ReadProcessMemory(hProcess, (LPCVOID)MWeaponAddr, &CurrentWeapId2, sizeof(CurrentWeapId2), nullptr);

                    printf("MapId: %d | WeapId: %d\n", CurrentMapId2, CurrentWeapId2);
                    */



                    //This needs isTableBuilt Check? 




                    // TableCamoAddr = FindPatternEx(hProcess, "\xCD\x2C\xA8\x44\xAC\x4C\xB2\x43\xBE\xFF\x74\x43", "xxxxxxxxxxxx");
                    //TableCamoAddr = sig::FindPatternEx(hProcess, "\xCD\x2C\xA8\x44\xAC\x4C\xB2\x43\xBE\xFF\x74\x43", "x?x?x?xx");

                    if (TableCamoAddr != 0) {
                        //TableCamoAddr -= 0x16C;
                        TableCamoAddr -= 0x150;
                    }









                    //  TWeaponAddr = FindDMAAddy(hProcess, TableBase, { 0x954 });




                      //GoldTU8E's name base = blackops3.exe+37C49E0
                }


                std::cout << "X v0.1.0" << std::endl;
                std::cout << "LocalPlayer:" << "0x" << std::hex << LocalPlayerOffset << std::endl;
                std::cout << "Score:" << "0x" << std::hex << ScoreAddr << std::endl;
                std::cout << "Primary Weapon:" << "0x" << std::hex << MWeaponAddr << std::endl;
                std::cout << "Secondary Weapon:" << "0x" << std::hex << QWeaponAddr << std::endl;
                std::cout << "Third Weapon:" << "0x" << std::hex << WWeaponAddr << std::endl;
                std::cout << "HookLastByteAddr: " << std::hex << HookLastByteAddr << std::endl;
                std::cout << "ClanTag sig test: " << std::hex << HookClanByte << std::endl;
                std::cout << "ClanTag sig test2: " << std::hex << TestAddr << std::endl;
                std::cout << "Table Sig test 1" << std::hex << TableCamoAddr << std::endl;
                std::cout << "Spirit Addr" << std::hex << SpiritTimerAddr << std::endl;





                // GoldTU8ERespawnFunction();

            }
        } // end auto re-attach block

        static bool modSpoofPatched = false;
        if (bModSpoof && ModIdAddr == 3446990455)
        {
            uint32_t u = 2502333264u;
            modSpoofPatched = PatchBytesIfValid(ModIdAddr, reinterpret_cast<const BYTE*>(&u), sizeof(u), hProcess) || modSpoofPatched;
        }
        else if (modSpoofPatched)
        {
            uint32_t o = 3446990455u;
            if (PatchBytesIfValid(ModIdAddr, reinterpret_cast<const BYTE*>(&o), sizeof(o), hProcess))
                modSpoofPatched = false;
        }

        static bool captionPatched = false;
        if (bHideCaption && !isCycling3)
        {
            captionPatched = WriteStringIfValid(CaptionAddr, "", hProcess) || captionPatched;
        }
        else if (captionPatched && !isCycling3)
        {
            if (WriteStringIfValid(CaptionAddr, "BO3Enhanced v1.16", hProcess))
                captionPatched = false;
        }
        // if isCycling3 is true, neither block runs



        static bool ammoPatched = false;
        if (bAmmo)
        {
            const bool patchedAmmo = PatchBytesIfValid(moduleBase + 0x27C8234, reinterpret_cast<const BYTE*>("\xC7\x84\x83\x84\x06\x00\x00\x67\x02\x00\x00\x90\x90\x90\x90"), 15, hProcess);

            int A1 = 615;
            const bool patchedPistol = PatchIntIfValid(LeftPistolAddr, A1, hProcess);
            //Unlimited Grenades
            const bool patchedGrenades = PatchBytesIfValid(moduleBase + 0x27C6E9A, reinterpret_cast<const BYTE*>("\x8B\xFF"), 2, hProcess);
            ammoPatched = ammoPatched || (patchedAmmo && patchedPistol && patchedGrenades);
        }
        else if (ammoPatched)
        {
            const bool restoredAmmo = PatchBytesIfValid(moduleBase + 0x27C8234, reinterpret_cast<const BYTE*>("\x83\xBC\x83\x84\x06\x00\x00\x00\x75\x0B\xB8\x01\x00\x00\x00"), 15, hProcess);


            const bool restoredGrenades = PatchBytesIfValid(moduleBase + 0x27C6E9A, reinterpret_cast<const BYTE*>("\x8B\x01"), 2, hProcess);
            if (restoredAmmo && restoredGrenades)
                ammoPatched = false;
        }


        static bool namePatched = false;
        if (bName)
        {
            bNameToggle = true;
            //mem::WriteStringEx((BYTE*)NameAddr, "^1[DivX]SyntaX-_-", hProcess);
            namePatched = WriteStringIfValid(NameAddr, "Danny", hProcess) || namePatched;
        }
        else if (namePatched)
        {
            bNameToggle = false;
            if (WriteStringIfValid(NameAddr, "^1SyntaX-_-", hProcess))
                namePatched = false;
        }


        static bool nameRainbowPatched = false;
        if (isPatched)
        {
            nameRainbowPatched = PatchBytesIfValid(moduleBase + 0x27EB255, reinterpret_cast<const BYTE*>("\x75\x06"), 2, hProcess) || nameRainbowPatched;
        }
        else if (nameRainbowPatched)
        {
            if (PatchBytesIfValid(moduleBase + 0x27EB255, reinterpret_cast<const BYTE*>("\x74\x06"), 2, hProcess))
                nameRainbowPatched = false;
        }

        if (bClan)
        {



            /*
            //size_t Offset = 116;
            BYTE camoValue = 0x42;
           // mem::PatchEx((BYTE*)(HookClanByte ), &camoValue, sizeof(camoValue), hProcess);
            WriteStringIfValid(HookClanByte, "3arc", hProcess);


        */


        /*
        BYTE targetByte = 0x44;
        BYTE buffer[256] = { 0 };

        SIZE_T bytesRead = 0;
        if (ReadProcessMemory(hProcess, (LPCVOID)(HookClanByte + targetByte), buffer, sizeof(buffer), &bytesRead)) {
            for (size_t i = 0; i < bytesRead; ++i) {
                if (buffer[i] == targetByte) {
                    std::cout << "Found 0x44 at offset " << (targetByte + i) << std::endl;
                    break;
                }
            }
        }
        else {
            std::cerr << "Failed to read process memory. Error: " << GetLastError() << std::endl;
        }
        */


        // mem::WriteStringEx((BYTE*)(HookClanByte + Offset), "^133", hProcess);
        }
        else
        {

        }



        ///new signature test
        if (bCamo)
        {
            const size_t lastByteOffset = 23;
            BYTE Galaxy = 0x7D;
            PatchBytesIfValid(HookLastByteAddr + lastByteOffset, &Galaxy, sizeof(Galaxy), hProcess);
        }


        if (GetAsyncKeyState(VK_F4))
        {
            TryPatchBankValue(m1, "F4 bank hotkey");
        }



        // Debug output to see what address we're working with


        // Just output the address without trying to patch yet



        /*
        if (bName)
        {
            std::string fullName = "^1[DivX]";
            mem::WriteStringEx((BYTE*)NameAddr, fullName.c_str(), hProcess);
        }
        else
        {
            std::string baseName = "^1SyntaX-_-";
            mem::WriteStringEx((BYTE*)NameAddr, baseName.c_str(), hProcess);

            // Optionally pad/clear the rest to prevent leftover chars from [DivX]
            char nullBuf[32] = { 0 }; // adjust size to expected name length
            mem::WriteStringEx((BYTE*)NameAddr + baseName.size(), nullBuf, hProcess);
        }
        */

        static bool svCheatsPatched = false;
        if (bSvCheats)
        {
            svCheatsPatched = PatchIntIfValid(SvCheatsAddr, ON, hProcess) || svCheatsPatched;
        }
        else if (svCheatsPatched)
        {
            if (PatchIntIfValid(SvCheatsAddr, OFF, hProcess))
                svCheatsPatched = false;
        }



        static bool rapidPatched = false;
        if (bRapid)//Need logic for burst fire weapons
        {
            const bool patchedRifles = PatchBytesIfValid(moduleBase + 0x27B6353, reinterpret_cast<const BYTE*>("\xC7\x04\x38\x00\x00\x00\x00\x90\x90\x90"), 10, hProcess);
            const bool patchedShotguns = PatchBytesIfValid(moduleBase + 0x27C6519, reinterpret_cast<const BYTE*>("\xC7\x40\x2C\x02\x00\x00\x00\x90\x90"), 9, hProcess);//Shotguns
            rapidPatched = rapidPatched || (patchedRifles && patchedShotguns);


        }
        else if (rapidPatched)
        {
            const bool restoredRifles = PatchBytesIfValid(moduleBase + 0x27B6353, reinterpret_cast<const BYTE*>("\x89\x0C\x38\x44\x38\xAE\x68\x0E\x00\x00"), 10, hProcess);
            const bool restoredShotguns = PatchBytesIfValid(moduleBase + 0x27C6519, reinterpret_cast<const BYTE*>("\x44\x08\x70\x2C\xE8\x5E\xB4\x00\x00"), 9, hProcess);
            if (restoredRifles && restoredShotguns)
                rapidPatched = false;

        }

        static bool instantPatched = false;
        if (bInstant)
        {

            instantPatched = PatchBytesIfValid(moduleBase + 0x26279A5, reinterpret_cast<const BYTE*>("\x0F\x8C"), 2, hProcess) || instantPatched;

        }
        else if (instantPatched)
        {
            if (PatchBytesIfValid(moduleBase + 0x26279A5, reinterpret_cast<const BYTE*>("\x0F\x8F"), 2, hProcess))
                instantPatched = false;
        }





        // Add Logic to keep this off if map id changes  7/20/25 Added

        ///*

        /*
        int currentMapId = 0;
        if (hProcess && MapIdAddr != 0) {
            ReadProcessMemory(hProcess, (LPCVOID)MapIdAddr, &currentMapId, sizeof(currentMapId), nullptr);

            switch (currentMapId) {
            case 1683975546: // Die Rise
                /*
                if (!bRewardGiven)
                {
                    if (ReadProcessMemory(hProcess, (LPCVOID)GoldTU8EHealthAddr, &GoldTU8EHealth, sizeof(GoldTU8EHealth), nullptr))
                    {
                        if (GoldTU8EHealth == 0) // GoldTU8E Dead
                        {
                            // Reward: weapon upgrade
                            int R1 = 264;
                            mem::PatchEx((BYTE*)QWeaponAddr, (BYTE*)&R1, sizeof(R1), hProcess);

                            // Reward: gold camo
                            int camoVal = 15;
                            mem::PatchEx((BYTE*)CamoAddr2, (BYTE*)&camoVal, sizeof(camoVal), hProcess);

                            bRewardGiven = true; // Lock forever
                        }
                    }


                }
                break;
            case 1952411002: // Tranzit

                break;
            default:
                currentWeaponList = &weaponList;
                break;
            }
        }
        */





        //*/





        if (bTest2)
        {
            static SHORT prevF3State = 0;
            static SHORT prevF4State = 0;

            int N1 = 0;
            if (!IsAddressValid(MWeaponAddr, hProcess) ||
                !ReadProcessMemory(hProcess, (LPCVOID)MWeaponAddr, &N1, sizeof(N1), nullptr))
                N1 = 0;
            SHORT currentF3State = GetAsyncKeyState(VK_F3);
            if ((currentF3State & 0x8000) && !(prevF3State & 0x8000))
            {
                N1--;
                PatchIntIfValid(MWeaponAddr, N1, hProcess);
            }
            prevF3State = currentF3State;

            SHORT currentF4State = GetAsyncKeyState(VK_F4);
            if ((currentF4State & 0x8000) && !(prevF4State & 0x8000))
            {
                N1++;
                PatchIntIfValid(MWeaponAddr, N1, hProcess);
            }
            prevF4State = currentF4State;
        }


        if (bTest3)
        {
            static SHORT prevF3State = 0;
            static SHORT prevF4State = 0;

            int N1 = 0;
            if (!IsAddressValid(CamoAddr2, hProcess) ||
                !ReadProcessMemory(hProcess, (LPCVOID)CamoAddr2, &N1, sizeof(N1), nullptr))
                N1 = 0;
            SHORT currentF3State = GetAsyncKeyState(VK_F3);
            if ((currentF3State & 0x8000) && !(prevF3State & 0x8000))
            {
                N1--;
                PatchIntIfValid(CamoAddr2, N1, hProcess);
            }
            prevF3State = currentF3State;

            SHORT currentF4State = GetAsyncKeyState(VK_F4);
            if ((currentF4State & 0x8000) && !(prevF4State & 0x8000))
            {
                N1++;
                PatchIntIfValid(CamoAddr2, N1, hProcess);
            }
            prevF4State = currentF4State;
        }


        if (bTest && NoclipAddr != 0)
        {
            if (GetAsyncKeyState(0x45))  // key is down
            {
                int N1 = 1;
                PatchIntIfValid(NoclipAddr, N1, hProcess);
            }
            else
            {
                int N0 = 0;
                PatchIntIfValid(NoclipAddr, N0, hProcess);
            }
        }

        static bool healthPatched = false;
        if (bHealth && GodAddr != 0) {
            int God = 12297;
            healthPatched = PatchIntIfValid(GodAddr, God, hProcess) || healthPatched;
        }
        else if (healthPatched)
        {
            int God = 12296;
            if (PatchIntIfValid(GodAddr, God, hProcess))
                healthPatched = false;
        }

        static bool goldTU8EHealthPatched = false;
        if (bGoldTU8EHealth && GoldTU8EHealthAddr != 0) {
            const int uHealth = 13337;
            Sleep(5);
            goldTU8EHealthPatched = PatchIntIfValid(GoldTU8EHealthAddr, uHealth, hProcess) || goldTU8EHealthPatched;




        }

        if (isCycling) {
            const float currentTime = static_cast<float>(ImGui::GetTime());  // Get the current time in seconds

            // Only update targetIndex if enough time has passed since the last cycle
            if (currentTime - lastCycleTime >= cycleInterval) {

                targetIndex = (targetIndex + 1) % 9;

                const std::string& currentTarget = targets[targetIndex];

                PatchBytesIfValid(HookClanByte, reinterpret_cast<const BYTE*>(currentTarget.c_str()), currentTarget.size(), hProcess);

                lastCycleTime = currentTime;
            }

        }



        if (isCycling2) {
            const float currentTime2 = static_cast<float>(ImGui::GetTime());  // Get the current time in seconds

            // Only update targetIndex if enough time has passed since the last cycle
            if (currentTime2 - lastCycleTime2 >= cycleInterval2) {

                targetIndex2 = (targetIndex2 + 1) % 9;

                const std::string& currentTarget2 = targets2[targetIndex2];

                PatchBytesIfValid(GoldTU8ENameAddr, reinterpret_cast<const BYTE*>(currentTarget2.c_str()), currentTarget2.size(), hProcess);

                lastCycleTime2 = currentTime2;
            }

        }


        if (isCycling3) {
            const float currentTime3 = static_cast<float>(ImGui::GetTime());  // Get the current time in seconds

            // Only update targetIndex if enough time has passed since the last cycle
            if (currentTime3 - lastCycleTime3 >= cycleInterval3) {

                targetIndex3 = (targetIndex3 + 1) % 9;

                const std::string& currentTarget3 = targets3[targetIndex3];

                PatchBytesIfValid(CaptionAddr, reinterpret_cast<const BYTE*>(currentTarget3.c_str()), currentTarget3.size(), hProcess);

                lastCycleTime3 = currentTime3;
            }

        }

        if (!bGoldTU8EHealth && goldTU8EHealthPatched)
        {
            int UHealth = 100;
            if (PatchIntIfValid(GoldTU8EHealthAddr, UHealth, hProcess))
                goldTU8EHealthPatched = false;
            //mem::PatchEx((BYTE*)(moduleBase + 0x1CFF64), (BYTE*)"\xC7\x86\x70\x02\x00\x00\x08\x00\x00\x00", 10, hProcess);
           // mem::PatchEx((BYTE*)(moduleBase + 0x2BA72F), (BYTE*)"\xFF\x48\x04", 3, hProcess);
        }


        if (bToggleMW && MWeaponAddr != 0)
        {
            PatchIntIfValid(MWeaponAddr, MWeaponId, hProcess);
        }

        if (bToggleQW && QWeaponAddr != 0)
        {
            PatchIntIfValid(QWeaponAddr, QWeaponId, hProcess);
        }

        if (bToggleWW && WWeaponAddr != 0)
        {
            PatchIntIfValid(WWeaponAddr, WWeaponId, hProcess);
        }

        if (bToggleEW && EWeaponAddr != 0)
        {
            PatchIntIfValid(EWeaponAddr, EWeaponId, hProcess);
        }




        if (bToggleScore && ScoreAddr != 0)
        {
            PatchIntIfValid(ScoreAddr, ScoreId, hProcess);
        }
        else
        {

        }
        //Recode this for lobby id detection but it serves it's purpose
        if (bClan)
        {
            /*
            HookClanByte = FindPatternEx(hProcess, "\x5B\x33\x33\x5D\x5E", "xxxxx");

            if (HookClanByte != 0)
            {
                HookClanByte += 0x4;
                ClanFound = true;
            }
            */
        }



        if (MapIdAddr != 0 && MWeaponAddr != 0)
        {
            char CurrentMapStr[16] = {};
            uint32_t CurrentMapId2 = 0;
            int CurrentWeapId2 = 0;
            bool bankContextReady = TryReadBankContext(CurrentMapStr, CurrentMapId2, CurrentWeapId2);

            if (!bankContextReady)
            {
                if (BankAddr != 0)
                {
                    BankAddr = 0;
                    printf("[BANK GUARD] map/weapon context became unreadable | BankAddr reset\n");
                }
                bWaitingForMap = true;
                bScanInProgress = false;
            }

            if (bankContextReady)
            {
                bool bValidMap = IsBankMapId(CurrentMapId2);
                bool bValidWeapon = IsBankWeaponId(CurrentWeapId2);

                if (CurrentMapId2 != LastMapId)
                {
                    BankAddr = 0;
                    LastMapId = CurrentMapId2;
                    bWaitingForMap = true;
                    bScanInProgress = false;
                    // system("cls");
                    printf("[MAP CHANGE] New Map: %s | BankAddr reset\n", CurrentMapStr);
                }

                if (!bValidMap)
                {
                    BankAddr = 0;
                    ClipIdAddr = 7;
                }

                if (!bValidWeapon)
                {
                    if (BankAddr != 0)
                    {
                        BankAddr = 0;
                        //   system("cls");
                        printf("[WEAPON CHANGE] WeapId: %d is not valid | BankAddr reset\n", CurrentWeapId2);
                    }
                    bWaitingForMap = true;
                    bScanInProgress = false;
                }

                if (bWaitingForMap && BankAddr == 0 && bValidMap && bValidWeapon && !bScanInProgress)
                {
                    bScanInProgress = true;
                    g_ScanParams.hProc = hProcess;
                    printf("[BANK SCAN] Conditions met for %s / weapon %d - launching scan thread...\n",
                        CurrentMapStr, CurrentWeapId2);

                    HANDLE hThread = CreateThread(nullptr, 0, ScanThread, &g_ScanParams, 0, nullptr);
                    if (hThread)
                        CloseHandle(hThread);
                }
            }

            // ── Spirit timer scan: zm_prison + weapon 143 ─────────────────────
            bool bPrisonMap = bankContextReady && (strcmp(CurrentMapStr, "zm_prison") == 0);
            //bool bSpiritWeapon = (CurrentWeapId2 == 191);//143 is afterlife hands

            if (!bPrisonMap)
            {
                if (SpiritTimerAddr != 0)
                {
                    SpiritTimerAddr = 0;
                    bWaitingForSpirit = true;
                    bSpiritScanInProgress = false;
                    printf("[SPIRIT] Map or weapon changed — SpiritTimerAddr reset\n");
                }
            }

            if (bWaitingForSpirit && SpiritTimerAddr == 0 && bPrisonMap && !bSpiritScanInProgress)
            {
                bSpiritScanInProgress = true;
                g_ScanParams.hProc = hProcess;
                printf("[SPIRIT SCAN] Conditions met - launching spirit scan thread...\n");

                HANDLE hSpiritThread = CreateThread(nullptr, 0, SpiritScanThread, &g_ScanParams, 0, nullptr);
                if (hSpiritThread)
                    CloseHandle(hSpiritThread);
            }

            //A5 9B 56 DF 00 00 00 00 87
            // ── Mod scan: zm_core (main menu map) ────────────────────────────────
            bool bMenuMap = bankContextReady && (strcmp(CurrentMapStr, "core_frontend") == 0);

            if (!bMenuMap)
            {
                if (ModIdAddr != 0)
                {
                    ModIdAddr = 0;
                    bWaitingForMod = true;
                    bModScanInProgress = false;
                    printf("[MOD SCAN] Map changed — ModIdAddr reset\n");
                }
            }

            if (bWaitingForMod && ModIdAddr == 0 && bMenuMap && !bModScanInProgress)
            {
                bModScanInProgress = true;
                g_ScanParams.hProc = hProcess;
                printf("[MOD SCAN] Conditions met - launching mod scan thread...\n");

                HANDLE hModThread = CreateThread(nullptr, 0, ModScanThread, &g_ScanParams, 0, nullptr);
                if (hModThread)
                    CloseHandle(hModThread);
            }
        }

        AutoAttachDone:
            ;

        ImGui::Render();

        int display_w = 0;
        int display_h = 0;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    stopThread.store(true);
    stopThread2.store(true);
    stopThread3.store(true);

    if (rainbowThread.joinable())
        rainbowThread.join();
    if (rainbowThread2.joinable())
        rainbowThread2.join();
    if (rainbowThread3.joinable())
        rainbowThread3.join();

    if (hProcess)
        CloseHandle(hProcess);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

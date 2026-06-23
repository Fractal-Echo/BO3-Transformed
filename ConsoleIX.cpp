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
static constexpr int kMapIdDieRise = 1683975546;
static constexpr int kMapIdTranzit = 1952411002;
static constexpr uintptr_t kClanTagPatchOffset = 0x4;
static constexpr uintptr_t kCamoBytePatchOffset = 23;
static constexpr uintptr_t kTableCamoPatternOffset = 0x150;
static constexpr uintptr_t kBankPatternOffset = 0x28;


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
static uint32_t gTargetSessionId = 0;

static bool CanPatchAddress(HANDLE process, uintptr_t address) {
    return process != nullptr && address != 0;
}
template <typename T>
static bool ReadValue(HANDLE process, uintptr_t address, T& value) {
    if (!CanPatchAddress(process, address))
        return false;

    SIZE_T bytesRead = 0;
    return ReadProcessMemory(
        process,
        reinterpret_cast<LPCVOID>(address),
        &value,
        sizeof(value),
        &bytesRead) && bytesRead == sizeof(value);
}

template <typename T>
static bool PatchValue(HANDLE process, uintptr_t address, const T& value) {
    if (!CanPatchAddress(process, address))
        return false;

    return mem::TryPatchEx(reinterpret_cast<BYTE*>(address),
        reinterpret_cast<const BYTE*>(&value),
        sizeof(value),
        process);
}

static bool PatchBytes(HANDLE process, uintptr_t address, const char* bytes, size_t size) {
    if (!CanPatchAddress(process, address) || bytes == nullptr || size == 0)
        return false;

    return mem::TryPatchEx(reinterpret_cast<BYTE*>(address),
        reinterpret_cast<const BYTE*>(bytes),
        size,
        process);
}

static bool WriteString(HANDLE process, uintptr_t address, const char* value) {
    if (!CanPatchAddress(process, address) || value == nullptr)
        return false;

    return mem::TryWriteStringEx(reinterpret_cast<BYTE*>(address), value, process);
}

template <typename T>
static bool PatchToggleValueOnChange(
    HANDLE process,
    uintptr_t address,
    bool enabled,
    bool& initialized,
    bool& previousEnabled,
    uintptr_t& previousAddress,
    uint32_t& previousSessionId,
    const T& enabledValue,
    const T& disabledValue) {
    if (initialized &&
        previousEnabled == enabled &&
        previousAddress == address &&
        previousSessionId == gTargetSessionId) {
        return true;
    }

    if (!PatchValue(process, address, enabled ? enabledValue : disabledValue))
        return false;

    initialized = true;
    previousEnabled = enabled;
    previousAddress = address;
    previousSessionId = gTargetSessionId;
    return true;
}

static bool PatchToggleBytesOnChange(
    HANDLE process,
    uintptr_t address,
    bool enabled,
    bool& initialized,
    bool& previousEnabled,
    uintptr_t& previousAddress,
    uint32_t& previousSessionId,
    const char* enabledBytes,
    size_t enabledSize,
    const char* disabledBytes,
    size_t disabledSize) {
    if (initialized &&
        previousEnabled == enabled &&
        previousAddress == address &&
        previousSessionId == gTargetSessionId) {
        return true;
    }

    if (!PatchBytes(process,
        address,
        enabled ? enabledBytes : disabledBytes,
        enabled ? enabledSize : disabledSize)) {
        return false;
    }

    initialized = true;
    previousEnabled = enabled;
    previousAddress = address;
    previousSessionId = gTargetSessionId;
    return true;
}

bool PatchWeaponID(uintptr_t addr, int id, HANDLE hProcess) {
    return PatchValue(hProcess, addr, id);
}

bool PatchCamoID(uintptr_t addr2, int id, HANDLE hProcess) {
    return PatchValue(hProcess, addr2, id);
}
static bool TryReadMapId(HANDLE process, uintptr_t mapIdAddress, int& mapId) {
    return ReadValue(process, mapIdAddress, mapId);
}

static bool IsSupportedMapId(int mapId) {
    return mapId == kMapIdDieRise || mapId == kMapIdTranzit;
}

static bool ShouldScanBankAddress(int mapId) {
    return mapId == kMapIdDieRise || mapId == kMapIdTranzit;
}

static bool ShouldScanTableCamoAddress(int mapId) {
    return mapId == kMapIdDieRise;
}

static bool CanPatchBankAddress(int mapId, uintptr_t bankAddress) {
    return ShouldScanBankAddress(mapId) && bankAddress != 0;
}

static bool CanPatchTableCamoAddress(int mapId, uintptr_t tableCamoAddress) {
    return ShouldScanTableCamoAddress(mapId) && tableCamoAddress != 0;
}

static std::vector<Weapon>* GetWeaponListForMapId(int mapId) {
    switch (mapId) {
    case kMapIdDieRise:
        return &weaponList;
    case kMapIdTranzit:
        return &TranzitWeaponList_1;
    default:
        return nullptr;
    }
}

static bool IsValidWeaponAddress(HANDLE process, uintptr_t weaponAddress) {
    int currentWeaponId = 0;
    return ReadValue(process, weaponAddress, currentWeaponId);
}

static uintptr_t ResolvePatternAddress(HANDLE process, const char* pattern, const char* mask, uintptr_t subtractOffset) {
    const uintptr_t patternAddress = FindPatternEx(process, pattern, mask);
    if (patternAddress == 0 || patternAddress < subtractOffset)
        return 0;

    return patternAddress - subtractOffset;
}

static uintptr_t ResolveClanTagAddress(HANDLE process) {
    const uintptr_t patternAddress = FindPatternEx(process, "\x5B\x33\x33\x5D\x5E", "xxxxx");
    if (patternAddress == 0)
        return 0;

    return patternAddress + kClanTagPatchOffset;
}

static uintptr_t ResolveCamoByteAddress(HANDLE process, DWORD processId, uintptr_t baseAddress) {
    return FindPattern(
        process,
        processId,
        baseAddress,
        "\x00\x00\x00\x00\x00\x00\x00\xBD\x78\x50\x54\x00\x00\x00\x00\x03\x00\x00\x00\x00\x00\x00\x00\x16",
        "???????xxx????x????????x");
}



bool bHealth = true, bAmmo = false, bRewardGiven = false, bCamoFlag = false, bName = false, bCamo = false, bNameToggle = false, bSvCheats = false, bDebug = false, bInstant = false, bNoclip = false, bToggleMW = false, bToggleQW = false, bToggleWW = false, bToggleEW = false, bToggleScore = false, bToggleAbilityDamage = false, bToggleSpeed = false, bToggleJump = false, bRainbow = false, bRainbow2 = false, bRainbow3 = false, bRainbowBypass = false, bRainbowCycle = false, bTest = false, bTest2 = false, bTest3 = false, bReviveGoldTU8E = false, bGoldTU8EHealth = false, bUnlimitedGrenades = false, bGoldTU8EFlag = true, bRapid = false;
bool bClan = false, isPatched = false, bBank = false;
std::thread rainbowThread;
std::atomic<bool> stopThread(false);
std::thread rainbowThread2;
std::atomic<bool> stopThread2(false);
std::thread rainbowThread3;
std::atomic<bool> stopThread3(false);

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



DWORD procId;
HANDLE hProcess;
uintptr_t moduleBase = 0, HookLastByteAddr = 0, HookClanByte = 0, SigCamoAddr = 0, ModIdAddr = 0, TestAddr = 0, LeftPistolAddr = 0, AmmoBaseAddr = 0, NameAddr = 0, NameBaseAddr = 0, MapIdAddr = 0, ConnectionIDAddr = 0, WeaponInHandAddr = 0, TableBase = 0, SvCheatsAddr = 0, DeveloperAddr = 0, TWeaponAddr = 0, LocalPlayerOffset = 0, ZombiesCountAddr = 0, CamoAddr1 = 0, CamoAddr2 = 0, CamoAddr3 = 0, CamoAddr4 = 0, CamoAddr5 = 0, XCoordAddr = 0, YCoordAddr = 0, ZCoordAddr = 0, NoclipAddr = 0, GoldTU8EHealthAddr = 0, GoldTU8EBaseAddr = 0, GoldTU8ENameAddr = 0, GodBaseAddr = 0, GodAddr = 0, MWeaponAddr = 0, QWeaponAddr = 0, WWeaponAddr = 0, EWeaponAddr = 0, ScoreAddr = 0, ScoreBaseAddr = 0, ClassCamoBaseAddr = 0, ClassCamoAddr = 0;
uintptr_t TestAddr2 = 0, TableCamoAddr = 0, BankAddr = 0, EntityList = 0, DistanceBetween = 0, HealthAddr = 0;

static bool IsTargetProcessAlive(HANDLE process) {
    if (!process)
        return false;

    DWORD exitCode = 0;
    return GetExitCodeProcess(process, &exitCode) && exitCode == STILL_ACTIVE;
}

static void StopRainbowThreads() {
    stopThread.store(true);
    stopThread2.store(true);
    stopThread3.store(true);

    if (rainbowThread.joinable())
        rainbowThread.join();
    if (rainbowThread2.joinable())
        rainbowThread2.join();
    if (rainbowThread3.joinable())
        rainbowThread3.join();

    bRainbow = false;
    bRainbow2 = false;
    bRainbow3 = false;
}

static void ResetRuntimeAddresses() {
    moduleBase = HookLastByteAddr = HookClanByte = SigCamoAddr = ModIdAddr = TestAddr = LeftPistolAddr = AmmoBaseAddr = NameAddr = NameBaseAddr = MapIdAddr = ConnectionIDAddr = WeaponInHandAddr = TableBase = SvCheatsAddr = DeveloperAddr = TWeaponAddr = LocalPlayerOffset = ZombiesCountAddr = CamoAddr1 = CamoAddr2 = CamoAddr3 = CamoAddr4 = CamoAddr5 = XCoordAddr = YCoordAddr = ZCoordAddr = NoclipAddr = GoldTU8EHealthAddr = GoldTU8EBaseAddr = GoldTU8ENameAddr = GodBaseAddr = GodAddr = MWeaponAddr = QWeaponAddr = WWeaponAddr = EWeaponAddr = ScoreAddr = ScoreBaseAddr = ClassCamoBaseAddr = ClassCamoAddr = 0;
    TestAddr2 = TableCamoAddr = BankAddr = EntityList = DistanceBetween = HealthAddr = 0;
}

static void CloseTargetProcess() {
    StopRainbowThreads();

    if (hProcess) {
        CloseHandle(hProcess);
        hProcess = nullptr;
    }

    procId = 0;
    ResetRuntimeAddresses();
    ++gTargetSessionId;
    bBank = false;
}

static void SyncMapSpecificRuntimeAddresses(bool hasMapId, int mapId) {
    static bool hadPreviousMapId = false;
    static int previousMapId = 0;

    if (!hasMapId) {
        hadPreviousMapId = false;
        BankAddr = 0;
        TableCamoAddr = 0;
        return;
    }

    if (!hadPreviousMapId || previousMapId != mapId) {
        BankAddr = 0;
        TableCamoAddr = 0;
        previousMapId = mapId;
        hadPreviousMapId = true;
    }

    if (!ShouldScanBankAddress(mapId))
        BankAddr = 0;

    if (!ShouldScanTableCamoAddress(mapId))
        TableCamoAddr = 0;
}

static void ResolveRuntimeSignatures(HANDLE process, DWORD processId, uintptr_t baseAddress, int mapId) {
    if (!CanPatchAddress(process, baseAddress))
        return;

    if (HookClanByte == 0)
        HookClanByte = ResolveClanTagAddress(process);

    if (HookLastByteAddr == 0)
        HookLastByteAddr = ResolveCamoByteAddress(process, processId, baseAddress);

    if (ShouldScanTableCamoAddress(mapId) && TableCamoAddr == 0) {
        TableCamoAddr = ResolvePatternAddress(
            process,
            "\xCD\x2C\xA8\x44\xAC\x4C\xB2\x43\xBE\xFF\x74\x43",
            "xxxxxxxxxxxx",
            kTableCamoPatternOffset);
    }

    if (ShouldScanBankAddress(mapId) && BankAddr == 0) {
        BankAddr = ResolvePatternAddress(
            process,
            "\x9C\x50\xE5\xCB\x00\x00\x00\x00",
            "xxxxxxxx",
            kBankPatternOffset);
    }
}


static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void RainbowCycleLoop() {
    while (!stopThread.load()) {//87 = original valuev


        for (int RainbowValue = 1; RainbowValue <= 126; ++RainbowValue) {
            if (stopThread.load()) break;
            PatchValue(hProcess, CamoAddr2, RainbowValue);
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Rainbow Delay //100 MS original value
        }
    }
}

void RainbowCycleLoop2() {
    while (!stopThread2.load()) {
        for (int RainbowValue2 = 1; RainbowValue2 <= 126; ++RainbowValue2) {
            if (stopThread2.load()) break;
            int currentMapId = 0;
            if (TryReadMapId(hProcess, MapIdAddr, currentMapId) &&
                CanPatchTableCamoAddress(currentMapId, TableCamoAddr)) {
                PatchValue(hProcess, TableCamoAddr, RainbowValue2);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Rainbow Delay //100 MS original value
        }
    }
}

void RainbowCycleLoop3() {
    while (!stopThread3.load()) {
        for (int RainbowValue3 = 1; RainbowValue3 <= 126; ++RainbowValue3) {
            if (stopThread3.load()) break;
            PatchValue(hProcess, CamoAddr5, RainbowValue3);
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
    ShowMenu(window);

    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (hProcess && !IsTargetProcessAlive(hProcess))
            CloseTargetProcess();

        DrawTextWatermark("X v0.1.0");
        DrawDebugWatermark(hProcess, MWeaponAddr);
        DrawDebugWatermark2(hProcess, CamoAddr2);

        int frameMapId = 0;
        const bool hasFrameMapId = TryReadMapId(hProcess, MapIdAddr, frameMapId);
        SyncMapSpecificRuntimeAddresses(hasFrameMapId, frameMapId);
        const bool canPatchBankAddress = hasFrameMapId && CanPatchBankAddress(frameMapId, BankAddr);
        const bool canPatchTableCamoAddress = hasFrameMapId && CanPatchTableCamoAddress(frameMapId, TableCamoAddr);





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
            ImGui::Checkbox("Sig", &bClan);
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


            if (ImGui::Button("Max Bank") && canPatchBankAddress)
            {
                bBank = !bBank;


            }




            static bool bankPatchInitialized = false;
            static bool previousBankEnabled = false;
            static uintptr_t previousBankAddress = 0;
            static uint32_t previousBankSessionId = 0;
            if (!canPatchBankAddress) {
                bBank = false;
                bankPatchInitialized = false;
                previousBankEnabled = false;
                previousBankAddress = 0;
                previousBankSessionId = 0;
            }
            else {
                PatchToggleValueOnChange(
                    hProcess,
                    BankAddr,
                    bBank,
                    bankPatchInitialized,
                    previousBankEnabled,
                    previousBankAddress,
                    previousBankSessionId,
                    m1,
                    m0);
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
            const bool hasMapId = TryReadMapId(hProcess, MapIdAddr, currentMapId);
            const bool hasSupportedMapId = hasMapId && IsSupportedMapId(currentMapId);
            std::vector<Weapon>* mapWeaponList = hasSupportedMapId ? GetWeaponListForMapId(currentMapId) : nullptr;
            currentWeaponList = mapWeaponList != nullptr ? mapWeaponList : &weaponList;
            if (currentWeaponList == nullptr || currentWeaponList->empty()) {
                currentWeaponList = &weaponList;
            }
            if (selectedWeaponIndex < 0 || selectedWeaponIndex >= static_cast<int>(currentWeaponList->size())) {
                selectedWeaponIndex = 0;
            }
            const bool hasValidWeaponAddress = IsValidWeaponAddress(hProcess, QWeaponAddr);

            ImGui::Text("Weapon Selector");
            if (!hasSupportedMapId) {
                ImGui::TextDisabled("Waiting for Tranzit or Die Rise map ID");
            }
            else if (!hasValidWeaponAddress) {
                ImGui::TextDisabled("Waiting for valid weapon address");
            }
            else if (ImGui::BeginCombo("##WeaponSelector", (*currentWeaponList)[selectedWeaponIndex].name)) {
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
                        if (!PatchCamoID(camoAddrToUse, CamoID, hProcess)) {
                            std::cerr << "Failed to patch camo ID to process memory!" << std::endl;
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


                    PatchValue(hProcess, CamoAddr2, originalCamoValue);
                }
            }


            if (!canPatchTableCamoAddress && bRainbow2) {
                bRainbow2 = false;
                stopThread2.store(true);
                if (rainbowThread2.joinable()) {
                    rainbowThread2.join();
                }
            }

            if (ImGui::Button("Toggle Rainbow Table Camo") && canPatchTableCamoAddress) {
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
                    PatchValue(hProcess, CamoAddr4, staticColorValue);
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
                    PatchValue(hProcess, CamoAddr5, staticColorValue);
                    //  mem::PatchEx((BYTE*)(moduleBase + 0x427A75), (BYTE*)"\x74\x0D", 2, hProcess);
                }
            }








            if (ImGui::Button("Exit")) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }

            ImVec4 rainbowTextColor = RainbowColor(1.0f, 0.3f, 1.0f);

            ImGui::TextColored(rainbowTextColor, "*_* != GoldTU8E;");
            ImGui::End();
        }

        if (procId == 0) {
            static float lastProcessSearch = -2.0f;
            const float currentTime = static_cast<float>(ImGui::GetTime());
            if (currentTime - lastProcessSearch >= 2.0f) {
                lastProcessSearch = currentTime;
                procId = GetProcId(L"BlackOps3.exe");
            }

            if (procId != 0) {
                hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
                if (hProcess) {
                    ResetRuntimeAddresses();
                    moduleBase = GetModuleBaseAddress(procId, L"BlackOps3.exe");
                    if (moduleBase == 0) {
                        CloseTargetProcess();
                    }
                    else {
                    ++gTargetSessionId;
                    EntityList = moduleBase + 0x7DD48D8;
                    DistanceBetween = 0x3088;
                    HealthAddr = FindDMAAddy(hProcess, HealthAddr, { 0x2C8 });
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




                    int initialMapId = 0;
                    const bool hasInitialMapId = TryReadMapId(hProcess, MapIdAddr, initialMapId);
                    SyncMapSpecificRuntimeAddresses(hasInitialMapId, initialMapId);
                    ResolveRuntimeSignatures(hProcess, procId, moduleBase, initialMapId);
                    //5B F5 ?? ?? ?? ?? ?? ?? 06 ?? ?? ?? F7 7F




                  //  TWeaponAddr = FindDMAAddy(hProcess, TableBase, { 0x954 });




                    //GoldTU8E's name base = blackops3.exe+37C49E0
                    }
                }
                else {
                    procId = 0;
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
                std::cout << "Bank Sig test 1" << std::hex << BankAddr << std::endl;





                // GoldTU8ERespawnFunction();

            }
        }


        static float lastRuntimeSignatureRetry = 0.0f;
        const bool shouldScanTableCamoAddress = hasFrameMapId && ShouldScanTableCamoAddress(frameMapId);
        const bool shouldScanBankAddress = hasFrameMapId && ShouldScanBankAddress(frameMapId);
        const bool hasUnresolvedRuntimeSignature =
            HookClanByte == 0 ||
            HookLastByteAddr == 0 ||
            (shouldScanTableCamoAddress && TableCamoAddr == 0) ||
            (shouldScanBankAddress && BankAddr == 0);

        if (hasUnresolvedRuntimeSignature && hProcess && moduleBase != 0) {
            const float currentTime = static_cast<float>(ImGui::GetTime());
            if (currentTime - lastRuntimeSignatureRetry >= 2.0f) {
                ResolveRuntimeSignatures(hProcess, procId, moduleBase, frameMapId);
                lastRuntimeSignatureRetry = currentTime;
            }
        }
        static bool ammoPatchApplied = false;
        static uint32_t ammoPatchSessionId = 0;
        if (bAmmo && (!ammoPatchApplied || ammoPatchSessionId != gTargetSessionId))
        {
            const bool patchedAmmo = PatchBytes(hProcess, moduleBase + 0x27C8234, "\xC7\x84\x83\x84\x06\x00\x00\x67\x02\x00\x00\x90\x90\x90\x90", 15);

            int A1 = 615;
            const bool patchedLeftPistol = PatchValue(hProcess, LeftPistolAddr, A1);
            //Unlimited Grenades
            const bool patchedGrenades = PatchBytes(hProcess, moduleBase + 0x27C6E9A, "\x8B\xFF", 2);
            if (patchedAmmo && patchedLeftPistol && patchedGrenades) {
                ammoPatchApplied = true;
                ammoPatchSessionId = gTargetSessionId;
            }
        }
        else if (!bAmmo)
        {
            //    mem::PatchEx((BYTE*)(moduleBase + 0x27C8234), (BYTE*)"\x83\xBC\x83\x84\x06\x00\x00\x00\x75\x0B\xB8\x01\x00\x00\x00", 15, hProcess);


             //   mem::PatchEx((BYTE*)(moduleBase + 0x27C6E9A), (BYTE*)"\x8B\x01", 2, hProcess);
            ammoPatchApplied = false;
            ammoPatchSessionId = 0;
        }


        static bool namePatchInitialized = false;
        static bool previousNameEnabled = false;
        static uintptr_t previousNameAddress = 0;
        static uint32_t previousNameSessionId = 0;
        if (!namePatchInitialized ||
            previousNameEnabled != bName ||
            previousNameAddress != NameAddr ||
            previousNameSessionId != gTargetSessionId)
        {
            const char* playerName = bName ? "Danny" : "^1SyntaX-_-";
            if (WriteString(hProcess, NameAddr, playerName))
            {
                bNameToggle = bName;
                previousNameEnabled = bName;
                previousNameAddress = NameAddr;
                previousNameSessionId = gTargetSessionId;
                namePatchInitialized = true;
            }
        }


        static bool clanBypassPatchInitialized = false;
        static bool previousClanBypassEnabled = false;
        static uintptr_t previousClanBypassAddress = 0;
        static uint32_t previousClanBypassSessionId = 0;
        PatchToggleBytesOnChange(
            hProcess,
            moduleBase + 0x27EB255,
            isPatched,
            clanBypassPatchInitialized,
            previousClanBypassEnabled,
            previousClanBypassAddress,
            previousClanBypassSessionId,
            "\x75\x06",
            2,
            "\x74\x06",
            2);

        ///new signature test
        if (bCamo && HookLastByteAddr != 0)
        {
            BYTE Galaxy = 0x7D;
            PatchValue(hProcess, HookLastByteAddr + kCamoBytePatchOffset, Galaxy);
        }


        if (GetAsyncKeyState(VK_F4) && canPatchBankAddress)
        {
            PatchValue(hProcess, BankAddr, m1);
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

        static bool svCheatsPatchInitialized = false;
        static bool previousSvCheatsEnabled = false;
        static uintptr_t previousSvCheatsAddress = 0;
        static uint32_t previousSvCheatsSessionId = 0;
        PatchToggleValueOnChange(
            hProcess,
            SvCheatsAddr,
            bSvCheats,
            svCheatsPatchInitialized,
            previousSvCheatsEnabled,
            previousSvCheatsAddress,
            previousSvCheatsSessionId,
            ON,
            OFF);
        static bool rapidPatchInitialized = false;
        static bool previousRapidEnabled = false;
        static uintptr_t previousRapidMainAddress = 0;
        static uintptr_t previousRapidShotgunsAddress = 0;
        static uint32_t previousRapidSessionId = 0;
        const uintptr_t rapidMainAddress = moduleBase + 0x27B6353;
        const uintptr_t rapidShotgunsAddress = moduleBase + 0x27C6519;
        if (!rapidPatchInitialized ||
            previousRapidEnabled != bRapid ||
            previousRapidMainAddress != rapidMainAddress ||
            previousRapidShotgunsAddress != rapidShotgunsAddress ||
            previousRapidSessionId != gTargetSessionId)
        {
            const bool patchedMain = PatchBytes(
                hProcess,
                rapidMainAddress,
                bRapid ? "\xC7\x04\x38\x00\x00\x00\x00\x90\x90\x90" : "\x89\x0C\x38\x44\x38\xAE\x68\x0E\x00\x00",
                10);
            const bool patchedShotguns = PatchBytes(
                hProcess,
                rapidShotgunsAddress,
                bRapid ? "\xC7\x40\x2C\x02\x00\x00\x00\x90\x90" : "\x44\x08\x70\x2C\xE8\x5E\xB4\x00\x00",
                9);

            if (patchedMain && patchedShotguns)
            {
                rapidPatchInitialized = true;
                previousRapidEnabled = bRapid;
                previousRapidMainAddress = rapidMainAddress;
                previousRapidShotgunsAddress = rapidShotgunsAddress;
                previousRapidSessionId = gTargetSessionId;
            }
        }

        static bool instantPatchInitialized = false;
        static bool previousInstantEnabled = false;
        static uintptr_t previousInstantAddress = 0;
        static uint32_t previousInstantSessionId = 0;
        PatchToggleBytesOnChange(
            hProcess,
            moduleBase + 0x26279A5,
            bInstant,
            instantPatchInitialized,
            previousInstantEnabled,
            previousInstantAddress,
            previousInstantSessionId,
            "\x0F\x8C",
            2,
            "\x0F\x8F",
            2);





        if (bTest2)
        {
            static SHORT prevF3State = 0;
            static SHORT prevF4State = 0;

            int N1 = 0;
            if (CanPatchAddress(hProcess, MWeaponAddr))
                ReadProcessMemory(hProcess, (LPCVOID)MWeaponAddr, &N1, sizeof(N1), nullptr);
            SHORT currentF3State = GetAsyncKeyState(VK_F3);
            if ((currentF3State & 0x8000) && !(prevF3State & 0x8000))
            {
                N1--;
                PatchValue(hProcess, MWeaponAddr, N1);
            }
            prevF3State = currentF3State;

            SHORT currentF4State = GetAsyncKeyState(VK_F4);
            if ((currentF4State & 0x8000) && !(prevF4State & 0x8000))
            {
                N1++;
                PatchValue(hProcess, MWeaponAddr, N1);
            }
            prevF4State = currentF4State;
        }


        if (bTest3)
        {
            static SHORT prevF3State = 0;
            static SHORT prevF4State = 0;

            int N1 = 0;
            if (CanPatchAddress(hProcess, CamoAddr2))
                ReadProcessMemory(hProcess, (LPCVOID)CamoAddr2, &N1, sizeof(N1), nullptr);
            SHORT currentF3State = GetAsyncKeyState(VK_F3);
            if ((currentF3State & 0x8000) && !(prevF3State & 0x8000))
            {
                N1--;
                PatchValue(hProcess, CamoAddr2, N1);
            }
            prevF3State = currentF3State;

            SHORT currentF4State = GetAsyncKeyState(VK_F4);
            if ((currentF4State & 0x8000) && !(prevF4State & 0x8000))
            {
                N1++;
                PatchValue(hProcess, CamoAddr2, N1);
            }
            prevF4State = currentF4State;
        }


        if (bTest && NoclipAddr != 0)
        {
            if (GetAsyncKeyState(0x45))  // key is down
            {
                int N1 = 1;
                PatchValue(hProcess, NoclipAddr, N1);
            }
            else
            {
                int N0 = 0;
                PatchValue(hProcess, NoclipAddr, N0);
            }
        }

        static bool healthPatchInitialized = false;
        static bool previousHealthEnabled = false;
        static uintptr_t previousHealthAddress = 0;
        static uint32_t previousHealthSessionId = 0;
        const int godEnabled = 12297;
        const int godDisabled = 12296;
        PatchToggleValueOnChange(
            hProcess,
            GodAddr,
            bHealth,
            healthPatchInitialized,
            previousHealthEnabled,
            previousHealthAddress,
            previousHealthSessionId,
            godEnabled,
            godDisabled);

        static bool goldTU8EHealthResetApplied = false;
        if (bGoldTU8EHealth && GoldTU8EHealthAddr != 0) {
            goldTU8EHealthResetApplied = false;
            const int uHealth = 13337;
            static float lastGoldTU8EHealthPatch = 0.0f;
            const float currentTime = static_cast<float>(ImGui::GetTime());
            if (currentTime - lastGoldTU8EHealthPatch >= 0.1f)
            {
                PatchValue(hProcess, GoldTU8EHealthAddr, uHealth);
                lastGoldTU8EHealthPatch = currentTime;
            }




        }

        if (isCycling) {
            const float currentTime = static_cast<float>(ImGui::GetTime());  // Get the current time in seconds

            // Only update targetIndex if enough time has passed since the last cycle
            if (currentTime - lastCycleTime >= cycleInterval) {

                targetIndex = (targetIndex + 1) % 9;

                const std::string& currentTarget = targets[targetIndex];

                PatchBytes(hProcess, HookClanByte, currentTarget.c_str(), currentTarget.size());

                lastCycleTime = currentTime;
            }

        }



        if (isCycling2) {
            const float currentTime2 = static_cast<float>(ImGui::GetTime());  // Get the current time in seconds

            // Only update targetIndex if enough time has passed since the last cycle
            if (currentTime2 - lastCycleTime2 >= cycleInterval2) {

                targetIndex2 = (targetIndex2 + 1) % 9;

                const std::string& currentTarget2 = targets2[targetIndex2];

                PatchBytes(hProcess, GoldTU8ENameAddr, currentTarget2.c_str(), currentTarget2.size());

                lastCycleTime2 = currentTime2;
            }

        }

        if (!bGoldTU8EHealth && GoldTU8EHealthAddr != 0)
        {
            int UHealth = 100;
            if (!goldTU8EHealthResetApplied && PatchValue(hProcess, GoldTU8EHealthAddr, UHealth))
                goldTU8EHealthResetApplied = true;
            //mem::PatchEx((BYTE*)(moduleBase + 0x1CFF64), (BYTE*)"\xC7\x86\x70\x02\x00\x00\x08\x00\x00\x00", 10, hProcess);
           // mem::PatchEx((BYTE*)(moduleBase + 0x2BA72F), (BYTE*)"\xFF\x48\x04", 3, hProcess);
        }
        if (bToggleMW && MWeaponAddr != 0)
        {
            PatchValue(hProcess, MWeaponAddr, MWeaponId);
        }

        if (bToggleQW && QWeaponAddr != 0)
        {
            PatchValue(hProcess, QWeaponAddr, QWeaponId);
        }

        if (bToggleWW && WWeaponAddr != 0)
        {
            PatchValue(hProcess, WWeaponAddr, WWeaponId);
        }

        if (bToggleEW && EWeaponAddr != 0)
        {
            PatchValue(hProcess, EWeaponAddr, EWeaponId);
        }
        if (bToggleScore && ScoreAddr != 0)
        {
            PatchValue(hProcess, ScoreAddr, ScoreId);
        }
        else
        {

        }
        // bool isInMenu = true;   // optional flag if you later detect menu vs in-game

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

    CloseTargetProcess();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

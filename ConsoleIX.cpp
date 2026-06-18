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

bool PatchWeaponID(uintptr_t addr, int id, HANDLE hProcess) {
    mem::PatchEx((BYTE*)addr, (BYTE*)&id, sizeof(id), hProcess);
    return true; // always returns true since PatchEx is void
}

bool PatchCamoID(uintptr_t addr2, int id, HANDLE hProcess) {
    mem::PatchEx((BYTE*)addr2, (BYTE*)&id, sizeof(id), hProcess);
    return true; // always returns true since PatchEx is void
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


static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

void RainbowCycleLoop() {
    while (!stopThread.load()) {//87 = original valuev


        for (int RainbowValue = 1; RainbowValue <= 126; ++RainbowValue) {
            if (stopThread.load()) break;
            mem::PatchEx((BYTE*)CamoAddr2, (BYTE*)&RainbowValue, sizeof(RainbowValue), hProcess);
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Rainbow Delay //100 MS original value
        }
    }
}

void RainbowCycleLoop2() {
    while (!stopThread2.load()) {
        for (int RainbowValue2 = 1; RainbowValue2 <= 126; ++RainbowValue2) {
            if (stopThread2.load()) break;
            mem::PatchEx((BYTE*)TableCamoAddr, (BYTE*)&RainbowValue2, sizeof(RainbowValue2), hProcess);
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Rainbow Delay //100 MS original value
        }
    }
}

void RainbowCycleLoop3() {
    while (!stopThread3.load()) {
        for (int RainbowValue3 = 1; RainbowValue3 <= 126; ++RainbowValue3) {
            if (stopThread3.load()) break;
            mem::PatchEx((BYTE*)CamoAddr5, (BYTE*)&RainbowValue3, sizeof(RainbowValue3), hProcess);
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

    while (!glfwWindowShouldClose(window)) {

        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        DrawTextWatermark("X v0.1.0");
        DrawDebugWatermark(hProcess, MWeaponAddr);
        DrawDebugWatermark2(hProcess, CamoAddr2);





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


            if (ImGui::Button("Max Bank"))
            {
                bBank = !bBank;


            }




            if (bBank)
            {
                mem::PatchEx((BYTE*)BankAddr, (BYTE*)&m1, sizeof(m1), hProcess);
            }
            else
            {
                mem::PatchEx((BYTE*)BankAddr, (BYTE*)&m0, sizeof(m0), hProcess);
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


                    mem::PatchEx((BYTE*)CamoAddr2, (BYTE*)&originalCamoValue, sizeof(originalCamoValue), hProcess);
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
                    mem::PatchEx((BYTE*)CamoAddr4, (BYTE*)&staticColorValue, sizeof(staticColorValue), hProcess);
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
                    mem::PatchEx((BYTE*)CamoAddr5, (BYTE*)&staticColorValue, sizeof(staticColorValue), hProcess);
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
            procId = GetProcId(L"BlackOps3.exe");
            if (procId != 0) {
                hProcess = OpenProcess(PROCESS_ALL_ACCESS, NULL, procId);
                if (hProcess) {
                    moduleBase = GetModuleBaseAddress(procId, L"BlackOps3.exe");
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

               // TableCamoAddr = FindPatternEx(hProcess, "\xCD\x2C\xA8\x44\xAC\x4C\xB2\x43\xBE\xFF\x74\x43", "xxxxxxxxxxxx");
               //TableCamoAddr = sig::FindPatternEx(hProcess, "\xCD\x2C\xA8\x44\xAC\x4C\xB2\x43\xBE\xFF\x74\x43", "x?x?x?xx");

                    if (TableCamoAddr != 0) {
                        //TableCamoAddr -= 0x16C;
                        TableCamoAddr -= 0x150;
                    }


                       BankAddr = FindPatternEx(
                          hProcess,
                          "\x9C\x50\xE5\xCB\x00\x00\x00\x00",
                          "xxxxxxxx"
                      );
                  

                  // BankAddr = sig::FindPatternEx(hProcess, "\x9C\x50\xE5\xCB\x00\x00\x00\x00", "xxxxxx");

                    if (BankAddr != 0) {
                        BankAddr -= 0x28;

                    }

                    //5B F5 ?? ?? ?? ?? ?? ?? 06 ?? ?? ?? F7 7F




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





                // GoldTU8ERespawnFunction();

            }
        }


        if (bAmmo)
        {
            mem::PatchEx((BYTE*)(moduleBase + 0x27C8234), (BYTE*)"\xC7\x84\x83\x84\x06\x00\x00\x67\x02\x00\x00\x90\x90\x90\x90", 15, hProcess);

            int A1 = 615;
            mem::PatchEx((BYTE*)LeftPistolAddr, (BYTE*)&A1, sizeof(A1), hProcess);
            //Unlimited Grenades 
            mem::PatchEx((BYTE*)(moduleBase + 0x27C6E9A), (BYTE*)"\x8B\xFF", 2, hProcess);
        }
        else
        {
            //    mem::PatchEx((BYTE*)(moduleBase + 0x27C8234), (BYTE*)"\x83\xBC\x83\x84\x06\x00\x00\x00\x75\x0B\xB8\x01\x00\x00\x00", 15, hProcess);


             //   mem::PatchEx((BYTE*)(moduleBase + 0x27C6E9A), (BYTE*)"\x8B\x01", 2, hProcess);
        }


        if (bName)
        {
            bNameToggle = true;
            //mem::WriteStringEx((BYTE*)NameAddr, "^1[DivX]SyntaX-_-", hProcess);
            mem::WriteStringEx((BYTE*)NameAddr, "Danny", hProcess);
        }
        else
        {
            bNameToggle = false;
            mem::WriteStringEx((BYTE*)NameAddr, "^1SyntaX-_-", hProcess);
        }


        if (isPatched)
        {
            mem::PatchEx((BYTE*)(moduleBase + 0x27EB255), (BYTE*)"\x75\x06", 2, hProcess);
        }
        else
        {
            mem::PatchEx((BYTE*)(moduleBase + 0x27EB255), (BYTE*)"\x74\x06", 2, hProcess);
        }

        if (bClan)
        {



            /*
            //size_t Offset = 116;
            BYTE camoValue = 0x42;
           // mem::PatchEx((BYTE*)(HookClanByte ), &camoValue, sizeof(camoValue), hProcess);
            mem::WriteStringEx((BYTE*)(HookClanByte), "3arc", hProcess);
            
            
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
            mem::PatchEx((BYTE*)(HookLastByteAddr + lastByteOffset), &Galaxy, sizeof(Galaxy), hProcess);
        }


        if (GetAsyncKeyState(VK_F4))
        {
            mem::PatchEx((BYTE*)BankAddr, (BYTE*)&m1, sizeof(m1), hProcess);
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

        if (bSvCheats)
        {
            //   int C1 = 285587035;
            mem::PatchEx((BYTE*)SvCheatsAddr, (BYTE*)&ON, sizeof(ON), hProcess);
        }
        else
        {
            // int C1 = 694749996;
            mem::PatchEx((BYTE*)SvCheatsAddr, (BYTE*)&OFF, sizeof(OFF), hProcess);
        }



        if (bRapid)//Need logic for burst fire weapons
        {
            mem::PatchEx((BYTE*)(moduleBase + 0x27B6353), (BYTE*)"\xC7\x04\x38\x00\x00\x00\x00\x90\x90\x90", 10, hProcess);
            mem::PatchEx((BYTE*)(moduleBase + 0x27C6519), (BYTE*)"\xC7\x40\x2C\x02\x00\x00\x00\x90\x90", 9, hProcess);//Shotguns


        }
        else
        {
            mem::PatchEx((BYTE*)(moduleBase + 0x27B6353), (BYTE*)"\x89\x0C\x38\x44\x38\xAE\x68\x0E\x00\x00", 10, hProcess);
            mem::PatchEx((BYTE*)(moduleBase + 0x27C6519), (BYTE*)"\x44\x08\x70\x2C\xE8\x5E\xB4\x00\x00", 9, hProcess);

        }

        if (bInstant)
        {

            mem::PatchEx((BYTE*)(moduleBase + 0x26279A5), (BYTE*)"\x0F\x8C", 2, hProcess);

        }
        else
        {
            mem::PatchEx((BYTE*)(moduleBase + 0x26279A5), (BYTE*)"\x0F\x8F", 2, hProcess);
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
            ReadProcessMemory(hProcess, (LPCVOID)MWeaponAddr, &N1, sizeof(N1), nullptr);
            SHORT currentF3State = GetAsyncKeyState(VK_F3);
            if ((currentF3State & 0x8000) && !(prevF3State & 0x8000))
            {
                N1--;
                mem::PatchEx((BYTE*)MWeaponAddr, (BYTE*)&N1, sizeof(N1), hProcess);
            }
            prevF3State = currentF3State;

            SHORT currentF4State = GetAsyncKeyState(VK_F4);
            if ((currentF4State & 0x8000) && !(prevF4State & 0x8000))
            {
                N1++;
                mem::PatchEx((BYTE*)MWeaponAddr, (BYTE*)&N1, sizeof(N1), hProcess);
            }
            prevF4State = currentF4State;
        }


        if (bTest3)
        {
            static SHORT prevF3State = 0;
            static SHORT prevF4State = 0;

            int N1 = 0;
            ReadProcessMemory(hProcess, (LPCVOID)CamoAddr2, &N1, sizeof(N1), nullptr);
            SHORT currentF3State = GetAsyncKeyState(VK_F3);
            if ((currentF3State & 0x8000) && !(prevF3State & 0x8000))
            {
                N1--;
                mem::PatchEx((BYTE*)CamoAddr2, (BYTE*)&N1, sizeof(N1), hProcess);
            }
            prevF3State = currentF3State;

            SHORT currentF4State = GetAsyncKeyState(VK_F4);
            if ((currentF4State & 0x8000) && !(prevF4State & 0x8000))
            {
                N1++;
                mem::PatchEx((BYTE*)CamoAddr2, (BYTE*)&N1, sizeof(N1), hProcess);
            }
            prevF4State = currentF4State;
        }


        if (bTest && NoclipAddr != 0)
        {
            if (GetAsyncKeyState(0x45))  // key is down
            {
                int N1 = 1;
                mem::PatchEx((BYTE*)NoclipAddr, (BYTE*)&N1, sizeof(N1), hProcess);
            }
            else
            {
                int N0 = 0;
                mem::PatchEx((BYTE*)NoclipAddr, (BYTE*)&N0, sizeof(N0), hProcess);
            }
        }

        if (bHealth && GodAddr != 0) {
            int God = 12297;
            mem::PatchEx((BYTE*)GodAddr, (BYTE*)&God, sizeof(God), hProcess);
        }
        else
        {
            int God = 12296;
            mem::PatchEx((BYTE*)GodAddr, (BYTE*)&God, sizeof(God), hProcess);
        }

        if (bGoldTU8EHealth && GoldTU8EHealthAddr != 0) {
            const int uHealth = 13337;
            Sleep(5);
            mem::PatchEx((BYTE*)GoldTU8EHealthAddr, (BYTE*)&uHealth, sizeof(uHealth), hProcess);




        }

        if (isCycling) {
            const float currentTime = static_cast<float>(ImGui::GetTime());  // Get the current time in seconds

            // Only update targetIndex if enough time has passed since the last cycle
            if (currentTime - lastCycleTime >= cycleInterval) {

                targetIndex = (targetIndex + 1) % 9;

                const std::string& currentTarget = targets[targetIndex];

                mem::PatchEx((BYTE*)HookClanByte, (BYTE*)currentTarget.c_str(), static_cast<unsigned int>(currentTarget.size()), hProcess);

                lastCycleTime = currentTime;
            }

        }



        if (isCycling2) {
            const float currentTime2 = static_cast<float>(ImGui::GetTime());  // Get the current time in seconds

            // Only update targetIndex if enough time has passed since the last cycle
            if (currentTime2 - lastCycleTime2 >= cycleInterval2) {

                targetIndex2 = (targetIndex2 + 1) % 9;

                const std::string& currentTarget2 = targets2[targetIndex2];

                mem::PatchEx((BYTE*)GoldTU8ENameAddr, (BYTE*)currentTarget2.c_str(), static_cast<unsigned int>(currentTarget2.size()), hProcess);

                lastCycleTime2 = currentTime2;
            }

        }

        if (!bGoldTU8EHealth && GoldTU8EHealthAddr != 0)
        {
            int UHealth = 100;
            mem::PatchEx((BYTE*)GoldTU8EHealthAddr, (BYTE*)&UHealth, sizeof(UHealth), hProcess);
            //mem::PatchEx((BYTE*)(moduleBase + 0x1CFF64), (BYTE*)"\xC7\x86\x70\x02\x00\x00\x08\x00\x00\x00", 10, hProcess);
           // mem::PatchEx((BYTE*)(moduleBase + 0x2BA72F), (BYTE*)"\xFF\x48\x04", 3, hProcess);
        }


        if (bToggleMW && MWeaponAddr != 0)
        {
            mem::PatchEx((BYTE*)MWeaponAddr, (BYTE*)&MWeaponId, sizeof(MWeaponId), hProcess);
        }

        if (bToggleQW && QWeaponAddr != 0)
        {
            mem::PatchEx((BYTE*)QWeaponAddr, (BYTE*)&QWeaponId, sizeof(QWeaponId), hProcess);
        }

        if (bToggleWW && WWeaponAddr != 0)
        {
            mem::PatchEx((BYTE*)WWeaponAddr, (BYTE*)&WWeaponId, sizeof(WWeaponId), hProcess);
        }

        if (bToggleEW && EWeaponAddr != 0)
        {
            mem::PatchEx((BYTE*)EWeaponAddr, (BYTE*)&EWeaponId, sizeof(EWeaponId), hProcess);
        }




        if (bToggleScore && ScoreAddr != 0)
        {
            mem::PatchEx((BYTE*)ScoreAddr, (BYTE*)&ScoreId, sizeof(ScoreId), hProcess);
        }
        else
        {

        }
        //Recode this for lobby id detection but it serves it's purpose
        bool ClanFound = false;

        if (bClan && !ClanFound)
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

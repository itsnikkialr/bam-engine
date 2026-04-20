#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

#include <iostream>
#include <algorithm>
#include <optional>
#include <filesystem>
#include <string>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#include <regex>

#include "Helper.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "glm/glm.hpp"

#include "SDL2/SDL.h"
#include "SDL2_image/SDL_image.h"
#include "SDL2_mixer/SDL_mixer.h"
#include "SDL2_ttf/SDL_ttf.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include "lua.hpp"
#include "LuaBridge.h"
#include "box2d/box2d.h"

#include "Actor.h"
#include "EngineUtils.h"
#include "TemplateDB.h"
#include "Renderer.h"
#include "ImageDB.h"
#include "TextDB.h"
#include "AudioDB.h"
#include "SceneDB.h"
#include "Input.h"
#include "ComponentDB.h"
#include "EventBus.h"

namespace fs = std::filesystem;

std::optional<std::string> GetOptionalString(const rapidjson::Document& doc, const char* key) {
    if (!doc.IsObject()) return std::nullopt;
    auto it = doc.FindMember(key);
    if (it == doc.MemberEnd() || !it->value.IsString()) return std::nullopt;
    return std::string(it->value.GetString());
}

std::optional<int> GetOptionalInt(const rapidjson::Document& doc, const char* key) {
    if (!doc.IsObject()) return std::nullopt;
    auto it = doc.FindMember(key);
    if (it == doc.MemberEnd() || !it->value.IsInt()) return std::nullopt;
    return it->value.GetInt();
}

void mandatoryReads(rapidjson::Document& game_config) {
    if (!fs::exists("resources") || !fs::is_directory("resources")) exit(0);
    if (!fs::exists("resources/game.config")) exit(0);
    EngineUtils::ReadJsonFile("resources/game.config", game_config);
}

// dynamic tiles

struct TileDef {
    int         code;
    std::string label;
    ImVec4      color;
};

// deterministic colors assigned by index so the same tile always gets the same color
static ImVec4 TileColorForIndex(int idx) {
    static const ImVec4 palette[] = {
        {0.45f, 0.38f, 0.55f, 1.0f},  // purple-ish
        {0.20f, 0.65f, 0.30f, 1.0f},  // green
        {0.80f, 0.25f, 0.25f, 1.0f},  // red
        {0.85f, 0.70f, 0.10f, 1.0f},  // yellow
        {0.20f, 0.50f, 0.80f, 1.0f},  // blue
        {0.75f, 0.35f, 0.70f, 1.0f},  // pink
        {0.30f, 0.70f, 0.70f, 1.0f},  // teal
        {0.90f, 0.50f, 0.10f, 1.0f},  // orange
    };
    static const int N = sizeof(palette) / sizeof(palette[0]);
    return palette[idx % N];
}

// scans GameManager.lua for   if tile_code == N   followed within a few lines by   Instantiate("Name")
// returns a map of code -> template name (does not include 0/empty)
std::unordered_map<int, std::string> ParseTileDefsFromLua(const std::string& path) {
    std::unordered_map<int, std::string> defs;
    if (!fs::exists(path)) return defs;

    std::ifstream f(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(f, line)) lines.push_back(line);

    std::regex code_re(R"(tile_code\s*==\s*(\d+))");
    std::regex inst_re(R"REGEX(Instantiate\(\s*"(\w+)"\s*\))REGEX");

    for (int i = 0; i < (int)lines.size(); i++) {
        std::smatch cm;
        if (std::regex_search(lines[i], cm, code_re)) {
            int code = std::stoi(cm[1]);
            // look for Instantiate within the next 5 lines
            for (int j = i; j < std::min(i + 5, (int)lines.size()); j++) {
                std::smatch im;
                if (std::regex_search(lines[j], im, inst_re)) {
                    defs[code] = im[1];
                    break;
                }
            }
        }
    }
    return defs;
}

std::vector<TileDef> BuildTileDefs(const std::unordered_map<int, std::string>& raw) {
    std::vector<TileDef> defs;

    // code 0 is always empty
    defs.push_back({0, "Empty", {0.14f, 0.10f, 0.16f, 1.0f}});

    // sort by code so the palette order is stable
    std::vector<std::pair<int,std::string>> sorted(raw.begin(), raw.end());
    std::sort(sorted.begin(), sorted.end());

    int color_idx = 0;
    for (auto& [code, name] : sorted) {
        defs.push_back({code, name, TileColorForIndex(color_idx++)});
    }
    return defs;
}

static ImVec4 TileColor(const std::vector<TileDef>& defs, int code) {
    for (auto& t : defs) if (t.code == code) return t.color;
    return {0.3f, 0.3f, 0.3f, 1.0f};
}

static const char* TileLabel(const std::vector<TileDef>& defs, int code) {
    for (auto& t : defs) if (t.code == code) return t.label.c_str();
    return "?";
}

// ---------------------------------------------------------------------------
// Grid parse/write — reads and rewrites stage1 = { ... } in GameManager.lua
// ---------------------------------------------------------------------------

static const std::string GM_PATH = "resources/component_types/GameManager.lua";
static const int GRID_ROWS = 20;
static const int GRID_COLS = 20;

using Grid = std::vector<std::vector<int>>;

Grid DefaultGrid() {
    return Grid(GRID_ROWS, std::vector<int>(GRID_COLS, 0));
}

static bool FindStage1Lines(const std::vector<std::string>& lines, int& out_start, int& out_end) {
    out_start = -1; out_end = -1;
    for (int i = 0; i < (int)lines.size(); i++) {
        if (lines[i].find("stage1") != std::string::npos &&
            lines[i].find("{") != std::string::npos) {
            out_start = i; break;
        }
    }
    if (out_start < 0) return false;
    int depth = 0;
    for (int i = out_start; i < (int)lines.size(); i++) {
        for (char c : lines[i]) {
            if (c == '{') depth++;
            if (c == '}') { depth--; if (depth == 0) { out_end = i + 1; return true; } }
        }
    }
    return false;
}

Grid ParseGrid(const std::string& path) {
    Grid grid = DefaultGrid();
    if (!fs::exists(path)) return grid;
    std::ifstream f(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(f, line)) lines.push_back(line);
    int s, e;
    if (!FindStage1Lines(lines, s, e)) return grid;
    int row = 0;
    for (int i = s + 1; i < e - 1; i++) {
        if (row >= GRID_ROWS) break;
        auto lb = lines[i].find('{');
        auto rb = lines[i].find('}');
        if (lb == std::string::npos || rb == std::string::npos) continue;
        std::string inner = lines[i].substr(lb + 1, rb - lb - 1);
        std::istringstream ss(inner);
        std::string tok;
        int col = 0;
        while (std::getline(ss, tok, ',') && col < GRID_COLS) {
            tok.erase(remove_if(tok.begin(), tok.end(), ::isspace), tok.end());
            if (!tok.empty()) { try { grid[row][col] = std::stoi(tok); } catch (...) {} col++; }
        }
        row++;
    }
    return grid;
}

void WriteGrid(const std::string& path, const Grid& grid) {
    if (!fs::exists(path)) return;
    std::ifstream f(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(f, line)) lines.push_back(line);
    int s, e;
    if (!FindStage1Lines(lines, s, e)) return;
    std::vector<std::string> newblock;
    newblock.push_back("\tstage1 = {");
    for (int r = 0; r < GRID_ROWS; r++) {
        std::string row_line = "\t\t{";
        for (int c = 0; c < GRID_COLS; c++) {
            row_line += std::to_string(grid[r][c]);
            if (c < GRID_COLS - 1) row_line += ", ";
        }
        row_line += "}";
        if (r < GRID_ROWS - 1) row_line += ",";
        newblock.push_back(row_line);
    }
    newblock.push_back("\t},");
    std::vector<std::string> result;
    for (int i = 0; i < s; i++)                  result.push_back(lines[i]);
    for (auto& nl : newblock)                    result.push_back(nl);
    for (int i = e; i < (int)lines.size(); i++)  result.push_back(lines[i]);
    std::ofstream out(path);
    for (int i = 0; i < (int)result.size(); i++) {
        out << result[i];
        if (i < (int)result.size() - 1) out << "\n";
    }
}

// ---------------------------------------------------------------------------
// Theme
// ---------------------------------------------------------------------------

void ApplyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]             = ImVec4(0.09f, 0.07f, 0.10f, 1.00f);
    c[ImGuiCol_ChildBg]              = ImVec4(0.11f, 0.08f, 0.13f, 1.00f);
    c[ImGuiCol_PopupBg]              = ImVec4(0.12f, 0.09f, 0.14f, 1.00f);
    c[ImGuiCol_Border]               = ImVec4(0.40f, 0.08f, 0.28f, 0.70f);
    c[ImGuiCol_FrameBg]              = ImVec4(0.16f, 0.10f, 0.19f, 1.00f);
    c[ImGuiCol_FrameBgHovered]       = ImVec4(0.28f, 0.09f, 0.23f, 1.00f);
    c[ImGuiCol_FrameBgActive]        = ImVec4(0.38f, 0.09f, 0.28f, 1.00f);
    c[ImGuiCol_TitleBg]              = ImVec4(0.13f, 0.07f, 0.16f, 1.00f);
    c[ImGuiCol_TitleBgActive]        = ImVec4(0.55f, 0.09f, 0.32f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.13f, 0.07f, 0.16f, 0.80f);
    c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.50f, 0.10f, 0.32f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.65f, 0.13f, 0.42f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.85f, 0.18f, 0.52f, 1.00f);
    c[ImGuiCol_CheckMark]            = ImVec4(0.95f, 0.38f, 0.68f, 1.00f);
    c[ImGuiCol_SliderGrab]           = ImVec4(0.70f, 0.13f, 0.42f, 1.00f);
    c[ImGuiCol_SliderGrabActive]     = ImVec4(0.92f, 0.22f, 0.58f, 1.00f);
    c[ImGuiCol_Button]               = ImVec4(0.48f, 0.09f, 0.32f, 1.00f);
    c[ImGuiCol_ButtonHovered]        = ImVec4(0.68f, 0.13f, 0.44f, 1.00f);
    c[ImGuiCol_ButtonActive]         = ImVec4(0.90f, 0.22f, 0.58f, 1.00f);
    c[ImGuiCol_Header]               = ImVec4(0.45f, 0.09f, 0.30f, 1.00f);
    c[ImGuiCol_HeaderHovered]        = ImVec4(0.62f, 0.13f, 0.42f, 1.00f);
    c[ImGuiCol_HeaderActive]         = ImVec4(0.82f, 0.18f, 0.52f, 1.00f);
    c[ImGuiCol_Separator]            = ImVec4(0.40f, 0.08f, 0.28f, 0.80f);
    c[ImGuiCol_Text]                 = ImVec4(0.95f, 0.84f, 0.92f, 1.00f);
    c[ImGuiCol_TextDisabled]         = ImVec4(0.48f, 0.38f, 0.48f, 1.00f);
    s.WindowRounding = 5.0f; s.FrameRounding  = 4.0f;
    s.GrabRounding   = 4.0f; s.TabRounding    = 4.0f;
    s.WindowPadding  = ImVec2(12, 10);
    s.FramePadding   = ImVec2(7, 4);
    s.ItemSpacing    = ImVec2(8, 6);
}

// ---------------------------------------------------------------------------
// Editor
// ---------------------------------------------------------------------------

int editor_main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();

    SDL_Window* win = SDL_CreateWindow("Custom Editor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1200, 780, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* rend = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui_ImplSDL2_InitForSDLRenderer(win, rend);
    ImGui_ImplSDLRenderer2_Init(rend);
    ApplyTheme();

    rapidjson::Document gc;
    std::string game_title = "Custom", initial_scene = "";
    if (fs::exists("resources/game.config")) {
        EngineUtils::ReadJsonFile("resources/game.config", gc);
        game_title    = GetOptionalString(gc, "game_title").value_or("Custom");
        initial_scene = GetOptionalString(gc, "initial_scene").value_or("");
    }

    bool gm_exists = fs::exists(GM_PATH);

    // parse tile defs and grid from GameManager.lua
    auto raw_defs  = ParseTileDefsFromLua(GM_PATH);
    auto tile_defs = BuildTileDefs(raw_defs);
    Grid grid      = gm_exists ? ParseGrid(GM_PATH) : DefaultGrid();

    int  selected_tile = (tile_defs.size() > 1) ? tile_defs[1].code : 0;
    bool dirty         = false;
    bool launch_game   = false;

    auto CountTile = [&](int code) {
        int n = 0;
        for (auto& row : grid) for (int v : row) if (v == code) n++;
        return n;
    };

    // ---------------------------------------------------------------------------
    // Undo / Redo  — store up to 64 snapshots
    // ---------------------------------------------------------------------------
    static const int MAX_HISTORY = 64;
    std::deque<Grid> undo_stack;
    std::deque<Grid> redo_stack;

    // Call before any destructive edit to snapshot current state
    auto PushUndo = [&]() {
        undo_stack.push_back(grid);
        if ((int)undo_stack.size() > MAX_HISTORY)
            undo_stack.pop_front();
        redo_stack.clear();   // branching invalidates redo
        dirty = true;
    };

    auto DoUndo = [&]() {
        if (undo_stack.empty()) return;
        redo_stack.push_back(grid);
        grid = undo_stack.back();
        undo_stack.pop_back();
        dirty = true;
    };

    auto DoRedo = [&]() {
        if (redo_stack.empty()) return;
        undo_stack.push_back(grid);
        grid = redo_stack.back();
        redo_stack.pop_back();
        dirty = true;
    };

    // ---------------------------------------------------------------------------
    // Fill tool
    // ---------------------------------------------------------------------------
    bool fill_mode = false;   // toggled from sidebar

    // BFS flood fill: replace all connected cells of target_code with fill_code
    auto FloodFill = [&](int start_row, int start_col, int fill_code) {
        int target = grid[start_row][start_col];
        if (target == fill_code) return;   // nothing to do
        PushUndo();
        std::vector<std::pair<int,int>> q;
        q.push_back({start_row, start_col});
        while (!q.empty()) {
            auto [r, c] = q.back(); q.pop_back();
            if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) continue;
            if (grid[r][c] != target) continue;
            grid[r][c] = fill_code;
            q.push_back({r-1, c}); q.push_back({r+1, c});
            q.push_back({r, c-1}); q.push_back({r, c+1});
        }
    };

    // track whether the mouse was already down last frame so we only PushUndo once per stroke
    bool was_painting = false;

    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_KEYDOWN && !ImGui::GetIO().WantCaptureKeyboard) {
                bool ctrl  = (ev.key.keysym.mod & KMOD_CTRL)  != 0;
                bool shift = (ev.key.keysym.mod & KMOD_SHIFT) != 0;
                if (ctrl && ev.key.keysym.sym == SDLK_z) {
                    if (shift) DoRedo(); else DoUndo();
                }
                if (ctrl && ev.key.keysym.sym == SDLK_y) DoRedo();
            }
        }

        if (launch_game) {
            launch_game = false;
            if (dirty) { WriteGrid(GM_PATH, grid); dirty = false; }
            std::string exe(argv[0]);
#ifdef _WIN32
            std::string cmd = "start \"\" \"" + exe + "\" --play";
#else
            std::string cmd = "\"" + exe + "\" --play &";
#endif
            std::system(cmd.c_str());
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        const float sw = ImGui::GetIO().DisplaySize.x;
        const float sh = ImGui::GetIO().DisplaySize.y;

        // toolbar
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize({sw, 46});
        ImGui::Begin("##tb", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::SetCursorPosY(10);
        ImGui::TextColored({0.92f, 0.38f, 0.65f, 1.0f}, "Custom");
        ImGui::SameLine(0, 14);
        ImGui::TextDisabled("%s  |  scene: %s", game_title.c_str(), initial_scene.c_str());
        ImGui::SameLine(0, 20);

        ImGui::PushStyleColor(ImGuiCol_Button,        {0.15f, 0.50f, 0.20f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.18f, 0.65f, 0.25f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  {0.12f, 0.40f, 0.16f, 1.0f});
        if (ImGui::Button("  Play  ")) launch_game = true;
        ImGui::PopStyleColor(3);

        ImGui::SameLine(0, 10);
        if (ImGui::Button("Save")) { WriteGrid(GM_PATH, grid); dirty = false; }

        ImGui::SameLine(0, 10);
        ImGui::BeginDisabled(undo_stack.empty());
        if (ImGui::Button("Undo")) DoUndo();
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Undo  (Ctrl+Z)");

        ImGui::SameLine(0, 4);
        ImGui::BeginDisabled(redo_stack.empty());
        if (ImGui::Button("Redo")) DoRedo();
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
            ImGui::SetTooltip("Redo  (Ctrl+Y / Ctrl+Shift+Z");

        if (dirty) {
            ImGui::SameLine(0, 14);
            ImGui::TextColored({1.0f, 0.72f, 0.20f, 1.0f}, "● unsaved");
        }
        if (!gm_exists) {
            ImGui::SameLine(0, 14);
            ImGui::TextColored({1.0f, 0.35f, 0.35f, 1.0f}, "GameManager.lua not found at %s", GM_PATH.c_str());
        }

        ImGui::End();

        // sidebar — brush picker and tile counts
        const float TOOLBAR_H = 46.0f;
        const float SIDEBAR_W = 190.0f;

        ImGui::SetNextWindowPos({0, TOOLBAR_H});
        ImGui::SetNextWindowSize({SIDEBAR_W, sh - TOOLBAR_H});
        ImGui::Begin("##sidebar", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::TextColored({0.92f, 0.38f, 0.65f, 1.0f}, "Brush");
        ImGui::Separator();
        ImGui::Spacing();

        for (int ti = 0; ti < (int)tile_defs.size(); ti++) {
            auto& td    = tile_defs[ti];
            bool is_sel = (selected_tile == td.code);

            ImGui::PushID(ti);
            ImGui::PushStyleColor(ImGuiCol_Button,
                is_sel ? ImVec4(td.color.x*1.3f, td.color.y*1.3f, td.color.z*1.3f, 1.0f) : td.color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                ImVec4(td.color.x*1.4f, td.color.y*1.4f, td.color.z*1.4f, 1.0f));
            if (ImGui::Button(td.label.c_str(), {SIDEBAR_W - 24, 32}))
                selected_tile = td.code;
            ImGui::PopStyleColor(2);
            ImGui::PopID();
            ImGui::Spacing();
        }

        ImGui::Spacing();
        ImGui::Separator();

        // Fill tool toggle
        ImGui::Spacing();
        ImGui::TextColored({0.92f, 0.38f, 0.65f, 1.0f}, "Tool");
        ImGui::Separator();
        ImGui::Spacing();

        if (fill_mode) {
            ImGui::PushStyleColor(ImGuiCol_Button,        {0.20f, 0.50f, 0.80f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.25f, 0.62f, 0.95f, 1.0f});
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImGui::GetStyleColorVec4(ImGuiCol_Button));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
        }
        if (ImGui::Button(fill_mode ? "Fill (ON)" : "Fill (OFF)", {SIDEBAR_W - 24, 28}))
            fill_mode = !fill_mode;
        ImGui::PopStyleColor(2);
        ImGui::TextDisabled(fill_mode ? "Click to flood fill" : "Click to paint");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored({0.92f, 0.38f, 0.65f, 1.0f}, "Map Stats");
        ImGui::Separator();
        ImGui::Spacing();

        // dynamic stats — one line per non-empty tile type
        for (auto& td : tile_defs) {
            if (td.code == 0) continue;
            ImGui::Text("%-14s %d", td.label.c_str(), CountTile(td.code));
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextDisabled("Left click: paint/fill");
        ImGui::TextDisabled("Right click: erase/clear");
        ImGui::TextDisabled("Hold to drag-paint");
        ImGui::TextDisabled("Ctrl+Z: undo");
        ImGui::TextDisabled("Ctrl+Y: redo");

        ImGui::End();

        // the actual tile grid
        const float GRID_X = SIDEBAR_W;
        const float GRID_Y = TOOLBAR_H;
        const float GRID_W = sw - SIDEBAR_W;
        const float GRID_H = sh - TOOLBAR_H;

        ImGui::SetNextWindowPos({GRID_X, GRID_Y});
        ImGui::SetNextWindowSize({GRID_W, GRID_H});
        ImGui::Begin("##grid", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 origin  = ImGui::GetCursorScreenPos();

        // fit the grid into the window with a little breathing room
        float avail_w = GRID_W - 24;
        float avail_h = GRID_H - 24;
        float cell    = std::min(avail_w / GRID_COLS, avail_h / GRID_ROWS);

        float grid_px_w = cell * GRID_COLS;
        float grid_px_h = cell * GRID_ROWS;
        float off_x = (avail_w - grid_px_w) * 0.5f + 12;
        float off_y = (avail_h - grid_px_h) * 0.5f + 12;

        // invisible button so ImGui tracks hover/click over the grid area
        ImGui::InvisibleButton("##gridarea", {avail_w + 12, avail_h + 12});
        bool grid_hovered = ImGui::IsItemHovered();
        bool lmb = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        bool rmb = ImGui::IsMouseDown(ImGuiMouseButton_Right);

        ImVec2 mouse = ImGui::GetMousePos();

        // paint, erase, or fill on click/drag
        if (grid_hovered && (lmb || rmb)) {
            float rel_x = mouse.x - origin.x - off_x;
            float rel_y = mouse.y - origin.y - off_y;
            int col = (int)(rel_x / cell);
            int row = (int)(rel_y / cell);
            if (row >= 0 && row < GRID_ROWS && col >= 0 && col < GRID_COLS) {
                if (fill_mode) {
                    // flood fill only fires on the initial click, not on drag
                    if (!was_painting) {
                        int fill_code = lmb ? selected_tile : 0;
                        FloodFill(row, col, fill_code);
                    }
                } else {
                    int new_val = lmb ? selected_tile : 0;
                    if (grid[row][col] != new_val) {
                        if (!was_painting) PushUndo();   // snapshot once per stroke start
                        grid[row][col] = new_val;
                    }
                }
            }
        }
        was_painting = grid_hovered && (lmb || rmb);

        // draw cells
        for (int r = 0; r < GRID_ROWS; r++) {
            for (int c = 0; c < GRID_COLS; c++) {
                float px  = origin.x + off_x + c * cell;
                float py  = origin.y + off_y + r * cell;
                ImVec4 col4 = TileColor(tile_defs, grid[r][c]);
                ImU32 fill  = IM_COL32((int)(col4.x*255), (int)(col4.y*255), (int)(col4.z*255), 255);
                dl->AddRectFilled({px+1, py+1}, {px+cell-1, py+cell-1}, fill, 2.0f);

                // first letter of the tile name so you can tell them apart at small sizes
                if (grid[r][c] != 0) {
                    const char* lbl = TileLabel(tile_defs, grid[r][c]);
                    char ch[2] = {lbl[0], '\0'};
                    ImVec2 ts = ImGui::CalcTextSize(ch);
                    dl->AddText({px + (cell-ts.x)*0.5f, py + (cell-ts.y)*0.5f},
                        IM_COL32(255, 255, 255, 180), ch);
                }
            }
        }

        // grid lines
        ImU32 grid_line_col = IM_COL32(60, 40, 70, 255);
        for (int r = 0; r <= GRID_ROWS; r++) {
            float py = origin.y + off_y + r * cell;
            dl->AddLine({origin.x + off_x, py}, {origin.x + off_x + grid_px_w, py}, grid_line_col);
        }
        for (int c = 0; c <= GRID_COLS; c++) {
            float px = origin.x + off_x + c * cell;
            dl->AddLine({px, origin.y + off_y}, {px, origin.y + off_y + grid_px_h}, grid_line_col);
        }

        // highlight the cell under the cursor and show its coords
        if (grid_hovered) {
            float rel_x = mouse.x - origin.x - off_x;
            float rel_y = mouse.y - origin.y - off_y;
            int hc = (int)(rel_x / cell);
            int hr = (int)(rel_y / cell);
            if (hr >= 0 && hr < GRID_ROWS && hc >= 0 && hc < GRID_COLS) {
                float px = origin.x + off_x + hc * cell;
                float py = origin.y + off_y + hr * cell;
                dl->AddRect({px+1, py+1}, {px+cell-1, py+cell-1},
                    IM_COL32(255, 255, 255, 120), 2.0f, 0, 2.0f);
                ImGui::SetTooltip("(%d, %d)  %s", hc+1, hr+1, TileLabel(tile_defs, grid[hr][hc]));
            }
        }

        ImGui::End();

        ImGui::Render();
        SDL_SetRenderDrawColor(rend, 16, 11, 20, 255);
        SDL_RenderClear(rend);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), rend);
        SDL_RenderPresent(rend);
    }

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

// original game loop, nothing changed here
int game_main(int argc, char* argv[]) {
    rapidjson::Document game_config, rendering_config;
    Input input_manager; input_manager.Init();
    SceneDB scene_db; Renderer sdl_renderer; ImageDB img_db;
    TemplateDB template_db; TextDB text_db; AudioDB audio_db; ComponentDB comp_db;
    Actor::component_db = &comp_db;
    Actor::template_db  = &template_db;
    Actor::scene_db     = &scene_db;
    bool saw_quit = false;
    mandatoryReads(game_config);
    auto get_init = GetOptionalString(game_config, "initial_scene");
    if (!get_init) exit(0);
    std::string scene = *get_init;
    std::string init_scene = "resources/scenes/" + scene + ".scene";
    if (!fs::exists(init_scene)) exit(0);
    std::string title = GetOptionalString(game_config, "game_title").value_or("");
    int ww=640, wh=360, cr=255, cg=255, cb=255;
    if (fs::exists("resources/rendering.config")) {
        EngineUtils::ReadJsonFile("resources/rendering.config", rendering_config);
        if (auto v = GetOptionalInt(rendering_config, "x_resolution"))  ww = *v;
        if (auto v = GetOptionalInt(rendering_config, "y_resolution"))  wh = *v;
        if (auto v = GetOptionalInt(rendering_config, "clear_color_r")) cr = *v;
        if (auto v = GetOptionalInt(rendering_config, "clear_color_g")) cg = *v;
        if (auto v = GetOptionalInt(rendering_config, "clear_color_b")) cb = *v;
    }
    sdl_renderer.Init(title, ww, wh, cr, cg, cb);
    scene_db.SetRefs(sdl_renderer.renderer, img_db, template_db, comp_db);
    template_db.LoadAll("resources/actor_templates");
    scene_db.LoadSceneFile(init_scene, sdl_renderer.renderer, img_db, template_db, comp_db);
    SceneDB::current_scene_name = scene;
    while (true) {
        scene_db.CheckAndLoadPendingScene();
        SDL_Event e;
        while (Helper::SDL_PollEvent(&e)) input_manager.ProcessEvent(e, saw_quit);
        scene_db.SetLifecycleCount();
        scene_db.RunComponentOnStarts();
        scene_db.RunOnUpdates();
        scene_db.ProcessPendingChanges();
        EventBus::ProcessPending();
        scene_db.StepPhysics();
        scene_db.ProcessCollisionEvents();
        scene_db.RunOnLateUpdates();
        SDL_SetRenderDrawColor(sdl_renderer.renderer, (Uint8)cr, (Uint8)cg, (Uint8)cb, 255);
        SDL_RenderClear(sdl_renderer.renderer);
        img_db.RenderAndClearImages(sdl_renderer.renderer);
        img_db.RenderAndClearUI(sdl_renderer.renderer);
        text_db.Flush(sdl_renderer.renderer);
        img_db.RenderAndClearPixels(sdl_renderer.renderer, cr, cg, cb);
        Helper::SDL_RenderPresent(sdl_renderer.renderer);
        if (saw_quit) exit(0);
        input_manager.LateUpdate();
    }
    return 0;
}

// no args = editor, --play = game
int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++)
        if (std::string(argv[i]) == "--play") return game_main(argc, argv);
    return editor_main(argc, argv);
}

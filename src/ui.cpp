#include "ui.h"
#include "types.h"
#define NO_FONT_AWESOME
#include "imgui.h"
#include "rlImGui.h"

extern cpu_t cpu;
extern ppu_t ppu;
extern spc_t spc;

char bp_inter[7] = {0};
void cpu_window(void) {
    ImGui::Begin("cpu", NULL);
    ImGui::Text("PC: 0x%04x Opcode: 0x%02x", cpu.pc, read_8(cpu.pc, cpu.pbr));
    if (ImGui::Button("Start"))
        cpu.state = STATE_RUNNING;
    ImGui::SameLine();
    if (ImGui::Button("Stop"))
        cpu.state = STATE_STOPPED;
    ImGui::SameLine();
    if (ImGui::Button("Step"))
        cpu.state = STATE_STEPPED;
    ImGui::NewLine();
    ImGui::Text("Mode: %s", cpu.emulation_mode ? "emulation" : "native");
    ImGui::Text((cpu.emulation_mode || get_status_bit(STATUS_MEMNARROW))
                    ? "C: 0x%02x"
                    : "C: 0x%04x",
                cpu.c);
    ImGui::Text((cpu.emulation_mode || get_status_bit(STATUS_XNARROW))
                    ? "X: 0x%02x"
                    : "X: 0x%04x",
                cpu.x);
    ImGui::Text((cpu.emulation_mode || get_status_bit(STATUS_XNARROW))
                    ? "Y: 0x%02x"
                    : "Y: 0x%04x",
                cpu.y);
    ImGui::Text("D: 0x%04x", cpu.d);
    ImGui::Text("SP: 0x%04x", cpu.s);
    ImGui::Text("P: 0x%02x", cpu.p);

    ImGui::NewLine();

    ImGui::Text("Breakpoint: 0x");
    ImGui::SameLine();
    ImGui::PushItemWidth(4 * ImGui::GetFontSize());
    ImGui::InputText("##bpin", bp_inter, 7);
    ImGui::PopItemWidth();
    cpu.breakpoint_valid = true;
    for (char &c : bp_inter) {
        if ((c < '0' || c > '9') && (c < 'a' || c > 'f') &&
            (c < 'A' || c > 'F'))
            cpu.breakpoint_valid = false;
        break;
    }
    if (!cpu.breakpoint_valid) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4{0xff, 0, 0, 0xff}, "Invalid!");
    } else {
        cpu.breakpoint = strtoul(bp_inter, NULL, 16);
    }

    ImGui::End();
}

char spc_bp_inter[5] = {0};
void spc_window(void) {
    ImGui::Begin("spc", NULL);
    ImGui::Text("PC: 0x%04x Opcode: 0x%02x", spc.pc, spc_read_8(spc.pc));
    ImGui::Text("A: 0x%02x", spc.a);
    ImGui::Text("X: 0x%02x", spc.x);
    ImGui::Text("Y: 0x%02x", spc.y);
    ImGui::Text("SP: 0x%02x", spc.s);
    ImGui::Text("P: 0x%02x", spc.p);
    ImGui::Text("Breakpoint: 0x");
    ImGui::SameLine();
    ImGui::PushItemWidth(4 * ImGui::GetFontSize());
    ImGui::InputText("##spcbpin", spc_bp_inter, 5);
    ImGui::PopItemWidth();
    spc.breakpoint_valid = true;
    for (char &c : spc_bp_inter) {
        if ((c < '0' || c > '9') && (c < 'a' || c > 'f') &&
            (c < 'A' || c > 'F'))
            spc.breakpoint_valid = false;
        break;
    }
    if (!spc.breakpoint_valid) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4{0xff, 0, 0, 0xff}, "Invalid!");
    } else {
        spc.breakpoint = strtoul(spc_bp_inter, NULL, 16);
    }
    ImGui::End();
}

int32_t spc_page = 0;
void spc_ram_window(void) {
    ImGui::Begin("spc ram");
    ImGui::InputInt("Page", &spc_page);
    if(spc_page < 0) spc_page = 0;
    if(spc_page > 0xff) spc_page = 0xff;
    if(ImGui::BeginTable("##spcram", 16, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        for(uint16_t i = 0; i < 16; i++) {
            for(uint8_t j = 0; j < 16; j++) {
                ImGui::TableNextColumn();
                ImGui::Text("%02x", spc.memory.ram[spc_page * 0x100 + i * 16 + j]);
                if(ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("0x%04x", spc_page * 0x100 + i * 16 + j);
                }
            }
            if(i != 15) ImGui::TableNextRow();
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

extern "C" {
void cpp_init(void) {
    rlImGuiSetup(true);
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().FontGlobalScale *= 2;
}

void cpp_imgui_render(void) {
    rlImGuiBegin();
    spc_window();
    spc_ram_window();
    cpu_window();
    rlImGuiEnd();
}

void cpp_end(void) { rlImGuiShutdown(); }
}

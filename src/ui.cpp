#include "ui.h"
#include "types.h"
#define NO_FONT_AWESOME
#include "imgui.h"
#include "rlImGui.h"

extern cpu_t cpu;
extern ppu_t ppu;

bool show_cpu_window = false;
void cpu_window(void) {
    ImGui::Begin("cpu", NULL);
    ImGui::SetWindowFontScale(2);
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

    ImGui::End();
}

extern "C" {
void cpp_init(void) { rlImGuiSetup(true); }

void cpp_imgui_render(void) {
    rlImGuiBegin();
    ImGui::Begin("master", NULL, ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowFontScale(2);

    if (ImGui::Button("CPU"))
        show_cpu_window = !show_cpu_window;

    ImGui::End();

    if (show_cpu_window)
        cpu_window();
    rlImGuiEnd();
}

void cpp_end(void) { rlImGuiShutdown(); }
}

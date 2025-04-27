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
    ImGui::Begin("cpu", NULL, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::Text("PC: 0x%04x Opcode: 0x%02x", cpu.pc,
                read_8_no_log(cpu.pc, cpu.pbr));
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
    ImGui::Text("C: 0x%04x, %s", cpu.c,
                (cpu.emulation_mode || get_status_bit(STATUS_MEMNARROW))
                    ? "short"
                    : "long");
    ImGui::Text("X: 0x%04x, %s", cpu.x,
                (cpu.emulation_mode || get_status_bit(STATUS_XNARROW))
                    ? "short"
                    : "long");
    ImGui::Text("Y: 0x%04x, %s", cpu.y,
                (cpu.emulation_mode || get_status_bit(STATUS_XNARROW))
                    ? "short"
                    : "long");
    ImGui::Text("D: 0x%04x", cpu.d);
    ImGui::Text("SP: 0x%04x", cpu.s);
    ImGui::Text("P: 0x%02x", cpu.p);
    ImGui::Text("Instruction Bank: 0x%02x", cpu.pbr);
    ImGui::Text("Data Bank: 0x%02x", cpu.dbr);

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

int bg_selected = 0;
void ppu_window(void) {
    ImGui::Begin("ppu", NULL, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::InputInt("Channel", &bg_selected, 1, 1,
                    ImGuiInputTextFlags_CharsDecimal);
    if (bg_selected > 3)
        bg_selected = 3;
    if (bg_selected < 0)
        bg_selected = 0;

    ImGui::Text("Tile Data Addr: 0x%02x",
                ppu.bg_config[bg_selected].tiledata_addr);
    ImGui::Text("Tile Map Addr: 0x%02x",
                ppu.bg_config[bg_selected].tilemap_addr);
    ImGui::End();
}

int32_t vram_page = 0;
void vram_window(void) {
    ImGui::Begin("vram", NULL, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::InputInt("Page", &vram_page, 1, 16,
                    ImGuiInputTextFlags_CharsHexadecimal);
    if (vram_page < 0)
        vram_page = 0;
    if (vram_page > 0xff)
        vram_page = 0xff;
    if (ImGui::BeginTable("##vram", 16,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        for (uint16_t i = 0; i < 16; i++) {
            for (uint8_t j = 0; j < 16; j++) {
                ImGui::TableNextColumn();
                ImGui::Text("%02x", ppu.vram[vram_page * 0x100 + i * 16 + j]);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("0x%04x", vram_page * 0x100 + i * 16 + j);
                }
            }
            if (i != 15)
                ImGui::TableNextRow();
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

char spc_bp_inter[5] = {0};
void spc_window(void) {
    ImGui::Begin("spc", NULL, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::Text("PC: 0x%04x Opcode: 0x%02x", spc.pc, spc_read_8_no_log(spc.pc));
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
    ImGui::Text("Timer 0 enable: %s",
                spc.memory.timers[0].enable ? "true" : "false");
    ImGui::Text("Timer 0 timer: %d", spc.memory.timers[0].timer_internal);
    ImGui::Text("Timer 0 modulo: %d", spc.memory.timers[0].timer);
    ImGui::Text("Timer 0 counter: %d", spc.memory.timers[0].counter);
    ImGui::Text("Timer 1 enable: %s",
                spc.memory.timers[1].enable ? "true" : "false");
    ImGui::Text("Timer 1 timer: %d", spc.memory.timers[1].timer_internal);
    ImGui::Text("Timer 1 modulo: %d", spc.memory.timers[1].timer);
    ImGui::Text("Timer 1 counter: %d", spc.memory.timers[1].counter);
    ImGui::Text("Timer 2 enable: %s",
                spc.memory.timers[2].enable ? "true" : "false");
    ImGui::Text("Timer 2 timer: %d", spc.memory.timers[2].timer_internal);
    ImGui::Text("Timer 2 modulo: %d", spc.memory.timers[2].timer);
    ImGui::Text("Timer 2 counter: %d", spc.memory.timers[2].counter);
    ImGui::End();
}

int spc_ram_page = 0;
void spc_ram_window(void) {
    ImGui::Begin("spc ram", NULL, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::InputInt("Page", &spc_ram_page, 0, 0,
                    ImGuiInputTextFlags_CharsHexadecimal);
    if (spc_ram_page < 0)
        spc_ram_page = 0;
    if (spc_ram_page > 0xff)
        spc_ram_page = 0xff;
    if (ImGui::BeginTable("##spcram", 16,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        for (uint16_t i = 0; i < 16; i++) {
            for (uint8_t j = 0; j < 16; j++) {
                ImGui::TableNextColumn();
                ImGui::Text("%02x",
                            spc.memory.ram[spc_ram_page * 0x100 + i * 16 + j]);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("0x%04x",
                                      spc_ram_page * 0x100 + i * 16 + j);
                }
            }
            if (i != 15)
                ImGui::TableNextRow();
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

int cpu_ram_bank = 0;
int cpu_ram_page = 0;
void cpu_ram_window(void) {
    ImGui::Begin("cpu ram", NULL, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::InputInt("Bank", &cpu_ram_bank, 0, 0,
                    ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    ImGui::InputInt("Page", &cpu_ram_page, 0, 0,
                    ImGuiInputTextFlags_CharsHexadecimal);
    if (cpu_ram_page < 0)
        cpu_ram_page = 0;
    if (cpu_ram_page > 0xff) {
        cpu_ram_page = 0;
        cpu_ram_bank++;
    }
    if (cpu_ram_bank < 0)
        cpu_ram_bank = 0;
    if (cpu_ram_bank > 1)
        cpu_ram_bank = 1;
    if (ImGui::BeginTable("##cpuram", 16,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        for (uint16_t i = 0; i < 16; i++) {
            for (uint8_t j = 0; j < 16; j++) {
                ImGui::TableNextColumn();
                ImGui::Text("%02x",
                            cpu.memory.ram[cpu_ram_bank * 0x10000 +
                                           cpu_ram_page * 0x100 + i * 16 + j]);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("0x%06x", cpu_ram_bank * 0x10000 +
                                                    cpu_ram_page * 0x100 +
                                                    i * 16 + j);
                }
            }
            if (i != 15)
                ImGui::TableNextRow();
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

int cpu_rom_bank = 0;
int cpu_rom_page = 0;
void cpu_rom_window(void) {
    ImGui::Begin("cpu rom", NULL, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::InputInt("Bank", &cpu_rom_bank, 0, 0,
                    ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    ImGui::InputInt("Page", &cpu_rom_page, 0, 0,
                    ImGuiInputTextFlags_CharsHexadecimal);
    if (cpu_rom_page < 0x80)
        cpu_rom_page = 0x80;
    if (cpu_rom_page > 0xff) {
        cpu_rom_page = 0xff;
        cpu_rom_bank++;
    }
    if (cpu_rom_bank == 0x7e)
        cpu_rom_bank = 0x80;
    if (cpu_rom_bank == 0x7f)
        cpu_rom_bank = 0x7d;
    if (cpu_rom_bank < 0)
        cpu_rom_bank = 0;
    if (cpu_rom_bank > 0xff)
        cpu_rom_bank = 0xff;
    if (ImGui::BeginTable("##cpurom", 16,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        if (cpu.memory.mode != LOROM)
            TODO("other modes");
        for (uint16_t i = 0; i < 16; i++) {
            for (uint8_t j = 0; j < 16; j++) {
                ImGui::TableNextColumn();
                ImGui::Text("%02x", cpu.memory.rom[lo_rom_resolve(
                                        cpu_rom_bank * 0x10000 +
                                            cpu_rom_page * 0x100 + i * 16 + j,
                                        false)]);
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("0x%06x", cpu_rom_bank * 0x10000 +
                                                    cpu_rom_page * 0x100 +
                                                    i * 16 + j);
                }
            }
            if (i != 15)
                ImGui::TableNextRow();
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

int dsp_selected = 0;
void dsp_window(void) {
    ImGui::Begin("dsp", NULL, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::InputInt("Channel", &dsp_selected, 1, 1,
                    ImGuiInputTextFlags_CharsDecimal);
    if (dsp_selected > 7)
        dsp_selected = 7;
    if (dsp_selected < 0)
        dsp_selected = 0;
    ImGui::Text("Volume left: %d", spc.memory.channels[dsp_selected].vol_left);
    ImGui::Text("Volume right: %d",
                spc.memory.channels[dsp_selected].vol_right);
    ImGui::Text("Pitch: %d", spc.memory.channels[dsp_selected].pitch);
    ImGui::Text("Sample Source Directory: 0x%02x",
                spc.memory.channels[dsp_selected].sample_source_directory);
    ImGui::Text("ADSR %sabled",
                spc.memory.channels[dsp_selected].adsr_enable ? "en" : "dis");
    if (spc.memory.channels[dsp_selected].adsr_enable) {
        ImGui::Text("A: %d", spc.memory.channels[dsp_selected].a_rate);
        ImGui::Text("D: %d", spc.memory.channels[dsp_selected].d_rate);
        ImGui::Text("S: %d", spc.memory.channels[dsp_selected].s_rate);
        ImGui::Text("R: %d", spc.memory.channels[dsp_selected].r_rate);
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
    vram_window();
    ppu_window();
    cpu_ram_window();
    cpu_rom_window();
    cpu_window();
    dsp_window();
    rlImGuiEnd();
}

void cpp_end(void) { rlImGuiShutdown(); }
}

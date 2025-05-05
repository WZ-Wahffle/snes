#include "ui.h"
#include "types.h"
#include <string>
#include <vector>
#define NO_FONT_AWESOME
#include "imgui.h"
#include "rlImGui.h"

extern cpu_t cpu;
extern ppu_t ppu;
extern spc_t spc;

std::vector<breakpoint_t> cpu_bp;
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
    ImGui::SameLine();
    if (ImGui::Button("Dump State")) {
        for (uint16_t i = cpu.history_idx; i != cpu.history_idx - 1; i++) {
            printf("0x%06x: 0x%02x\n", cpu.pc_history[i],
                   cpu.opcode_history[i]);
        }
    }
    ImGui::Text("Speed: %.4lfx", cpu.speed);
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
    if (ImGui::Button("+##cpubpadd")) {
        cpu_bp.push_back(breakpoint_t{0, {0}, 0, 0, 0, 0});
        cpu.breakpoints = cpu_bp.data();
        cpu.breakpoints_size = cpu_bp.size();
    }
    bool remove = false;
    uint32_t to_remove = 0;
    for (uint32_t i = 0; i < cpu_bp.size(); i++) {
        ImGui::Text("0x");
        ImGui::SameLine();
        ImGui::PushItemWidth(4 * ImGui::GetFontSize());
        ImGui::InputText((std::string("##cpubpin") + std::to_string(i)).c_str(),
                         cpu_bp[i].bp_inter, 7);
        ImGui::PopItemWidth();
        cpu_bp[i].valid = true;
        for (char &c : cpu_bp[i].bp_inter) {
            if ((c < '0' || c > '9') && (c < 'a' || c > 'f') &&
                (c < 'A' || c > 'F'))
                cpu_bp[i].valid = false;
            break;
        }
        if (!cpu_bp[i].valid) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4{0xff, 0, 0, 0xff}, "Invalid!");
            ImGui::SameLine();
        } else {
            cpu_bp[i].line = strtoul(cpu_bp[i].bp_inter, NULL, 16);
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                 ImGui::CalcTextSize(" Invalid!").x);
        }
        ImGui::Checkbox((std::string("R##cpubpr") + std::to_string(i)).c_str(),
                        &cpu_bp[i].read);
        ImGui::SameLine();
        ImGui::Checkbox((std::string("W##cpubpw") + std::to_string(i)).c_str(),
                        &cpu_bp[i].write);
        ImGui::SameLine();
        ImGui::Checkbox((std::string("X##cpubpx") + std::to_string(i)).c_str(),
                        &cpu_bp[i].execute);
        ImGui::SameLine();
        if (ImGui::Button(
                (std::string("-##cpubprm") + std::to_string(i)).c_str())) {
            remove = true;
            to_remove = i;
        }
    }

    if (remove) {
        cpu_bp.erase(cpu_bp.begin() + to_remove);
        cpu.breakpoints = cpu_bp.data();
        cpu.breakpoints_size = cpu_bp.size();
    }

    ImGui::End();
}

void ppu_window(void) {
    ImGui::Begin("ppu", NULL, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::Text("BG Mode: %d", ppu.bg_mode);
    ImGui::Text("VRAM Address: 0x%04x", ppu.vram_addr);
    ImGui::Text("VRAM Address Remapping Index: %d", ppu.address_remapping);
    ImGui::Text("VRAM Address Increment Amount Index: %d",
                ppu.address_increment_amount);
    ImGui::Text("Window 1: %d-%d", ppu.window_1_l, ppu.window_1_r);
    ImGui::Text("Window 2: %d-%d", ppu.window_2_l, ppu.window_2_r);
    ImGui::End();
}

int bg_selected = 0;
void bg_window(void) {
    ImGui::Begin("bg", NULL, ImGuiWindowFlags_HorizontalScrollbar);

    ImGui::InputInt("BG Layer", &bg_selected, 1, 1,
                    ImGuiInputTextFlags_CharsDecimal);
    if (bg_selected > 3)
        bg_selected = 3;
    if (bg_selected < 0)
        bg_selected = 0;

    ImGui::Text("Tile Data Addr: 0x%02x",
                ppu.bg_config[bg_selected].tiledata_addr);
    ImGui::Text("Tile Map Addr: 0x%02x",
                ppu.bg_config[bg_selected].tilemap_addr);
    ImGui::Text("h: %d, v: %d", ppu.bg_config[bg_selected].double_h_tilemap + 1,
                ppu.bg_config[bg_selected].double_v_tilemap + 1);
    ImGui::Text("X Scroll: %d", ppu.bg_config[bg_selected].h_scroll);
    ImGui::Text("Y Scroll: %d", ppu.bg_config[bg_selected].v_scroll);
    ImGui::Text("Tile Size: %dpx",
                ppu.bg_config[bg_selected].large_characters ? 16 : 8);
    ImGui::Text("Window 1 %sabled",
                ppu.bg_config[bg_selected].window_1_enable ? "en" : "dis");
    ImGui::Text("Window 2 %sabled",
                ppu.bg_config[bg_selected].window_2_enable ? "en" : "dis");
    ImGui::End();
}

int vram_page = 0;
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

void oam_window(void) {
    ImGui::Begin("oam", NULL, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::Text("DEF Nametable: 0x%04x", ppu.obj_name_base_address << 14);
    ImGui::Text("ALT Nametable: 0x%04x", (ppu.obj_name_base_address << 14) +
                                             ((ppu.obj_name_select + 1) << 13));
    ImGui::Text("Sprite size index: %d", ppu.obj_sprite_size);
    if (ImGui::BeginTable("##oam", 6,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("X", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Y", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("TileIdx", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Palette", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("AltSize", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn("Prio", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();

        ImGui::TableNextRow();
        for (uint16_t i = 0; i < 128; i++) {
            ImGui::TableNextColumn();
            ImGui::Text("%4d", ppu.oam[i].x);
            ImGui::TableNextColumn();
            ImGui::Text("%4d", ppu.oam[i].y);
            ImGui::TableNextColumn();
            ImGui::Text("0x%03x", ppu.oam[i].tile_idx |
                                      (ppu.oam[i].use_second_sprite_page << 8));
            ImGui::TableNextColumn();
            ImGui::Text("0x%02x", ppu.oam[i].palette);
            ImGui::TableNextColumn();
            ImGui::Text("%c", ppu.oam[i].use_second_size ? 'X' : ' ');
            ImGui::TableNextColumn();
            ImGui::Text("%d", ppu.oam[i].priority);

            if (i != 127) {
                ImGui::TableNextRow();
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

int dma_selected = 0;
void dma_window(void) {
    ImGui::Begin("dma", NULL, ImGuiWindowFlags_HorizontalScrollbar);
    ImGui::InputInt("Page", &dma_selected, 1, 1,
                    ImGuiInputTextFlags_CharsHexadecimal);
    if (dma_selected < 0)
        dma_selected = 0;
    if (dma_selected > 7)
        dma_selected = 7;
    ImGui::Text("HDMA: %s",
                cpu.memory.dmas[dma_selected].hdma_enable ? "true" : "false");
    ImGui::Text("Direction: %s",
                cpu.memory.dmas[dma_selected].direction ? "B -> A" : "A -> B");
    ImGui::Text("Indirect: %s",
                cpu.memory.dmas[dma_selected].indirect_hdma ? "true" : "false");
    ImGui::Text("Address adjust mode: %d",
                cpu.memory.dmas[dma_selected].addr_inc_mode);
    ImGui::Text("Transfer pattern: %d",
                cpu.memory.dmas[dma_selected].transfer_pattern);
    ImGui::Text("B Address: 0x%04x",
                0x2100 + cpu.memory.dmas[dma_selected].b_bus_addr);
    ImGui::Text("A Address: 0x%06x",
                cpu.memory.dmas[dma_selected].dma_src_addr);
    ImGui::Text("Byte Count: 0x%04x",
                cpu.memory.dmas[dma_selected].dma_byte_count);
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
    cpu.breakpoints = cpu_bp.data();
    cpu.breakpoints_size = cpu_bp.size();
}

void cpp_imgui_render(void) {
    rlImGuiBegin();
    spc_window();
    spc_ram_window();
    vram_window();
    dma_window();
    ppu_window();
    bg_window();
    oam_window();
    cpu_ram_window();
    cpu_rom_window();
    cpu_window();
    dsp_window();
    rlImGuiEnd();
}

void cpp_end(void) { rlImGuiShutdown(); }
}

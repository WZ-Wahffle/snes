#ifndef UI_H_
#define UI_H_

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC void cpp_init(void);
EXTERNC void cpp_imgui_render(void);
EXTERNC void cpp_end(void);

#endif

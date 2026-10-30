#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "talk.h"
#include "call_state.h"
#include "mic_input.h"
#include "updater.h"
#define GUI_WIDTH 1280
#define GUI_HEIGHT 720
#define GUI_ROOMS_PER_PAGE 8
/* View values match the application state machine. UI never owns account data. */
typedef struct {
 const TalkClient *client;
 int view,room_sel,room_page,message_scroll,current_room;
 bool busy;
 const char *status;
} GuiModel;
typedef enum {
 GUI_NONE,GUI_CONNECT,GUI_EDIT,GUI_MIC,GUI_REFRESH,GUI_BACK,GUI_COMPOSE,GUI_CALL,
 GUI_SCROLL_UP,GUI_SCROLL_DOWN,GUI_SELECT_ROOM,GUI_PAGE_UP,GUI_PAGE_DOWN,
 GUI_MESSAGE,GUI_MUTE,GUI_HANGUP,GUI_EXIT,GUI_MIC_TOGGLE,GUI_MIC_RESCAN,GUI_MIC_PREV,GUI_MIC_NEXT,
 GUI_EDIT_SERVER,GUI_EDIT_USER,GUI_EDIT_PASSWORD,GUI_UPDATES,GUI_UPDATE_CHECK,GUI_UPDATE_AUTO,GUI_UPDATE_RESTORE
} GuiAction;
bool gui_init(void);
void gui_exit(void);
void gui_app(const GuiModel *model);
int gui_message_lines(const TalkMessage *message);
void gui_message(const TalkMessage *message,int offset);
void gui_call_room(const TalkRoom *room);
void gui_call(const CallState *state,uint32_t result,bool ending);
void gui_mic(const MicState *mic);
void gui_updates(const GuiUpdateModel *model);
void gui_notice(const char *title,const char *message);
GuiAction gui_touch(const GuiModel *model,int *selected);
/* Shared hit testing also drives host-side touch tests. */
GuiAction gui_hit_test(int x,int y,int *selected);
#ifdef GUI_HEADLESS
void gui_test_buffer(uint32_t *buffer,unsigned stride_pixels);
#endif

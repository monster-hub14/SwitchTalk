/* The microphone screen starts real capture immediately on entry. */
#include <switch.h>
#include <stdio.h>
#include "audio_test.h"
#include "mic_input.h"
#include "gui.h"
static MicState mic;
bool audio_test_run(void){
    gui_notice("Microphone test","Opening microphone input...");
    mic_rescan(&mic);
    PadState pad;padInitializeDefault(&pad);bool exit_app=false;
    while(appletMainLoop()){
        padUpdate(&pad);u64 down=padGetButtonsDown(&pad);
        GuiAction action=gui_touch(NULL,NULL);
        if(action==GUI_BACK)down|=HidNpadButton_B;
        if(action==GUI_MIC_TOGGLE)down|=HidNpadButton_A;
        if(action==GUI_MIC_RESCAN)down|=HidNpadButton_X;
        if(action==GUI_MIC_PREV)down|=HidNpadButton_Left;
        if(action==GUI_MIC_NEXT)down|=HidNpadButton_Right;
        if(down&HidNpadButton_Plus){exit_app=true;break;}
        if(down&HidNpadButton_B)break;
        if(down&HidNpadButton_X)mic_rescan(&mic);
        else if(down&HidNpadButton_A){if(mic.capturing)mic_stop(&mic);else mic_select(&mic,mic.selected);}
        else if(down&HidNpadButton_Left && mic.device_count)mic_select(&mic,(mic.selected+mic.device_count-1)%mic.device_count);
        else if(down&HidNpadButton_Right && mic.device_count)mic_select(&mic,(mic.selected+1)%mic.device_count);
        mic_update(&mic);gui_mic(&mic);svcSleepThread(33000000);
    }
    mic_stop(&mic);return exit_app;
}

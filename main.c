#include <psp2/kernel/modulemgr.h>
#include <psp2/ctrl.h>
#include <taihen.h>
#include <vitasdk.h>
#include "blit.h"
#include "log.h"
#include "config.h"


static SceUID g_hooks[1] = {-1};
static vitascale_cfg_t g_config = {0};
static char g_titleid[TITLEID_SIZE+1] = {0};
static int vitascale_enabled = 1;
static tai_hook_ref_t ref_hook0;

static uint32_t current_buttons = 0, pressed_buttons = 0;

int holdButtons(SceCtrlData *pad, uint32_t buttons, uint64_t time)
{
	if ((pad->buttons & buttons) == buttons) {
		uint64_t time_start = sceKernelGetProcessTimeWide();

		while ((pad->buttons & buttons) == buttons) {
			sceCtrlPeekBufferPositive(0, pad, 1);
			pressed_buttons = pad->buttons & ~current_buttons;
			current_buttons = pad->buttons;
			if ((sceKernelGetProcessTimeWide() - time_start) >= time) {
				return 1;
			}
		}
	}
	return 0;
}

int  sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync)
{
	if (vitascale_enabled) {
		blit_set_frame_buf(pParam);
		bilt_scale_rect(g_config.sx, g_config.sy, g_config.width, g_config.height, g_config.scale);
	}
	return TAI_CONTINUE(int, ref_hook0, pParam, sync);
}

void _start() __attribute__ ((weak, alias ("module_start")));

int module_start(SceSize argc, const void *args)
{

	int ret;
	sceIoMkdir(MAIN_DIR, 0777);
	sceAppMgrAppParamGetString(0, 12, g_titleid, TITLEID_SIZE);
	ret = vs_config_load(g_titleid, &g_config);
	if (ret < 0) {
		if (ret == -2) { // no config for this game
			DBG(g_titleid, "load config failed: %d\n", ret);
		} else {
			LOG(g_titleid, "load config failed: %d\n", ret);
		}
		return SCE_KERNEL_START_SUCCESS;
	}
	DBG(g_titleid, "config: (%d, %d) width: %d height: %d scale: %f", 
		g_config.sx, g_config.sy, g_config.width, g_config.height, g_config.scale);
	g_hooks[0] = taiHookFunctionImport(&ref_hook0,
										TAI_MAIN_MODULE,
										TAI_ANY_LIBRARY,
										0x7A410B64, // sceDisplaySetFrameBuf
										sceDisplaySetFrameBuf_patched);
	DBG(g_titleid, "%s", "module init finished\n");

	return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{

	// free hooks that didn't fail
	if (g_hooks[0] >= 0) {
		taiHookRelease(g_hooks[0], ref_hook0);
		g_hooks[0] = -1;
	}
	g_titleid[0] = 0;
	g_config.scale = 0.0f;
	return SCE_KERNEL_STOP_SUCCESS;
}

#include <pebble.h>
#include "GlobalState.h"
#include "LargeImage.h"
#include "MiniAdventure.h"
#include "TextBox.h"
#include "Utils.h"

static TextBox *okTextBox = NULL;
static bool largeImageForceBacklight = false;

#define DIALOG_TEXT_X_OFFSET 2
#define DIALOG_TEXT_Y_OFFSET 2
#if defined(PBL_RECT)
static GRect okFrame = {.origin = {.x = 144 - OK_FRAME_WIDTH, .y = 168 / 2 - OK_FRAME_HEIGHT / 2}, .size = {.w = OK_FRAME_WIDTH, .h = OK_FRAME_HEIGHT}};
#elif defined(PBL_ROUND)
static GRect okFrame = {.origin = {.x = 180 - OK_FRAME_WIDTH - 5, .y = 180 / 2 - OK_FRAME_HEIGHT / 2}, .size = {.w = OK_FRAME_WIDTH, .h = OK_FRAME_HEIGHT}};
#endif

#define INTERNAL_LARGE_IMAGE_OFFSET 2

static GBitmap *largeImage = NULL;

static Layer *largeImageTopLayer = NULL;
static BitmapLayer *largeImageLayer = NULL;

static GRect largeImagePosition = {.origin = {.x = LARGE_IMAGE_LAYER_X, .y = LARGE_IMAGE_LAYER_Y}, .size = {.w = LARGE_IMAGE_LAYER_W, .h = LARGE_IMAGE_LAYER_H}};

static bool largeImageInitialized = false;

static int largeImageResourceId = -1;

void LargeImageUpdateProc(struct Layer *layer, GContext *ctx)
{
	GRect bounds = layer_get_bounds(layer);
	DrawContentFrame(ctx, &bounds, GColorBlue);
}

void InitializeLargeImageLayer(Window *window)
{
	if(!largeImageInitialized)
	{
		largeImageTopLayer = layer_create(largeImagePosition);
		
		GRect image_bounds = layer_get_bounds(largeImageTopLayer);
		image_bounds.origin.x += INTERNAL_LARGE_IMAGE_OFFSET;
		image_bounds.origin.y += INTERNAL_LARGE_IMAGE_OFFSET;
		image_bounds.size.w -= 2 * INTERNAL_LARGE_IMAGE_OFFSET;
		image_bounds.size.h -= 2 * INTERNAL_LARGE_IMAGE_OFFSET;
		
		largeImageLayer = bitmap_layer_create(image_bounds);
		largeImage = gbitmap_create_with_resource(largeImageResourceId);
		bitmap_layer_set_bitmap(largeImageLayer, largeImage);
		bitmap_layer_set_alignment(largeImageLayer, GAlignCenter);

		layer_set_update_proc(largeImageTopLayer, LargeImageUpdateProc);
		layer_add_child(largeImageTopLayer, (Layer*)largeImageLayer);

		okTextBox = CreateTextBox(DIALOG_TEXT_X_OFFSET, DIALOG_TEXT_Y_OFFSET, fonts_get_system_font(FONT_KEY_GOTHIC_14), okFrame, GTextAlignmentCenter, false, false);

		largeImageInitialized = true;
	}
	Layer *window_layer = window_get_root_layer(window);
	layer_add_child(window_layer, largeImageTopLayer);
	InitializeTextBox(window_layer, okTextBox, "OK");
}

void RemoveLargeImageLayer(void)
{
	if(!largeImageInitialized)
		return;
	
	layer_remove_from_parent(largeImageTopLayer);
	RemoveTextBox(okTextBox);
}

void CleanupLargeImageLayer(void)
{
	if(largeImageInitialized)
	{
		largeImageResourceId = -1;
		layer_destroy(largeImageTopLayer);
		bitmap_layer_destroy(largeImageLayer);
		gbitmap_destroy(largeImage);
		largeImageInitialized = false;
		FreeTextBox(okTextBox);
		okTextBox = NULL;
	}
}

void LargeImagePush(void *data)
{
	Window *window = GetBaseWindow();
	InitializeLargeImageLayer(window);
	if(largeImageForceBacklight)
		light_enable(true);
}

void LargeImagePop(void *data)
{
	RemoveLargeImageLayer();
	CleanupLargeImageLayer();
	if(largeImageForceBacklight)
		light_enable(false);
}

void TriggerLargeImage(int resourceId, bool forceBacklight)
{
	largeImageResourceId = resourceId;
	largeImageForceBacklight = forceBacklight;
	GlobalState_Push(STATE_LARGE_IMAGE, 0, NULL);
}

void QueueLargeImage(int resourceId, bool forceBacklight)
{
    largeImageResourceId = resourceId;
    largeImageForceBacklight = forceBacklight;
    GlobalState_Queue(STATE_LARGE_IMAGE, 0, NULL);
}

#pragma once

#include <obs-module.h>

enum {
	ALIGN_LEFT = 0,
	ALIGN_RIGHT = 1,
	ALIGN_CENTER = 2,
};

struct pango_source {
	/* Config */
	char *text;

	char *font_name;
	uint16_t font_size;
	uint32_t font_flags;

	int align;

	uint32_t color[2];

	bool outline;
	uint32_t outline_width;
	uint32_t outline_color;

	bool drop_shadow;
	uint32_t drop_shadow_offset;
	uint32_t drop_shadow_color;

	uint32_t custom_width;
	bool word_wrap;

	/* State */
	gs_texture_t *tex;
	uint32_t width, height;

	bool file_load_failed;
	time_t file_timestamp;
	uint64_t file_last_checked;
};

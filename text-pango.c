#include <obs-module.h>
#include <util/platform.h>
#include <sys/stat.h>
#include <pango/pangocairo.h>
#include <math.h>

#include "text-pango.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("text-pango", "en-US")

#ifdef _WIN32
#define DEFAULT_FACE "Arial"
#elif __APPLE__
#define DEFAULT_FACE "Helvetica"
#else
#define DEFAULT_FACE "Sans Serif"
#endif

#define max(a, b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

cairo_t *create_layout_context()
{
	cairo_surface_t *temp_surface;
	cairo_t *context;

	temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
	context = cairo_create(temp_surface);
	cairo_surface_destroy(temp_surface);

	return context;
}

void get_text_size(PangoLayout *layout, int *width, int *height)
{
	int w, h;

	pango_layout_get_size (layout, &w, &h);
	/* Divide by pango scale to get dimensions in pixels */
	*width = w / PANGO_SCALE;
	*height = h / PANGO_SCALE;
}

void set_font(struct pango_source *src, PangoLayout *layout) {
	PangoFontDescription *desc;

	desc = pango_font_description_new ();
	pango_font_description_set_family(desc, src->font_name);
	pango_font_description_set_size(desc, src->font_size * PANGO_SCALE);
	pango_font_description_set_weight(desc, !!(src->font_flags & OBS_FONT_BOLD) ? PANGO_WEIGHT_BOLD : 0);
	pango_font_description_set_style(desc, !!(src->font_flags & OBS_FONT_ITALIC) ? PANGO_STYLE_ITALIC : 0);
	pango_layout_set_font_description(layout, desc);
	pango_font_description_free(desc);
}

void set_alignment(struct pango_source *src, PangoLayout *layout) {
	PangoAlignment pangoAlignment;

	switch (src->align) {
	case ALIGN_LEFT:
		pangoAlignment = PANGO_ALIGN_LEFT;
		break;
	case ALIGN_RIGHT:
		pangoAlignment = PANGO_ALIGN_RIGHT;
		break;
	case ALIGN_CENTER:
		pangoAlignment = PANGO_ALIGN_CENTER;
		break;
	}
	pango_layout_set_alignment(layout, pangoAlignment);
}

cairo_t *create_cairo_context(struct pango_source *src,
	cairo_surface_t **surface, uint8_t **surface_data)
{
	*surface_data = bzalloc(src->width * src->height * 4);
	*surface = cairo_image_surface_create_for_data(*surface_data,
			CAIRO_FORMAT_ARGB32,
			src->width, src->height, 4 * src->width);

	return cairo_create(*surface);
}

#define RGBA_CAIRO(c) \
	((c & 0xff0000) >> 16) / 256.0, \
	((c & 0xff00) >> 8) / 256.0, \
	 (c & 0xff) / 256.0, \
	((c & 0xff000000) >> 24) / 256.0

void render_text(struct pango_source *src)
{
	cairo_t *layout_context;
	cairo_t *render_context;
	cairo_surface_t *surface;
	uint8_t *surface_data = NULL;
	PangoLayout *layout;
	int text_width, text_height;

	if (src->tex) {
		obs_enter_graphics();
		gs_texture_destroy(src->tex);
		obs_leave_graphics();
		src->tex = NULL;
	}

	if (!src->text)
		return;
	if (!src->font_name)
		return;

	int outline_width = src->outline ? src->outline_width : 0;
	int drop_shadow_offset = src->drop_shadow ? src->drop_shadow_offset : 0;

	layout_context = create_layout_context();

	/* Create a PangoLayout */
	layout = pango_cairo_create_layout(layout_context);

	set_font(src, layout);
	set_alignment(src, layout);

	if (src->custom_width > 0 && src->word_wrap)
		pango_layout_set_width(layout, src->custom_width * PANGO_SCALE);

	pango_layout_set_text(layout, src->text, -1);

	/* Get text dimensions and create a context to render to */
	get_text_size(layout, &text_width, &text_height);
	if (src->custom_width > 0)
		src->width = src->custom_width;
	else
		src->width = text_width;
	src->width += outline_width;
	src->width += max(outline_width, drop_shadow_offset);
	src->height = text_height;
	src->height += outline_width;
	src->height += max(outline_width, drop_shadow_offset);
	render_context = create_cairo_context(src, &surface, &surface_data);
	cairo_set_operator(render_context, CAIRO_OPERATOR_SOURCE);

	double xoffset;
	if (src->custom_width && !src->word_wrap) {
		switch (src->align) {
		case ALIGN_LEFT:
			xoffset = 0;
			break;
		case ALIGN_RIGHT:
			xoffset = src->custom_width - text_width;
			break;
		case ALIGN_CENTER:
			xoffset = (src->custom_width - text_width) / 2;
			break;
		}
	} else {
		xoffset = 0;
	}
	xoffset += outline_width;

	double yoffset;
	yoffset = 0;
	yoffset += outline_width;

	/* Render */
	pango_cairo_update_layout(render_context, layout);

	PangoLayoutIter *iter = pango_layout_get_iter(layout);
	do {
		PangoLayoutLine *line;
		PangoRectangle rect;
		int y1, y2;
		cairo_pattern_t *pattern;

		line = pango_layout_iter_get_line_readonly(iter);

		pango_layout_iter_get_line_extents(iter, NULL, &rect);
		int baseline = pango_layout_iter_get_baseline(iter);
		int xpos = xoffset + rect.x / PANGO_SCALE;
		int ypos = yoffset + baseline / PANGO_SCALE;

		if (drop_shadow_offset > 0) {
			cairo_move_to(render_context,
					xpos + drop_shadow_offset,
					ypos + drop_shadow_offset);
			pango_cairo_layout_line_path(render_context, line);
			cairo_set_source_rgba(render_context,
					RGBA_CAIRO(src->drop_shadow_color));
			cairo_fill(render_context);
		}

		cairo_move_to(render_context, xpos, ypos);
		pango_cairo_layout_line_path(render_context, line);

		if (outline_width > 0) {
			cairo_set_line_width(render_context, outline_width * 2);
			cairo_set_source_rgba(render_context,
					RGBA_CAIRO(src->outline_color));
			cairo_stroke_preserve(render_context);
		}

		pango_layout_iter_get_line_yrange(iter, &y1, &y2);
		pattern = cairo_pattern_create_linear(
				0, y1 / PANGO_SCALE + yoffset,
				0, y2 / PANGO_SCALE + yoffset);
		cairo_pattern_set_extend(pattern, CAIRO_EXTEND_NONE);
		cairo_pattern_add_color_stop_rgba(pattern, 0.0,
				RGBA_CAIRO(src->color[0]));
		cairo_pattern_add_color_stop_rgba(pattern, 1.0,
				RGBA_CAIRO(src->color[1]));

		cairo_set_source(render_context, pattern);
		cairo_fill(render_context);

		cairo_pattern_destroy(pattern);
	} while (pango_layout_iter_next_line(iter));
	pango_layout_iter_free(iter);

	obs_enter_graphics();
	src->tex = gs_texture_create(src->width, src->height, GS_RGBA, 1,
			(const uint8_t **) &surface_data, 0);
	obs_leave_graphics();

	/* Clean up */
	bfree(surface_data);
	g_object_unref(layout);
	cairo_destroy(layout_context);
	cairo_destroy(render_context);
	cairo_surface_destroy(surface);
}

static const char *pango_source_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);

	return obs_module_text("TextPango");
}

static uint32_t pango_source_get_width(void *data)
{
	struct pango_source *src = data;

	return src->width;
}

static uint32_t pango_source_get_height(void *data)
{
	struct pango_source *src = data;

	return src->height;
}

static void pango_source_get_defaults(obs_data_t *settings)
{
	obs_data_t *font;

	font = obs_data_create();
	obs_data_set_default_string(font, "face", DEFAULT_FACE);
	obs_data_set_default_int(font, "size", 32);
	obs_data_set_default_obj(settings, "font", font);
	obs_data_release(font);

	obs_data_set_default_int(settings, "color1", 0xFFFFFFFF);
	obs_data_set_default_int(settings, "color2", 0xFFFFFFFF);

	obs_data_set_default_int(settings, "outline_width", 2);
	obs_data_set_default_int(settings, "outline_color", 0xFF000000);

	obs_data_set_default_int(settings, "drop_shadow_offset", 4);
	obs_data_set_default_int(settings, "drop_shadow_color", 0xFF000000);
}

static bool pango_source_properties_outline_changed(obs_properties_t *props,
		obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	obs_property_t *prop;

	bool enabled = obs_data_get_bool(settings, "outline");

	prop = obs_properties_get(props, "outline_width");
	obs_property_set_visible(prop, enabled);
	prop = obs_properties_get(props, "outline_color");
	obs_property_set_visible(prop, enabled);

	return true;
}

static bool pango_source_properties_drop_shadow_changed(obs_properties_t *props,
		obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	obs_property_t *prop;

	bool enabled = obs_data_get_bool(settings, "drop_shadow");

	prop = obs_properties_get(props, "drop_shadow_offset");
	obs_property_set_visible(prop, enabled);
	prop = obs_properties_get(props, "drop_shadow_color");
	obs_property_set_visible(prop, enabled);

	return true;
}

static obs_properties_t *pango_source_get_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	obs_properties_t *props;
	obs_property_t *prop;

	props = obs_properties_create();

	obs_properties_add_text(props, "text",
		obs_module_text("Text"), OBS_TEXT_MULTILINE);

	obs_properties_add_font(props, "font",
		obs_module_text("Font"));

	prop = obs_properties_add_list(props, "align",
		obs_module_text("Alignment"), OBS_COMBO_TYPE_LIST,
		OBS_COMBO_FORMAT_INT);
	obs_property_list_add_int(prop,
		obs_module_text("AlignLeft"), ALIGN_LEFT);
	obs_property_list_add_int(prop,
		obs_module_text("AlignRight"), ALIGN_RIGHT);
	obs_property_list_add_int(prop,
		obs_module_text("AlignCenter"), ALIGN_CENTER);

	obs_properties_add_color(props, "color1",
		obs_module_text("ColorTop"));
	obs_properties_add_color(props, "color2",
		obs_module_text("ColorBottom"));

	prop = obs_properties_add_bool(props, "outline",
		obs_module_text("Outline"));
	obs_property_set_modified_callback(prop,
		pango_source_properties_outline_changed);
	prop = obs_properties_add_int(props, "outline_width",
		obs_module_text("OutlineWidth"), 1, 256, 1);
	obs_property_set_visible(prop, false);
	prop = obs_properties_add_color(props, "outline_color",
		obs_module_text("OutlineColor"));
	obs_property_set_visible(prop, false);

	prop = obs_properties_add_bool(props, "drop_shadow",
		obs_module_text("DropShadow"));
	obs_property_set_modified_callback(prop,
		pango_source_properties_drop_shadow_changed);
	prop = obs_properties_add_int(props, "drop_shadow_offset",
		obs_module_text("DropShadowOffset"), 1, 256, 1);
	obs_property_set_visible(prop, false);
	prop = obs_properties_add_color(props, "drop_shadow_color",
		obs_module_text("DropShadowColor"));
	obs_property_set_visible(prop, false);

	obs_properties_add_int(props, "custom_width",
		obs_module_text("CustomWidth"), 0, 4096, 1);
	obs_properties_add_bool(props, "word_wrap",
		obs_module_text("WordWrap"));

	return props;
}

static void pango_source_destroy(void *data)
{
	struct pango_source *src = data;

	if (src->text != NULL)
		bfree(src->text);

	if (src->font_name != NULL)
		bfree(src->font_name);

	obs_enter_graphics();

	if (src->tex != NULL) {
		gs_texture_destroy(src->tex);
		src->tex = NULL;
	}

	obs_leave_graphics();

	bfree(src);
}

static void pango_source_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	struct pango_source *src = data;

	if (src->tex == NULL)
		return;

	gs_reset_blend_state();
	gs_effect_set_texture(gs_effect_get_param_by_name(effect, "image"),
			src->tex);
	gs_draw_sprite(src->tex, 0,
			src->width, src->height);
}

static void pango_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(seconds);
}

static void pango_source_update(void *data, obs_data_t *settings)
{
	struct pango_source *src = data;
	obs_data_t *font;

	if (src->text)
		bfree(src->text);
	src->text = bstrdup(obs_data_get_string(settings, "text"));

	font = obs_data_get_obj(settings, "font");
	if (src->font_name)
		bfree(src->font_name);
	src->font_name  = bstrdup(obs_data_get_string(font, "face"));
	src->font_size   = (uint16_t)obs_data_get_int(font, "size");
	src->font_flags  = (uint32_t)obs_data_get_int(font, "flags");
	obs_data_release(font);

	src->align = obs_data_get_int(settings, "align");

	src->color[0] = (uint32_t)obs_data_get_int(settings, "color1");
	src->color[1] = (uint32_t)obs_data_get_int(settings, "color2");

	src->outline = obs_data_get_bool(settings, "outline");
	src->outline_width = obs_data_get_int(settings, "outline_width");
	src->outline_color = obs_data_get_int(settings, "outline_color");

	src->drop_shadow = obs_data_get_bool(settings, "drop_shadow");
	src->drop_shadow_offset = obs_data_get_int(settings, "drop_shadow_offset");
	src->drop_shadow_color = obs_data_get_int(settings, "drop_shadow_color");

	src->custom_width = (uint32_t)obs_data_get_int(settings, "custom_width");
	src->word_wrap = obs_data_get_bool(settings, "word_wrap");

	src->file_timestamp = 0;
	src->file_last_checked = 0;

	render_text(src);
}

static void *pango_source_create(obs_data_t *settings, obs_source_t *source)
{
	UNUSED_PARAMETER(source);
	struct pango_source *src = bzalloc(sizeof(struct pango_source));

	pango_source_update(src, settings);

	return src;
}

static struct obs_source_info pango_source_info = {
	.id = "text_pango_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = pango_source_get_name,
	.create = pango_source_create,
	.destroy = pango_source_destroy,
	.update = pango_source_update,
	.get_width = pango_source_get_width,
	.get_height = pango_source_get_height,
	.video_render = pango_source_render,
	.video_tick = pango_video_tick,
	.get_defaults = pango_source_get_defaults,
	.get_properties = pango_source_get_properties,
};

bool obs_module_load()
{
	obs_register_source(&pango_source_info);

	return true;
}

void obs_module_unload(void)
{
}

#include "text-pango.h"

time_t get_modified_timestamp(const char *filename);

void load_text_from_file(struct pango_source *srcdata, const char *filename);

void read_from_end(struct pango_source *src, const char *filename);

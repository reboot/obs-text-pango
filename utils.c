#include <stdio.h>

#include <sys/stat.h>

#include <util/platform.h>

#include "text-pango.h"

time_t get_modified_timestamp(const char *filename)
{
	struct stat stats;
	if (os_stat(filename, &stats) != 0)
		return -1;
	return stats.st_mtime;
}

void load_text_from_file(struct pango_source *src, const char *filename)
{
	FILE *file = NULL;
	uint32_t filesize = 0;
	uint8_t *tmp_read = NULL;
	uint16_t header = 0;
	size_t bytes_read;
	bool utf16 = false;

	src->file_timestamp = get_modified_timestamp(src->text_file);

	file = os_fopen(filename, "rb");
	if (file == NULL) {
		if (!src->file_load_failed) {
			blog(LOG_WARNING, "Failed to open file %s", filename);
			src->file_load_failed = true;
		}
		return;
	}
	fseek(file, 0, SEEK_END);
	filesize = (uint32_t)ftell(file);
	fseek(file, 0, SEEK_SET);
	bytes_read = fread(&header, 1, 2, file);

	if (bytes_read == 2 && header == 0xFEFF)
		utf16 = true;
	else
		fseek(file, 0, SEEK_SET);

	tmp_read = bzalloc(filesize + 1);
	bytes_read = 0;
	while (bytes_read < filesize) {
		int b = fread(tmp_read + bytes_read, 1, filesize - bytes_read, file);
		if (b <= 0)
			break;
		bytes_read += b;
	}
	fclose(file);

	if (src->text != NULL) {
		bfree(src->text);
		src->text = NULL;
	}
	if (utf16) {
		return;
		/* No conversion function for now
		src->text = bzalloc(bytes_read * 2);
		os_wcs_to_utf8((wchar_t *) tmp_read, bytes_read, src->text, bytes_read * 2);
		*/
	} else {
		src->text = bstrdup((char *) tmp_read);
	}

	bfree(tmp_read);
}

void read_from_end(struct pango_source *src, const char *filename)
{
	FILE *tmp_file = NULL;
	uint32_t filesize = 0, cur_pos = 0;
	char *tmp_read = NULL;
	uint16_t value = 0, line_breaks = 0;
	size_t bytes_read;
	char bvalue;

	bool utf16 = false;

	tmp_file = fopen(filename, "rb");
	if (tmp_file == NULL) {
		if (!src->file_load_failed) {
			blog(LOG_WARNING, "Failed to open file %s", filename);
			src->file_load_failed = true;
		}
		return;
	}
	bytes_read = fread(&value, 2, 1, tmp_file);

	if (bytes_read == 2 && value == 0xFEFF)
		utf16 = true;

	fseek(tmp_file, 0, SEEK_END);
	filesize = (uint32_t)ftell(tmp_file);
	cur_pos = filesize;

	while (line_breaks <= 6 && cur_pos != 0) {
		if (!utf16) cur_pos--;
		else cur_pos -= 2;
		fseek(tmp_file, cur_pos, SEEK_SET);

		if (!utf16) {
			bytes_read = fread(&bvalue, 1, 1, tmp_file);
			if (bytes_read == 1 && bvalue == '\n')
				line_breaks++;
		}
		else {
			bytes_read = fread(&value, 2, 1, tmp_file);
			if (bytes_read == 2 && value == L'\n')
				line_breaks++;
		}
	}

	if (cur_pos != 0)
		cur_pos += (utf16) ? 2 : 1;

	fseek(tmp_file, cur_pos, SEEK_SET);

	if (utf16) {
		if (src->text != NULL) {
			bfree(src->text);
			src->text = NULL;
		}
		src->text = bzalloc(filesize - cur_pos);
		bytes_read = fread(src->text, (filesize - cur_pos), 1,
				tmp_file);

		src->file_timestamp = get_modified_timestamp(src->text_file);
		bfree(tmp_read);
		fclose(tmp_file);

		return;
	}

	tmp_read = bzalloc((filesize - cur_pos) + 1);
	bytes_read = fread(tmp_read, filesize - cur_pos, 1, tmp_file);
	fclose(tmp_file);

	if (src->text != NULL) {
		bfree(src->text);
		src->text = NULL;
	}
	src->text = bstrdup(tmp_read);

	src->file_timestamp = get_modified_timestamp(src->text_file);
	bfree(tmp_read);
}

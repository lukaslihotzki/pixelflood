#include "text.hpp"
#include <ft2build.h>
#include <freetype/freetype.h>
#include <stdexcept>
#include <memory>

std::string exec(const char* cmd)
{
	char buffer[128];
	std::string result = "";
	std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
	if (!pipe) throw std::runtime_error("popen() failed!");
	while (!feof(pipe.get())) {
		if (fgets(buffer, 128, pipe.get()) != NULL)
			result += buffer;
	}
	return result;
}

void writeText(const Canvas& canvas, int px, int py, std::string text)
{
	FT_Library library;
	if (FT_Init_FreeType(&library)) {
		throw std::runtime_error("FT_Init_FreeType");
	}

	FT_Face face;
	if (FT_New_Face(library, "/usr/share/fonts/TTF/DejaVuSans.ttf", 0, &face)) {
		throw std::runtime_error("FT_New_Face");
	}

	if (FT_Set_Char_Size(face, 30 * 64, 0, 50, 0)) {
		throw std::runtime_error("FT_Set_Char_Size");
	}

	FT_GlyphSlot slot = face->glyph;

	FT_Vector pen;
	pen.x = 0;
	pen.y = face->size->metrics.height;

	for (char c : text) {
		if (c == '\n') {
			pen.x = 0;
			pen.y += face->size->metrics.height;
			continue;
		}
		int error = FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_FORCE_AUTOHINT);
		if (error)
			continue;

		for (int i = 0; i < slot->bitmap.rows; i++) {
			for (int j = 0; j < slot->bitmap.width; j++) {
				unsigned char c = slot->bitmap.buffer[slot->bitmap.pitch * i + j];
				canvas.data[canvas.width * (i + pen.y / 64 - slot->bitmap_top) + j + slot->bitmap_left + pen.x / 64] = c << 24 | c << 16 | c << 8;
			}
		}

		/* increment pen position */
		pen.x += slot->advance.x;
		pen.y += slot->advance.y;
	}

	FT_Done_Face(face);
	FT_Done_FreeType(library);
}

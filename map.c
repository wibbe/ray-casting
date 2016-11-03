
#define MAX_VERTICES 1024
#define MAX_SECTORS 512
#define MAX_WALLS (MAX_SECTORS * 8)


typedef struct {
   int wall_start;
   int wall_count;
} Sector;

typedef struct {
   Vec2 pos;
   int next_sector;
   int color;
} Wall;

typedef struct {
	u32 wall_count;
	u32 sector_count;

	Sector sectors[MAX_SECTORS];
	Wall walls[MAX_WALLS];
} Map;



typedef struct {
	char identifier[4];
	i32 lump_count;
	i32 directory_offset;
} WadHeader;

typedef struct {
	i32 file_pos;
	i32 file_len;
	char file_name[8];
} WadDirectoryEntry;

typedef struct {
	i16 x;
	i16 y;
} WadVertex;

typedef struct {
	i16 start_vertex;
	i16 end_vertex;
	i16 flags;
	i16 special_type;
	i16 sector_tag;
	i16 front_side_def;
	i16 back_side_def;
} WadLineDef;

typedef struct {
	i16 x_tex_offset;
	i16 y_tex_offset;
	char upper_tex[8];
	char lower_tex[8];
	char middle_tex[8];
	i16 sector;
} WadSideDef;

typedef struct {
	i16 floor_height;
	i16 ceiling_height;
	char floor_tex[8];
	char ceiling_tex[8];
	i16 light_level;
	i16 type;
	i16 tag;
} WadSector;


bool map_load_wad(const char * filename, Map * map)
{
	size_t data_size = 0;
	unsigned char * data = resource_get(filename, &data_size);
	if (!data)
		return false;

	memset(map, 0, sizeof(Map));

	unsigned char * it = data;

	printf("Loading map: %s size: %zd\n", filename, data_size);

	// Read header
	WadHeader header;
	memcpy(&header, it, sizeof(header));
	it += sizeof(header);

	printf("Identifier: %c, %c, %c, %c\n", header.identifier[0], header.identifier[1], header.identifier[2], header.identifier[3]);
	printf("Lump count: %d\n", header.lump_count);
	printf("Directory offset: %d\n", header.directory_offset);

	size_t vertex_count = 0;
	WadVertex * vertices = NULL;

	size_t line_count = 0;
	WadLineDef * lines = NULL;

	size_t side_count = 0;
	WadSideDef * sides = NULL;

	size_t sector_count = 0;
	WadSector * sectors = NULL;

	// Read all the lumps
	unsigned char * dir_it = (unsigned char *)data + header.directory_offset;
	for (int i = 0; i < header.lump_count; ++i)
	{
		WadDirectoryEntry entry;

		memcpy(&entry, dir_it, sizeof(WadDirectoryEntry));
		dir_it += sizeof(WadDirectoryEntry);

		char str[9];
		str[8] = '\0';
		memcpy(str, entry.file_name, 8);

		printf("Dir: %s pos: %d len: %d\n", str, entry.file_pos, entry.file_len);

		if (strcmp(str, "VERTEXES") == 0)
		{
			if (entry.file_len % sizeof(WadVertex) != 0)
			{
				fprintf(stderr, "  Wrong number of vertices in this lump\n");
				continue;
			}

			vertex_count = entry.file_len / sizeof(WadVertex);
			vertices = (WadVertex *)(data + entry.file_pos);

			printf("  Got %d vertices\n", vertex_count);
		}
		else if (strcmp(str, "LINEDEFS") == 0)
		{
			if (entry.file_len % sizeof(WadLineDef) != 0)
			{
				fprintf(stderr, "  Wrong number of line-defs in this lump\n");
				continue;
			}

			line_count = entry.file_len / sizeof(WadLineDef);
			lines = (WadLineDef *)(data + entry.file_pos);

			printf("  Got %d line-defs\n", line_count);
		}
		else if (strcmp(str, "SIDEDEFS") == 0)
		{
			if (entry.file_len % sizeof(WadSideDef) != 0)
			{
				fprintf(stderr, "  Wrong number of side-defs in this lump\n");
				continue;
			}

			side_count = entry.file_len / sizeof(WadSideDef);
			sides = (WadSideDef *)(data + entry.file_pos);

			printf("  Got %d side-defs\n", side_count);
		}
		else if (strcmp(str, "SECTORS") == 0)
		{
			if (entry.file_len % sizeof(WadSector) != 0)
			{
				fprintf(stderr, "  Wrong number of sectors in this lump\n");
				continue;
			}

			sector_count = entry.file_len / sizeof(WadSector);
			sectors = (WadSector *)(data + entry.file_pos);

			printf("  Got %d sectors\n", sector_count);
		}
	}

	// Print information about the map

	printf("\nVERTICES\n");
	for (int i = 0; i < vertex_count; ++i)
		printf("  [%2d] = %d, %d\n", i, vertices[i].x, vertices[i].y);

	printf("\nLINEDEFS\n");
	for (int i = 0; i < line_count; ++i)
		printf("  [%2d] = %d -> %d sec: %d front: %d back: %d\n", i, lines[i].start_vertex, lines[i].end_vertex, lines[i].sector_tag, lines[i].front_side_def, lines[i].back_side_def);

	printf("\nSIDEDEFS\n");
	for (int i = 0; i < side_count; ++i)
	{
		char upper[9];
		char lower[9];
		char middle[9];
		memcpy(upper, sides[i].upper_tex, 8);
		memcpy(lower, sides[i].lower_tex, 8);
		memcpy(middle, sides[i].middle_tex, 8);
		upper[8] = '\0';
		lower[8] = '\0';
		middle[8] = '\0';

		printf("  [%2d] = sector: %d, x-offset: %d y-offset: %d upper-tex: %s lower-tex: %s middle-tex: %s\n", i, sides[i].sector, sides[i].x_tex_offset, sides[i].y_tex_offset, upper, lower, middle);
	}

	printf("\nSECTORS\n");
	for (int i = 0; i < sector_count; ++i)
	{
		char floor[9];
		char ceil[9];
		memcpy(floor, sectors[i].floor_tex, 8);
		memcpy(ceil, sectors[i].ceiling_tex, 8);
		floor[8] = '\0';
		ceil[8] = '\0';

		printf("  [%2d] = floor-height: %d ceiling-height: %d floor-tex: %s ceiling-tex: %s\n", i, sectors[i].floor_height, sectors[i].ceiling_height, floor, ceil);
	}


	int wall_start = 0;
	int wall_it = 0;

	map->sector_count = sector_count;
	for (int sec_it = 0; sec_it < sector_count; ++sec_it)
	{
		Sector * map_sector = map->sectors[sec_it];

		// Find the first line that belongs to this sector
		for (int line_it = 0; line_it < line_count; ++line_it)
		{
			int side_def = lines[line_it].front_side_def;
			if (side_def == -1)
				continue;

			if (sides[side_def].sector == sec_it)
		}

		map->sector_count++;
	}



	return true;
}
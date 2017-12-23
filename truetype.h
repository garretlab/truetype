#ifndef TRUETYPE_H
#define TRUETYPE_H
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define FLAG_ONCURVE (1 << 0)
#define FLAG_XSHORT (1 << 1)
#define FLAG_YSHORT (1 << 2)
#define FLAG_REPEAT (1 << 3)
#define FLAG_XSAME (1 << 4)
#define FLAG_YSAME (1 << 5)

typedef struct {
  char name[5];
  uint32_t checkSum;
  uint32_t offset;
  uint32_t length;
} ttTable_t;

typedef struct {
  uint32_t version;
  uint32_t revision;
  uint32_t checkSumAdjustment;
  uint32_t magicNumber;
  uint16_t flags;
  uint16_t unitsPerEm;
  char     created[8];
  char     modified[8];
  int16_t  xMin;
  int16_t  yMin;
  int16_t  xMax;
  int16_t  yMax;
  uint16_t macStyle;
  uint16_t lowestRecPPEM;
  int16_t fontDirectionHint;
  int16_t indexToLocFormat;
  int16_t glyphDataFormat;
} ttHeadttTable_t;

typedef struct {
  uint8_t flag;
  int16_t x;
  int16_t y;
} ttPoint_t;

typedef struct {
  int16_t numberOfContours;
  int16_t xMin;
  int16_t yMin;
  int16_t xMax;
  int16_t yMax;
  uint16_t *endPtsOfContours;
  uint16_t numberOfPoints;
  ttPoint_t *points;
} ttGlyph_t;

typedef struct {
  uint16_t version;
  uint16_t numberSubtables;
} ttCmapIndex_t;

typedef struct {
  uint16_t platformId;
  uint16_t platformSpecificId;
  uint16_t offset;
} ttCmapEncoding_t;

typedef struct {
  uint16_t format;
  uint16_t length;
  uint16_t language;
  uint16_t segCountX2;
  uint16_t searchRange;
  uint16_t entrySelector;
  uint16_t rangeShift;
  uint32_t offset;
  uint32_t endCodeOffset;
  uint32_t startCodeOffset;
  uint32_t idDeltaOffset;
  uint32_t idRangeOffsetOffset;
  uint32_t glyphIndexArrayOffset;
} ttCmapFormat4_t;

class truetypeClass {
  public:
    truetypeClass(SDFS *sd);
    int begin(int cs, const char * path, int checkCheckSum = 0);
    void end();
    int readGlyph(uint16_t code, ttGlyph_t *glyph);
    void adjustGlyph(ttGlyph_t *glyph);
    static void freeGlyph(ttGlyph_t *glyph);
    int xMin, xMax, yMin, yMax;
  private:
    static SDFS *sd;

    const int numTablesPos = 4;
    const int tablePos = 12;

    uint16_t numTables;
    ttTable_t *table;
    File file;
    ttHeadttTable_t headTable;
    ttCmapIndex_t cmapIndex;
    ttCmapEncoding_t *cmapEncoding;
    ttCmapFormat4_t cmapFormat4;

    uint8_t getUInt8t();
    int16_t getInt16t();
    uint16_t getUInt16t();
    uint32_t getUInt32t();
    uint32_t calculateCheckSum(uint32_t offset, uint32_t length);
    uint32_t seekToTable(const char *name);
    int readTableDirectory(int checkCheckSum);
    void readHeadTable();
    uint32_t getGlyphOffset(uint16_t index);
    int readCmapFormat4();
    int readCmap();
    void readCoords(ttGlyph_t *glyph, char xy);
    void insertGlyph(ttGlyph_t *glyph, int contour, int position, int16_t x, int16_t y, uint8_t flag);
    uint16_t codeToGlyphId(uint16_t code);
    int readSimpleGlyph(ttGlyph_t *glyph);
};

#endif /* TRUETYPE_H */

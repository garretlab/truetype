#include "truetype.h"

SDFS *truetypeClass::sd;

/* constructor */
truetypeClass::truetypeClass(SDFS *sd) {
  this->sd = sd;
}

/* get uint8_t at the current position */
uint8_t truetypeClass::getUInt8t() {
  uint8_t x;

  file.read(&x, 1);
  return x;
}

/* get int16_t at the current position */
int16_t truetypeClass::getInt16t() {
  byte x[2];

  file.read(x, 2);
  return (x[0] << 8) | x[1];
}

/* get uint16_t at the current position */
uint16_t truetypeClass::getUInt16t() {
  byte x[2];

  file.read(x, 2);
  return (x[0] << 8) | x[1];
}

/* get uint32_t at the current position */
uint32_t truetypeClass::getUInt32t() {
  byte x[4];

  file.read(x, 4);
  return (x[0] << 24) | (x[1] << 16) | (x[2] << 8) | x[3];
}

/* calculate checksum */
uint32_t truetypeClass::calculateCheckSum(uint32_t offset, uint32_t length) {
  uint32_t checksum = 0L;

  length = (length + 3) / 4;
  file.seek(offset);

  while (length-- > 0) {
    checksum += getUInt32t();
  }
  return checksum;
}

/* seek to the first position of the specified table name */
uint32_t truetypeClass::seekToTable(const char *name) {
  for (int i = 0; i < numTables; i++) {
    if (strcmp(table[i].name, name) == 0) {
      file.seek(table[i].offset);
      return table[i].offset;
    }
  }
  return 0;
}

/* read table directory */
int truetypeClass::readTableDirectory(int checkCheckSum) {
  file.seek(numTablesPos);
  numTables = getUInt16t();
  table = (ttTable_t *)malloc(sizeof(ttTable_t) * numTables);

  file.seek(tablePos);
  for (int i = 0; i < numTables; i++) {
    for (int j = 0; j < 4; j++) {
      table[i].name[j] = getUInt8t();
    }
    table[i].name[4] = '\0';
    table[i].checkSum = getUInt32t();
    table[i].offset = getUInt32t();
    table[i].length = getUInt32t();
  }

  if (checkCheckSum) {
    for (int i = 0; i < numTables; i++) {
      if (strcmp(table[i].name, "head") != 0) { /* checksum of "head" is invalid */
        uint32_t c = calculateCheckSum(table[i].offset, table[i].length);
        if (table[i].checkSum != c) {
          return 0;
        }
      }
    }
  }
  return 1;
}

/* read head table */
void truetypeClass::readHeadTable() {
  for (int i = 0; i < numTables; i++) {
    if (strcmp(table[i].name, "head") == 0) {
      file.seek(table[i].offset);

      headTable.version = getUInt32t();
      headTable.revision = getUInt32t();
      headTable.checkSumAdjustment = getUInt32t();
      headTable.magicNumber = getUInt32t();
      headTable.flags = getUInt16t();
      headTable.unitsPerEm = getUInt16t();
      for (int j = 0; j < 8; j++) {
        headTable.created[i] = getUInt8t();
      }
      for (int j = 0; j < 8; j++) {
        headTable.modified[i] = getUInt8t();
      }
      xMin = headTable.xMin = getInt16t();
      yMin = headTable.yMin = getInt16t();
      xMax = headTable.xMax = getInt16t();
      yMax = headTable.yMax = getInt16t();
      headTable.macStyle = getUInt16t();
      headTable.lowestRecPPEM = getUInt16t();
      headTable.fontDirectionHint = getInt16t();
      headTable.indexToLocFormat = getInt16t();
      headTable.glyphDataFormat = getInt16t();
    }
  }
}

/* get glyph offset */
uint32_t truetypeClass::getGlyphOffset(uint16_t index) {
  uint32_t offset = 0;

  for (int i = 0; i < numTables; i++) {
    if (strcmp(table[i].name, "loca") == 0) {
      if (headTable.indexToLocFormat == 1) {
        file.seek(table[i].offset + index * 4);
        offset = getUInt32t();
      } else {
        file.seek(table[i].offset + index * 2);
        offset = getUInt16t() * 2;
      }
      break;
    }
  }

  for (int i = 0; i < numTables; i++) {
    if (strcmp(table[i].name, "glyf") == 0) {
      return (offset + table[i].offset);
    }
  }

  return 0;
}

/* read cmap format 4 */
int truetypeClass::readCmapFormat4() {
  file.seek(cmapFormat4.offset);
  if ((cmapFormat4.format = getUInt16t()) != 4) {
    return 0;
  }

  cmapFormat4.length = getUInt16t();
  cmapFormat4.language = getUInt16t();
  cmapFormat4.segCountX2 = getUInt16t();
  cmapFormat4.searchRange = getUInt16t();
  cmapFormat4.entrySelector = getUInt16t();
  cmapFormat4.rangeShift = getUInt16t();
  cmapFormat4.endCodeOffset = cmapFormat4.offset + 14;
  cmapFormat4.startCodeOffset = cmapFormat4.endCodeOffset + cmapFormat4.segCountX2 + 2;
  cmapFormat4.idDeltaOffset = cmapFormat4.startCodeOffset + cmapFormat4.segCountX2;
  cmapFormat4.idRangeOffsetOffset = cmapFormat4.idDeltaOffset + cmapFormat4.segCountX2;
  cmapFormat4.glyphIndexArrayOffset = cmapFormat4.idRangeOffsetOffset + cmapFormat4.segCountX2;

  return 1;
}

/* read cmap */
int truetypeClass::readCmap() {
  uint16_t platformId, platformSpecificId;
  uint32_t cmapOffset, tableOffset;
  int foundMap = 0;

  if ((cmapOffset = seekToTable("cmap")) == 0) {
    return 0;
  }

  cmapIndex.version = getUInt16t();
  cmapIndex.numberSubtables = getUInt16t();

  for (int i = 0; i < cmapIndex.numberSubtables; i++) {
    platformId = getUInt16t();
    platformSpecificId = getUInt16t();
    tableOffset = getUInt32t();
    if ((platformId == 3) && (platformSpecificId == 1)) {
      cmapFormat4.offset = cmapOffset + tableOffset;
      readCmapFormat4();
      foundMap = 1;
      break;
    }
  }

  if (foundMap == 0) {
    return 0;
  }

  return 1;
}

/* convert character code to glyph id */
uint16_t truetypeClass::codeToGlyphId(uint16_t code) {
  uint16_t start, end, idRangeOffset;
  int16_t idDelta;
  int found = 0;
  uint16_t offset, glyphId;

  for (int i = 0; i < cmapFormat4.segCountX2 / 2; i++) {
    file.seek(cmapFormat4.endCodeOffset + 2 * i);
    end = getUInt16t();
    if (code <= end) {
      file.seek(cmapFormat4.startCodeOffset + 2 * i);
      start = getUInt16t();
      if (code >= start) {
        file.seek(cmapFormat4.idDeltaOffset + 2 * i);
        idDelta = getInt16t();
        file.seek(cmapFormat4.idRangeOffsetOffset + 2 * i);
        idRangeOffset = getUInt16t();
        if (idRangeOffset == 0) {
          glyphId = (idDelta + code) % 65536;
        } else {
          offset = (idRangeOffset / 2 + i + code - start - cmapFormat4.segCountX2 / 2) * 2;
          file.seek(cmapFormat4.glyphIndexArrayOffset + offset);
          glyphId = getUInt16t();
        }

        found = 1;
        break;
      }
    }
  }
  if (!found) {
    return 0;
  }
  return glyphId;
}

/* initialize */
int truetypeClass::begin(int cs, const char * path, int checkCheckSum) {
  if (!sd->begin(cs)) {
    return 0;
  }

  if ((file = sd->open(path)) == 0) {
    return 0;
  }

  if (readTableDirectory(checkCheckSum) == 0) {
    file.close();
    return 0;
  }

  if (readCmap() == 0) {
    file.close();
    return 0;
  }

  readHeadTable();

  return 1;
}

void truetypeClass::end() {
  file.close();
}

/* read coords */
void truetypeClass::readCoords(ttGlyph_t *glyph, char xy) {
  int value = 0;
  int shortFlag, sameFlag;

  if (xy == 'x') {
    shortFlag = FLAG_XSHORT;
    sameFlag = FLAG_XSAME;
  } else {
    shortFlag = FLAG_YSHORT;
    sameFlag = FLAG_YSAME;
  }

  for (int i = 0; i < glyph->numberOfPoints; i++) {
    if (glyph->points[i].flag & shortFlag) {
      if (glyph->points[i].flag & sameFlag) {
        value += getUInt8t();
      } else {
        value -= getUInt8t();
      }
    } else if (~glyph->points[i].flag & sameFlag) {
      value += getUInt16t();
    }

    if (xy == 'x') {
      glyph->points[i].x = value;
    } else {
      glyph->points[i].y = value;
    }
  }
}

/* insert a point at the specified position */
void truetypeClass::insertGlyph(ttGlyph_t *glyph, int contour, int position, int16_t x, int16_t y, uint8_t flag) {
  glyph->numberOfPoints++;
  glyph->points = (ttPoint_t *)realloc(glyph->points, sizeof(ttPoint_t) * glyph->numberOfPoints);

  for (int k = glyph->numberOfPoints - 1; k >= position; k--) {
    glyph->points[k].x = glyph->points[k - 1].x;
    glyph->points[k].y = glyph->points[k - 1].y;
    glyph->points[k].flag = glyph->points[k - 1].flag;
  }
  glyph->points[position].flag = flag;
  glyph->points[position].x = x;
  glyph->points[position].y = y;

  for (int k = contour; k < glyph->numberOfContours; k++) {
    glyph->endPtsOfContours[k]++;
  }
}

/* treat consecutive off curves */
void truetypeClass::adjustGlyph(ttGlyph_t *glyph) {
  /* insert each start point at the end of the contour to close the figure */
  for (int i = 0, j = 0; i < glyph->numberOfContours; i++) {
    insertGlyph(glyph, i, glyph->endPtsOfContours[i] + 1, glyph->points[j].x, glyph->points[j].y, glyph->points[j].flag);
    j = glyph->endPtsOfContours[i] + 1;
  }

  for (int i = 0, j = 0; i < glyph->numberOfContours; i++, j += 2) {
    for (; j < glyph->endPtsOfContours[i] - 1; j++) {
      if (((glyph->points[j].flag & FLAG_ONCURVE) == 0) && ((glyph->points[j + 1].flag & FLAG_ONCURVE) == 0)) {
        insertGlyph(glyph, j + 1, i, (glyph->points[j].x + glyph->points[j + 1].x) / 2, (glyph->points[j].y + glyph->points[j + 1].y) / 2, FLAG_ONCURVE);
      }
    }
  }

  /* insert each start + 1 point at the end of the contour if the start is OFF curve */
  for (int i = 0, j = 0; i < glyph->numberOfContours; i++) {
    if ((glyph->points[j].flag & FLAG_ONCURVE) == 0) {
      insertGlyph(glyph, i, glyph->endPtsOfContours[i] + 1, glyph->points[j + 1].x, glyph->points[j + 1].y, glyph->points[j + 1].flag);
      j = glyph->endPtsOfContours[i] + 1;
    }
  }

}

/* read simple glyph */
int truetypeClass::readSimpleGlyph(ttGlyph_t *glyph) {
  int repeatCount;
  uint8_t flag;

  glyph->endPtsOfContours = (uint16_t *)malloc((sizeof(uint16_t) * glyph->numberOfContours));

  for (int i = 0; i < glyph->numberOfContours; i++) {
    glyph->endPtsOfContours[i] = getUInt16t();
  }
  file.seek(getUInt16t(), SeekCur);

  if (glyph->numberOfContours == 0) {
    return 0;
  }

  glyph->numberOfPoints = 0;
  for (int i = 0; i < glyph->numberOfContours; i++) {
    if (glyph->endPtsOfContours[i] > glyph->numberOfPoints) {
      glyph->numberOfPoints = glyph->endPtsOfContours[i];
    }
  }
  glyph->numberOfPoints++;

  glyph->points = (ttPoint_t *)malloc(sizeof(ttPoint_t) * (glyph->numberOfPoints + glyph->numberOfContours));

  for (int i = 0; i < glyph->numberOfPoints; i++) {
    flag = getUInt8t();
    glyph->points[i].flag = flag;
    if (flag & FLAG_REPEAT) {
      repeatCount = getUInt8t();
      while (repeatCount--) {
        glyph->points[++i].flag = flag;
      }
    }
  }

  readCoords(glyph, 'x');
  readCoords(glyph, 'y');

  return 1;
}

/* read glyph */
int truetypeClass::readGlyph(uint16_t code, ttGlyph_t *glyph) {
  uint32_t offset = getGlyphOffset(codeToGlyphId(code));
  file.seek(offset);
  glyph->numberOfContours = getInt16t();
  glyph->xMin = getInt16t();
  glyph->yMin = getInt16t();
  glyph->xMax = getInt16t();
  glyph->yMax = getInt16t();

  if (glyph->numberOfContours != -1) {
    readSimpleGlyph(glyph);
    return 1;
  }
  return 0;
}

/* free glyph */
void truetypeClass::freeGlyph(ttGlyph_t *glyph) {
  free(glyph->points);
  free(glyph->endPtsOfContours);
}

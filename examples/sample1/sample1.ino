#include "truetype.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

const int stmpeCs = 32;
const int tftCs = 15;
const int tftDc = 33;
const int sdCs = 14;

typedef struct {
  int x;
  int y;
} point_t ;

point_t *points = NULL;
int numPoints = 0;

int *beginPoints;
int numBeginPoints = 0;
int *endPoints;
int numEndPoints = 0;

Adafruit_ILI9341 tft = Adafruit_ILI9341(tftCs, tftDc);
truetypeClass ttf1 = truetypeClass(&SD);
truetypeClass ttf2 = truetypeClass(&SD);

const char *fontFile1 = "/ipag.ttf";
const char *fontFile2 = "/ipam.ttf";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  ttGlyph_t glyph;

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);

  ttf1.begin(sdCs, fontFile1);
  ttf2.begin(sdCs, fontFile2);

  wchar_t *s1 = L"あいうえお";
  wchar_t *s2 = L"憂鬱薔薇";
  char *s3 = "QuickBrownFoxLazyDog";

  tft.fillScreen(ILI9341_BLACK);

  for (int i = 0, x = 0; i < wcslen(s1); i++) {
    ttf1.readGlyph(s1[i], &glyph);
    ttf1.adjustGlyph(&glyph);
    x += outputTFT(x, 0, &glyph, &ttf1, 64);
    truetypeClass::freeGlyph(&glyph);
  }

  for (int i = 0, x = 0; i < wcslen(s1); i++) {
    ttf2.readGlyph(s1[i], &glyph);
    ttf2.adjustGlyph(&glyph);
    x += outputTFT(x, 50, &glyph, &ttf2, 64);
    truetypeClass::freeGlyph(&glyph);
  }

  for (int i = 0, x = 0; i < wcslen(s2); i++) {
    ttf1.readGlyph(s2[i], &glyph);
    ttf1.adjustGlyph(&glyph);
    x += outputTFT(x, 100, &glyph, &ttf1, 64);
    truetypeClass::freeGlyph(&glyph);
  }

  for (int i = 0, x = 0; i < wcslen(s2); i++) {
    ttf2.readGlyph(s2[i], &glyph);
    ttf2.adjustGlyph(&glyph);
    x += outputTFT(x, 150, &glyph, &ttf2, 64);
    truetypeClass::freeGlyph(&glyph);
  }

  for (int i = 0, x = 0; i < strlen(s3); i++) {
    ttf1.readGlyph(s3[i], &glyph);
    ttf1.adjustGlyph(&glyph);
    x += outputTFT(x, 200, &glyph, &ttf1, 48);
    truetypeClass::freeGlyph(&glyph);
  }
}

int getPixel(int x0, int y0, uint8_t *bitmap, int width, int height) {
  return (bitmap[(x0 / 8) + (((width + 7) / 8) * y0)]) & (1 << (7 - x0 % 8));
}

void drawPixel(int x0, int y0, uint8_t *bitmap, int width, int height) {
  bitmap[(x0 / 8) + (((width + 7) / 8) * y0)] |= (1 << (7 - x0 % 8));
}

/* Bresenham's line algorithm */
void drawLine(int x0, int y0, int x1, int y1, uint8_t *bitmap, int width, int height) {
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx, sy, err, e2;

  if (numPoints == 0) {
    addPoint(x0, y0);
    addBeginPoint(0);
  }
  addPoint(x1, y1);

  if (x0 < x1) {
    sx = 1;
  } else {
    sx = -1;
  }
  if (y0 < y1) {
    sy = 1;
  } else {
    sy = -1;
  }
  err = dx - dy;

  while (1) {
    drawPixel(x0, y0, bitmap, width, height);
    if ((x0 == x1) && (y0 == y1)) {
      break;
    }
    e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

int outputTFT(int x, int y, ttGlyph_t *glyph, truetypeClass *ttf, int height) {
  int width;

  width = height * (glyph->xMax - glyph->xMin) / (ttf->yMax - ttf->yMin);

  int length = (height * (width + 7) / 8);

  uint8_t *bitmap = (uint8_t *)calloc(length, sizeof(uint8_t));


  for (int i = 0, j = 0; i < glyph->numberOfContours; i++, j++) {
    while (j < glyph->endPtsOfContours[i]) {
      int x0, y0, x1, y1;
      if ((glyph->points[j].flag & FLAG_ONCURVE) == 1) {
        if ((glyph->points[j + 1].flag & FLAG_ONCURVE) == 1) {
          drawLine(map(glyph->points[j].x, glyph->xMin, glyph->xMax, 0, width - 1),
                   map(glyph->points[j].y, ttf->yMin, ttf->yMax, height - 1, 0),
                   map(glyph->points[j + 1].x, glyph->xMin, glyph->xMax, 0, width - 1),
                   map(glyph->points[j + 1].y, ttf->yMin, ttf->yMax, height - 1, 0),
                   bitmap, width, height);
        } else { /* off curve */
          x0 = glyph->points[j].x, y0 = glyph->points[j].y;
          for (float t = 0; t <= 1; t += 0.2) {
            x1 = (1 - t) * (1 - t) * glyph->points[j].x + 2 * t * (1 - t) * glyph->points[j + 1].x + t * t * glyph->points[j + 2].x;
            y1 = (1 - t) * (1 - t) * glyph->points[j].y + 2 * t * (1 - t) * glyph->points[j + 1].y + t * t * glyph->points[j + 2].y;
            drawLine(map(x0, glyph->xMin, glyph->xMax, 0, width - 1),
                     map(y0, ttf->yMin, ttf->yMax, height - 1, 0),
                     map(x1, glyph->xMin, glyph->xMax, 0, width - 1),
                     map(y1, ttf->yMin, ttf->yMax, height - 1, 0),
                     bitmap, width, height);
            x0 = x1, y0 = y1;
          }
          drawLine(map(x0, glyph->xMin, glyph->xMax, 0, width - 1),
                   map(y0, ttf->yMin, ttf->yMax, height - 1, 0),
                   map(glyph->points[j + 2].x, glyph->xMin, glyph->xMax, 0, width - 1),
                   map(glyph->points[j + 2].y, ttf->yMin, ttf->yMax, height - 1, 0),
                   bitmap, width, height);
          j++;
        }
      }
      j++;

    }
    addEndPoint(numPoints - 1);
    addBeginPoint(numPoints);
  }

  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      if (isInside(x, y)) {
        drawPixel(x, y, bitmap, width, height);
      }
    }
  }

  tft.drawBitmap(x, y, bitmap, width, height, 0xffff);

  free(bitmap);
  freePoints();
  freeBeginPoints();
  freeEndPoints();
  return width + 2;
}

void addPoint(int x, int y) {
  ++numPoints;
  points = (point_t *)realloc(points, sizeof(point_t) * numPoints);
  points[(numPoints - 1)].x = x;
  points[(numPoints - 1)].y = y;
}

void freePoints() {
  free(points);
  points = NULL;
  numPoints = 0;
}

void addBeginPoint(int bp) {
  ++numBeginPoints;
  beginPoints = (int *)realloc(beginPoints, sizeof(int) * numBeginPoints);
  beginPoints[(numBeginPoints - 1)] = bp;
}

void freeBeginPoints() {
  free(beginPoints);
  beginPoints = NULL;
  numBeginPoints = 0;
}

void addEndPoint(int ep) {
  ++numEndPoints;
  endPoints = (int *)realloc(endPoints, sizeof(int) * numEndPoints);
  endPoints[(numEndPoints - 1)] = ep;
}

void freeEndPoints() {
  free(endPoints);
  endPoints = NULL;
  numEndPoints = 0;
}

bool isInside(int x, int y) {
  int windingNumber = 0;
  int bpCounter = 0, epCounter = 0;
  point_t point = {x, y};

  point_t point1;
  point_t point2;

  for (int i = 0; i < numPoints; ++i) {
    point1.x = points[i].x;
    point1.y = points[i].y;
    // Wrap?
    if (i == endPoints[epCounter]) {
      point2.x = points[beginPoints[bpCounter]].x;
      point2.y = points[beginPoints[bpCounter]].y;
      epCounter++;
      bpCounter++;
    } else {
      point2.x = points[i + 1].x;
      point2.y = points[i + 1].y;
    }

    if (point1.y <= point.y) {
      if (point2.y > point.y) {
        if (isLeft(point1, point2, point) > 0) {
          ++windingNumber;  
        }
      }
    } else {
      // start y > point.y (no test needed)
      if (point2.y <= point.y) {
        if (isLeft(point1, point2, point) < 0) {
          --windingNumber;
        }
      }
    }
  }

  return (windingNumber != 0);
}

int isLeft(point_t &p0, point_t &p1, point_t &point) {
  return ((p1.x - p0.x) * (point.y - p0.y) - (point.x - p0.x) * (p1.y - p0.y));
}

void loop() {
  // put your main code here, to run repeatedly:

}

#include "Display.h"

// Waveshare ESP32 SPI pin assignments.
const int8_t kSpiPinMiso = 12;
const int8_t kSpiPinSck = 13;
const int8_t kSpiPinMosi = 14;
const int8_t kSpiPinCs = 15;
const int8_t kSpiPinBusy = 25;
const int8_t kSpiPinRst = 26;
const int8_t kSpiPinDc = 27;

void Display::Initialize() {
  Serial.println("Initializing display");

  // Allocate display buffers.
  gx_epd_ = new GxEPD2_3C<DISPLAY_TYPE, PAGE_HEIGHT>(
      DISPLAY_TYPE(kSpiPinCs, kSpiPinDc, kSpiPinRst, kSpiPinBusy));
  gx_epd_->init(serial_speed_);

  // Remap the Waveshare ESP32's non-standard SPI pins.
  SPI.end();
  SPI.begin(kSpiPinSck, kSpiPinMiso, kSpiPinMosi, kSpiPinCs);

  // Start paged drawing.
  gx_epd_->firstPage();
}

void Display::Load(const uint8_t* image_data, uint32_t size, uint32_t offset) {
  Serial.printf("Loading image data: %lu bytes\n", size);

  // Look at the image data one byte at a time, which is 4 input pixels.
  for (int i = 0; i < size; ++i) {
    // Read 4 input pixels.
    uint8_t input = image_data[i];
    uint16_t pixels[] = {
        ConvertPixel(input, 0xC0, 6),
        ConvertPixel(input, 0x30, 4),
        ConvertPixel(input, 0x0C, 2),
        ConvertPixel(input, 0x03, 0)
    };

    // Write 4 output pixels.
    for (int in = 0; in < 4; ++in) {
      uint16_t pixel = pixels[in];
      uint32_t out = 4 * (offset + i) + in;
      int16_t x = out % gx_epd_->width();
      int16_t y = out / gx_epd_->width();
      gx_epd_->drawPixel(x, y, pixel);

      // Trigger a display update after the last pixel of each page.
      if ((y + 1) % gx_epd_->pageHeight() == 0 && x == gx_epd_->width() - 1) {
        Serial.println("Updating display");
        gx_epd_->nextPage();
      }
    }
  }
}

void Display::Finalize() {
  Serial.println("Suspending display");
  gx_epd_->hibernate();

  // Free display buffers.
  delete gx_epd_;
}

void Display::ShowError() {
  ShowStatic(kErrorImageBlack, kErrorImageRed, kErrorWidth, kErrorHeight,
             kErrorBackground);
}

void Display::ShowWifiSetup() {
  ShowStatic(kWifiImageBlack, kWifiImageRed, kWifiWidth, kWifiHeight,
             kWifiBackground);
}

int16_t Display::Width() { return gx_epd_->width(); }

int16_t Display::Height() { return gx_epd_->height(); }

uint16_t Display::ConvertPixel(uint8_t input, uint8_t mask, uint8_t shift) {
  uint8_t value = (input & mask) >> shift;
  switch (value) {
    case 0x0:
      return GxEPD_BLACK;
    case 0x1:
      return GxEPD_WHITE;
    case 0x3:
      return GxEPD_RED;
    default:
      Serial.printf("Unknown pixel value: 0x%04X\n", value);
      return GxEPD_BLACK;
  }
}

void Display::ShowStatic(const uint8_t* black_data, const uint8_t* red_data,
                         uint16_t width, uint16_t height, uint16_t background) {
  Serial.println("Showing static image");

  Initialize();

  // Calculate the offset to center the static image in the display.
  int16_t x = (gx_epd_->width() - width) / 2;
  int16_t y = (gx_epd_->height() - height) / 2;

  do {
    // Fill in the background first.
    gx_epd_->fillScreen(background);

    // Draw the static image one color at a time.
    gx_epd_->fillRect(x, y, width, height, GxEPD_WHITE);
    gx_epd_->drawBitmap(x, y, black_data, width, height, GxEPD_BLACK);
    gx_epd_->drawBitmap(x, y, red_data, width, height, GxEPD_RED);
  } while (gx_epd_->nextPage());

  Finalize();
}

#pragma once

// Boot animation and PC bridge connection screens.

#include "state.h"
#include "config.h"

// Forward-declarations
void pollTimeSync();
void redrawCurrentScreen();

// Custom font rendering functions (from font_data.h)
extern void vaultSetCursor(int16_t x, int16_t y);
extern uint8_t vaultNextCodepoint(const char*& text);
extern void vaultPrintCodepoint(uint8_t cp);
extern int16_t vaultTextWidth(const char* text);
extern void vaultDrawText(int16_t x, int16_t y, const char* text, uint16_t fg = state.activeThemeColor, uint16_t bg = BLACK, uint8_t size = 1);
extern int16_t vaultCursorX, vaultCursorY;

namespace {
  void drawTerminalCursor() {
    tft.fillRect(vaultCursorX + 1, vaultCursorY + 1, 5, 8, state.activeThemeColor);
  }

  void clearTerminalCursor() {
    tft.fillRect(vaultCursorX + 1, vaultCursorY + 1, 5, 8, BLACK);
  }

  void terminalPrintLine(int16_t x, int16_t &y, const char *text, uint16_t charDelayMs, uint16_t pauseMs, uint8_t lineHeight) {
    vaultSetCursor(x, y);
    drawTerminalCursor();
    while (*text) {
      clearTerminalCursor();
      uint8_t cp = vaultNextCodepoint(text);
      vaultPrintCodepoint(cp);
      drawTerminalCursor();
      bootDelay(charDelayMs);
    }
    bootDelay(pauseMs);
    clearTerminalCursor();
    y += lineHeight;
  }

  void terminalSetLineWithCursor(int16_t x, int16_t y, const char *text) {
    tft.fillRect(x, y, SCREEN_W - x - 5, 12, BLACK);
    vaultSetCursor(x, y);
    vaultDrawText(x, y, text);
    drawTerminalCursor();
  }

  void terminalTypeTextWithCursor(const char *text, uint16_t charDelayMs) {
    while (*text) {
      clearTerminalCursor();
      vaultPrintCodepoint(vaultNextCodepoint(text));
      drawTerminalCursor();
      bootDelay(charDelayMs);
    }
  }

  void terminalAnimateEllipsis(int16_t x, int16_t y, const char *baseText, uint8_t cycles, uint16_t dotDelayMs, uint16_t charDelayMs, uint16_t pauseMs) {
    terminalSetLineWithCursor(x, y, baseText);
    int16_t dotsX = x + vaultTextWidth(baseText);
    for (uint8_t cycle = 0; cycle < cycles; cycle++) {
      clearTerminalCursor();
      tft.fillRect(dotsX, y, SCREEN_W - dotsX - 5, 12, BLACK);
      vaultSetCursor(dotsX, y);
      drawTerminalCursor();
      for (uint8_t dot = 0; dot < 3; dot++) {
        bootDelay(dotDelayMs);
        terminalTypeTextWithCursor(".", charDelayMs);
      }
      bootDelay(pauseMs);
    }
  }
} // anonymous namespace

void bootSequence() {
  displayWake();
  applyThemeColor();
  crtPowerOn();
  clearScreenWithCrt();
  tft.setTextColor(state.activeThemeColor, BLACK);
  tft.setTextSize(1);

  const char *bootHeader = "РЈР‘Р•Р–РР©Р• 337: РџР РћР’Р•Р РљРђ РџРћРЎРўРђ";
  const char *lines[] = {
    "S.O.C.H.-Tec CENTRAL POST", "РЎР›РЈР–Р‘Рђ РђРўРњРћРЎР¤Р•Р Р«, 2026", "", bootHeader, ""
  };

  int16_t y = 8;
  for (const char *line : lines) {
    terminalPrintLine(5, y, line, strlen(line) == 0 ? 180 : 38, strlen(line) == 0 ? 220 : 360, 14);
    if (strcmp(line, bootHeader) == 0) {
      terminalPrintLine(5, y, "-------------------------", 46, 320, 14);
    }
  }

  struct BootDiagnostic {
    const char *label;
    bool isOnline;
    bool isWarmingUp;
  };

  const BootDiagnostic diagnostics[] = {
    {"- BME280/Р’РќР•РЁРќ", state.bmeOutsideOnline, false},
    {"- BME280/Р–РР›РћР™", state.bmeRoomOnline, false},
    {"- GL5516/РЎР’Р•Рў", state.lightSensorOnline, false},
    {"- MQ-135/Р’РћР—Р”РЈРҐ", state.mqSensorOnline, true},
    {"- РљРћРќРўРЈР /РџР РРў", state.relayModuleOnline, false}
  };

  for (const auto &diag : diagnostics) {
    const char* status = USE_MOCK_SENSORS ? "[ РЎРРњРЈР›РЇР¦РРЇ ]" : (diag.isOnline ? (diag.isWarmingUp ? "[ РџР РћР“Р Р•Р’ ]" : "[ OK ]") : "[ РЎР‘РћР™ ]");
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%-16s %s", diag.label, status);
    terminalPrintLine(5, y, buffer, 28, 420, 14);
  }

  bootDelay(260);
  terminalAnimateEllipsis(5, y, "РРќРР¦РРђР›РР—РђР¦РРЇ РџРћРЎРўРђ", 3, 430, 28, 360);
  bootDelay(650);
  terminalSetLineWithCursor(5, y, "РРќРР¦РРђР›РР—РђР¦РРЇ РџРћРЎРўРђ... OK");
  clearTerminalCursor();
  bootDelay(900);
}

void showPcBridgeAnimation() {
  bool deviceAwake, bootComplete;
  if (LOCK_STATE()) {
  deviceAwake = state.deviceAwake;
  bootComplete = state.bootComplete;
  UNLOCK_STATE();
  }
  
  if (!deviceAwake || !bootComplete) return;

  displayWake();
  applyThemeColor();
  clearScreenWithCrt();
  tft.setTextSize(1);
  
  uint16_t activeThemeColor;
  if (LOCK_STATE()) {
  activeThemeColor = state.activeThemeColor;
  UNLOCK_STATE();
  }
  
  tft.setTextColor(activeThemeColor, BLACK);
  tft.drawRect(0, 0, SCREEN_W, SCREEN_H, activeThemeColor);

  auto terminalPrintAt = [&](int16_t x, int16_t y, const char *text, uint16_t charDelayMs = 34, uint16_t pauseMs = 130) {
    int16_t lineY = y;
    terminalPrintLine(x, lineY, text, charDelayMs, pauseMs, 12);
  };

  auto drawPcLinkRule = [&](int16_t y) {
    terminalPrintAt(12, y, "-------------------------", 34, 120);
  };

  terminalPrintAt(8, 12, "РљРђРќРђР› Р¦Р•РќРўР РђР›Р¬РќРћР“Рћ", 46, 160);
  terminalPrintAt(8, 28, "РџРћРЎРўРђ...", 46, 200);
  terminalPrintAt(8, 52, "РџРћРРЎРљ РЈР—Р›Рђ ESP32");
  terminalPrintAt(8, 68, "РћР‘РќРђР РЈР–Р•Рќ РџРћРЎРў:");
  terminalPrintAt(8, 84, "S.O.C.H.-Tec CENTRAL", 34, 160);
  drawPcLinkRule(106);
  terminalPrintAt(8, 124, "[!] Р”РћРџРЈРЎРљ РЎРњРћРўР РРўР•Р›РЇ");
  terminalPrintAt(8, 140, "РўР Р•Р‘РЈР•Рў РџР РћРўРћРљРћР›Рђ");
  terminalPrintAt(8, 156, "S.O.C.H.-Tec", 34, 140);
  drawPcLinkRule(178);
  terminalAnimateEllipsis(8, 188, "Р—РђРџР РћРЎ РЎР•РЎРЎРР РџРћРЎРўРђ", 3, 520, 28, 340);
  clearTerminalCursor();
  terminalPrintAt(8, 206, "РћРўР’Р•Рў РџРћР›РЈР§Р•Рќ", 34, 160);
  terminalPrintAt(8, 222, "РЎРРќРҐР . РђР РҐРР’Рђ РђРўРњРћРЎР¤Р•Р Р«... OK", 30, 130);
  terminalPrintAt(8, 238, "РљРђРќРђР› Р¦Р•РќРўР . РџРћРЎРўРђ РЈРЎРўРђРќРћР’Р›Р•Рќ.", 34, 130);
  drawPcLinkRule(268);
  terminalPrintAt(8, 280, "Р”РћР‘Р Рћ РџРћР–РђР›РћР’РђРўР¬ Р’ РЎР•РўР¬,", 34, 120);
  terminalPrintAt(8, 292, "РЎРњРћРўР РРўР•Р›Р¬.", 34, 160);
  bootDelay(900);
  redrawCurrentScreen();
}

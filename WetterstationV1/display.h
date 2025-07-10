/* display.h */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

/* Display-Grundlagen */
#define SCREEN_W 128
#define SCREEN_H  64

/* Schriftmaße */
#define FONT_W     3
#define FONT_H     5

/* Plot-Bereiche
 * HINWEIS: PLOT_X0 und PLOT_X1 wurden angepasst, um links mehr
 * Platz für die Achsenbeschriftung zu schaffen.
 */
#define PLOT_X0     30  /* War 22 */
#define PLOT_X1    125  /* War 117 */
#define PLOT_Y0      4
#define PLOT_Y1     57

/**
 * Page-Buffer für ks0108_write_page
 */
extern uint8_t pageBuf[SCREEN_W];

/**
 * Aktuelle Seite (1–5), definiert in main.c
 */
extern volatile uint8_t pageNumber;

/**
 * Inversion ein/aus
 */
extern bool displayInverted;

/**
 * @brief Löscht die Page (Buffer) und zeichnet den Rahmen
 * (vertikal in jeder Page + horizontal oben/unten).
 */
void clearPage(uint8_t pg);

/**
 * @brief Rendert Szene 0–4 in die Page.
 */
void renderScene(uint8_t scene, uint8_t pg);

/**
 * @brief Zeichnet eine Zahl mit dp Dezimalstellen.
 */
void drawNumber(int x, int y, int16_t val, uint8_t dp, uint8_t pg);

#endif /* DISPLAY_H */

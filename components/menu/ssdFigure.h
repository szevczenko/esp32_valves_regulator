#ifndef SSD_FIGURE_H
#define SSD_FIGURE_H
#include "app_config.h"

typedef struct ssdFigure
{
  uint8_t x;
  uint8_t y;
  uint8_t height;
  uint8_t width;
  uint8_t fill;
} loadBar_t;

typedef struct
{
  uint8_t y_start;
  uint8_t line_max;
  uint8_t all_line;
  uint8_t actual_line;
} scrollBar_t;

int ssdFigureDrawLoadBar( loadBar_t* figure );
int ssdFigureDrawScrollBar( scrollBar_t* figure );
int ssdFigureFillLine( int y_start, int height );
void drawBattery( uint8_t x, uint8_t y, float accum_voltage, bool is_charging );
void drawSignal( uint8_t x, uint8_t y, uint8_t signal_lvl );

void ssdFigure_DrawValve( uint8_t x, uint8_t y, bool is_error );
void ssdFigure_DrawValveAnimation( uint8_t x, uint8_t y, uint8_t animation );
void ssdFigure_DrawAcceptButton( uint8_t x, uint8_t y );
void ssdFigure_DrawDeclineButton( uint8_t x, uint8_t y );
void ssdFigure_DrawArrow( uint8_t x, uint8_t y, bool up );
void ssdFigure_DrawTank( uint8_t x, uint8_t y, uint8_t filled );
void ssdFigure_DrawLowAccu( uint8_t x, uint8_t y, float acc_voltage, float acc_current );

#endif
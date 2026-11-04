#ifndef KS9A_H
#define KS9A_H

//  KS9A Splash Screen
void drawSplashScreen(TFT_eSPI &tft) {
    //  Gradient Sky
    for (int i = 0; i < 320; i++) {
        uint16_t skyColor = tft.color565(25 - (i/20), 0, 50 - (i/10));
        tft.drawFastHLine(0, i, 480, skyColor);
    }

    //  Rohn Tower Base
    int tx = 380;       
    int baseLine = 315; 
    int topHeight = 130; 
    int tw = 14;        

    // Main Legs
    tft.drawLine(tx - tw, baseLine, tx - tw, topHeight, TFT_SILVER);
    tft.drawLine(tx + tw, baseLine, tx + tw, topHeight, TFT_SILVER);

    // Cross-bracing
    for (int y = baseLine; y > topHeight + 15; y -= 20) {
        tft.drawFastHLine(tx - tw, y, tw * 2, TFT_SILVER); 
        tft.drawLine(tx - tw, y, tx + tw, y - 20, TFT_SILVER); 
    }

    //  THE TOP HAT & MAST
    int mastTopY = topHeight - 40; // The tip of the taper
    tft.drawLine(tx - tw, topHeight, tx, mastTopY, TFT_SILVER); // Left Taper
    tft.drawLine(tx + tw, topHeight, tx, mastTopY, TFT_SILVER); // Right Taper
    
    // The Mast (2-inch pipe coming out of the taper)
    tft.drawFastVLine(tx, mastTopY - 30, 30, TFT_SILVER);
    tft.drawFastVLine(tx + 1, mastTopY - 30, 30, TFT_SILVER); // Double thick mast

    //  THE HF BEAM (Centered on Mast)
    int bx = tx;              // The Mast/Tower center (380)
    int by = mastTopY - 20;   // The height of the boom-to-mast clamp
    
    // THE BOOM - Now centered on the mast, pointing left and right
    // We draw it from back-right to front-left
    tft.drawLine(bx + 60, by + 20, bx - 180, by - 60, TFT_DARKGREY);
    tft.drawLine(bx + 60, by + 21, bx - 180, by - 59, TFT_DARKGREY); 

    // --- HORIZONTAL ELEMENTS (Centered on the Boom) ---

    //  Reflector (Back - mounted at bx + 40)
    int rX = bx + 40; 
    int rY = by + 13;
    tft.drawLine(rX - 160, rY, rX + 160, rY, TFT_WHITE); // Centered 320px wide
    tft.drawLine(rX - 160, rY + 1, rX + 160, rY + 1, TFT_WHITE);

    //  Driven Element (Mounted near the Mast at bx - 20)
    int deX = bx - 20; 
    int deY = by - 8;
    tft.drawLine(deX - 140, deY, deX + 140, deY, TFT_WHITE); // Centered 280px wide
    tft.drawLine(deX - 140, deY + 1, deX + 140, deY + 1, TFT_WHITE);

    //  Director 1 (Mounted at bx - 80)
    int d1X = bx - 80; 
    int d1Y = by - 28;
    tft.drawLine(d1X - 120, d1Y, d1X + 120, d1Y, TFT_WHITE); // Centered 240px wide

    //  Director 2 (Mounted at bx - 140)
    int d2X = bx - 140; 
    int d2Y = by - 48;
    tft.drawLine(d2X - 100, d2Y, d2X + 100, d2Y, TFT_WHITE); // Centered 200px wide

    

    //  Branding (Kept clean on the left)
    tft.setFreeFont(&FreeSansBold18pt7b);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Multus SDR, LLC", 180, 150);
    tft.setTextColor(TFT_BLACK);
    tft.drawString("Graphics by KS9A", 183, 213); 
    tft.setTextColor(TFT_PURPLE);
    tft.drawString("Graphics by KS9A", 180, 210);

    tft.setFreeFont(&FreeSansBold12pt7b);
    tft.setTextColor(TFT_WHITE);
    #ifdef VERSION_HF
      tft.drawString("160M - 10M SWR METER", 185, 270);
    #else
      tft.drawString("LONGWAVE SWR METER", 185, 270);
    #endif

    delay(3000);
    tft.setFreeFont(NULL);    
    tft.setTextSize(1);       
    tft.setTextDatum(TL_DATUM); 

}

#endif
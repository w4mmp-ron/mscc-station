// ========================================================
// Cross-needle analog face for the 480x320 SWR meter.
// Original Daiwa-style geometry for this panel — not a port.
// Printed card is 10 W forward / 2 W reflected. x1 / x10 / x100
// only changes the corner legend and needle mapping.
// Full 480x320 face — no lower mask. SWR contours stop on the power arcs.
// ========================================================
#pragma once

#include <math.h>
#include <stdio.h>
#include <lvgl.h>

#ifndef CN_DEG2RAD
#define CN_DEG2RAD 0.01745329252f
#endif

// Screen-space construction. 0 deg = 3 o'clock, clockwise (LVGL).
static const float CN_FWD_CX = 428.0f;
static const float CN_FWD_CY = 354.0f;
static const float CN_REF_CX = 52.0f;
static const float CN_REF_CY = 354.0f;
static const float CN_R      = 382.0f;
static const float CN_FWD_A0 = 188.0f;   // rest (printed 0) — lower-left corner
static const float CN_FWD_A1 = 235.0f;   // full scale
static const float CN_REF_A0 = 352.0f;
static const float CN_REF_A1 = 305.0f;
static const float CN_ARC_INSET = 2.0f;
static const float CN_FWD_FS = 10.0f;
static const float CN_REF_FS = 2.0f;

static const float CN_GUIDE_SWR[] = {1.2f, 1.5f, 2.0f, 3.0f, 5.0f};
static const char *CN_GUIDE_LBL[] = {"1.2", "1.5", "2", "3", "5"};
// Fraction of each contour from the hidden rest (0) toward the power arcs (1).
// 2/3/5 sit lower in the fan so they do not land in the scale numbers.
static const float CN_GUIDE_LBL_FRAC[] = {0.52f, 0.48f, 0.36f, 0.32f, 0.30f};
static const int CN_GUIDE_COUNT = 5;
static const int CN_GUIDE_SAMPLES = 32;

static lv_obj_t *cn_face = NULL;
static lv_obj_t *cn_needles = NULL;
static lv_obj_t *cn_range_btn = NULL;
static lv_obj_t *cn_range_lbl = NULL;
static lv_point_precise_t cn_guides[CN_GUIDE_COUNT][CN_GUIDE_SAMPLES];
static uint8_t cn_guide_len[CN_GUIDE_COUNT];
static bool cn_guides_ready = false;
static float cn_ang_fwd = CN_FWD_A0;
static float cn_ang_ref = CN_REF_A0;
static const float CN_NEEDLE_TIP_W = 3.0f;
static const float CN_NEEDLE_HUB_W = 9.0f;

static int  cn_range_mult = 1;      // 1, 10, 100
static bool cn_range_locked = false;
static bool cn_range_long_fired = false;
static uint32_t cn_idle_tick = 0;
static float cn_swr_alarm = 2.0f;
static int  cn_ref_band = 0;   // 0 green, 1 yellow, 2 red
static int  cn_last_fwd_x = -1;
static int  cn_last_fwd_y = -1;
static int  cn_last_ref_x = -1;
static int  cn_last_ref_y = -1;
static bool cn_last_alarm = false;

static float cn_deg2rad(float deg) { return deg * CN_DEG2RAD; }

static float cn_lerp_angle(float a0, float a1, float t) {
    return a0 + (a1 - a0) * t;
}

// D'Arsonval + square-law feel. p may exceed 1 for contour art.
static float cn_response(float p) {
    if (p < 0.0f) p = 0.0f;
    return sqrtf(p);
}

static float cn_angle_fwd(float watts, float fullScale, bool clamp) {
    float p = watts / fullScale;
    if (clamp) {
        if (p < 0.0f) p = 0.0f;
        if (p > 1.0f) p = 1.0f;
    }
    return cn_lerp_angle(CN_FWD_A0, CN_FWD_A1, cn_response(p));
}

static float cn_angle_ref(float watts, float fullScale, bool clamp) {
    float p = watts / fullScale;
    if (clamp) {
        if (p < 0.0f) p = 0.0f;
        if (p > 1.0f) p = 1.0f;
    }
    return cn_lerp_angle(CN_REF_A0, CN_REF_A1, cn_response(p));
}

static bool cn_intersect(float angF, float angR, float *ox, float *oy) {
    float a1 = cn_deg2rad(angF);
    float a2 = cn_deg2rad(angR);
    float d1x = cosf(a1), d1y = sinf(a1);
    float d2x = cosf(a2), d2y = sinf(a2);
    float det = d1x * d2y - d1y * d2x;
    if (fabsf(det) < 1e-4f) return false;
    float dx = CN_REF_CX - CN_FWD_CX;
    float dy = CN_REF_CY - CN_FWD_CY;
    float t = (dx * d2y - dy * d2x) / det;
    *ox = CN_FWD_CX + t * d1x;
    *oy = CN_FWD_CY + t * d1y;
    return true;
}

static void cn_ray_point(float cx, float cy, float angDeg, float r, float *x, float *y) {
    float a = cn_deg2rad(angDeg);
    *x = cx + r * cosf(a);
    *y = cy + r * sinf(a);
}

static float cn_visible_t0(float sa) {
    float t0 = 8.0f;
    if (fabsf(sa) > 0.05f) {
        t0 = (318.0f - CN_FWD_CY) / sa;
        if (t0 < 8.0f) t0 = 8.0f;
        if (t0 > CN_R - 12.0f) t0 = CN_R - 12.0f;
    }
    return t0;
}

static float cn_needle_width_at(float t) {
    float u = t / CN_R;
    if (u < 0.0f) u = 0.0f;
    if (u > 1.0f) u = 1.0f;
    return CN_NEEDLE_HUB_W + (CN_NEEDLE_TIP_W - CN_NEEDLE_HUB_W) * u;
}

static void cn_draw_taper(lv_layer_t *layer, float cx, float cy, float angDeg, uint32_t color) {
    float a = cn_deg2rad(angDeg);
    float ca = cosf(a);
    float sa = sinf(a);
    float px = -sa;
    float py = ca;
    float t0 = cn_visible_t0(sa);
    float w0 = cn_needle_width_at(t0) * 0.5f;
    float w1 = CN_NEEDLE_TIP_W * 0.5f;
    float x0 = cx + t0 * ca;
    float y0 = cy + t0 * sa;
    float x1 = cx + CN_R * ca;
    float y1 = cy + CN_R * sa;

    lv_draw_triangle_dsc_t tri;
    lv_draw_triangle_dsc_init(&tri);
    tri.color = lv_color_hex(color);
    tri.opa = LV_OPA_COVER;

    tri.p[0].x = (lv_value_precise_t)(x0 + px * w0 + 0.5f);
    tri.p[0].y = (lv_value_precise_t)(y0 + py * w0 + 0.5f);
    tri.p[1].x = (lv_value_precise_t)(x0 - px * w0 + 0.5f);
    tri.p[1].y = (lv_value_precise_t)(y0 - py * w0 + 0.5f);
    tri.p[2].x = (lv_value_precise_t)(x1 - px * w1 + 0.5f);
    tri.p[2].y = (lv_value_precise_t)(y1 - py * w1 + 0.5f);
    lv_draw_triangle(layer, &tri);

    tri.p[0].x = (lv_value_precise_t)(x0 + px * w0 + 0.5f);
    tri.p[0].y = (lv_value_precise_t)(y0 + py * w0 + 0.5f);
    tri.p[1].x = (lv_value_precise_t)(x1 - px * w1 + 0.5f);
    tri.p[1].y = (lv_value_precise_t)(y1 - py * w1 + 0.5f);
    tri.p[2].x = (lv_value_precise_t)(x1 + px * w1 + 0.5f);
    tri.p[2].y = (lv_value_precise_t)(y1 + py * w1 + 0.5f);
    lv_draw_triangle(layer, &tri);
}

static void cn_needle_bbox(float cx, float cy, float angDeg, lv_area_t *area) {
    float a = cn_deg2rad(angDeg);
    float ca = cosf(a);
    float sa = sinf(a);
    float t0 = cn_visible_t0(sa);
    float pad = CN_NEEDLE_HUB_W + 4.0f;
    float x0 = cx + t0 * ca;
    float y0 = cy + t0 * sa;
    float x1 = cx + CN_R * ca;
    float y1 = cy + CN_R * sa;
    area->x1 = (int32_t)((x0 < x1 ? x0 : x1) - pad);
    area->y1 = (int32_t)((y0 < y1 ? y0 : y1) - pad);
    area->x2 = (int32_t)((x0 > x1 ? x0 : x1) + pad);
    area->y2 = (int32_t)((y0 > y1 ? y0 : y1) + pad);
}

static void cn_needles_draw_cb(lv_event_t *e) {
    lv_layer_t *layer = lv_event_get_layer(e);
    cn_draw_taper(layer, CN_FWD_CX, CN_FWD_CY, cn_ang_fwd, 0xE07018);
    cn_draw_taper(layer, CN_REF_CX, CN_REF_CY, cn_ang_ref, 0xD04018);
}

static bool cn_inside_arcs(float x, float y) {
    float df = hypotf(x - CN_FWD_CX, y - CN_FWD_CY);
    float dr = hypotf(x - CN_REF_CX, y - CN_REF_CY);
    if (df > CN_R - CN_ARC_INSET) return false;
    if (dr > CN_R - CN_ARC_INSET) return false;
    if (y < 2.0f || y > 318.0f) return false;
    if (x < 2.0f || x > 478.0f) return false;
    return true;
}

static void cn_build_guides(void) {
    if (cn_guides_ready) return;
    for (int g = 0; g < CN_GUIDE_COUNT; g++) {
        float swr = CN_GUIDE_SWR[g];
        float rho = (swr - 1.0f) / (swr + 1.0f);
        float ratio = rho * rho;
        uint8_t n = 0;
        float last_x = 0.0f, last_y = 0.0f;
        bool have = false;
        const int sweep = 48;
        for (int i = 0; i < sweep; i++) {
            float p = 0.02f + 2.10f * ((float)i / (float)(sweep - 1));
            float pf = p * CN_FWD_FS;
            float pr = pf * ratio;
            bool ref_pegged = (pr >= CN_REF_FS);
            // SWR 2 may run past 10 W FWD so the contour has length.
            // 1.19x is halfway between the 10 W stub and the 1.38x spike.
            bool fwd_pegged = (swr >= 1.9f && swr < 2.5f && pf >= CN_FWD_FS * 1.19f);
            if (ref_pegged) {
                pr = CN_REF_FS;
                pf = pr / ratio;
            } else if (fwd_pegged) {
                pf = CN_FWD_FS;
                pr = pf * ratio;
            }
            float ox, oy;
            if (!cn_intersect(cn_angle_fwd(pf, CN_FWD_FS, false),
                              cn_angle_ref(pr, CN_REF_FS, false),
                              &ox, &oy)) {
                continue;
            }
            if (cn_inside_arcs(ox, oy)) {
                if (n < CN_GUIDE_SAMPLES) {
                    cn_guides[g][n].x = (lv_value_precise_t)(ox + 0.5f);
                    cn_guides[g][n].y = (lv_value_precise_t)(oy + 0.5f);
                    n++;
                }
                last_x = ox;
                last_y = oy;
                have = true;
                if (ref_pegged || fwd_pegged) break;
            } else if (have) {
                // Walk back onto the nearer power arc so the contour ends there.
                float x0 = last_x, y0 = last_y, x1 = ox, y1 = oy;
                for (int k = 0; k < 8; k++) {
                    float mx = 0.5f * (x0 + x1);
                    float my = 0.5f * (y0 + y1);
                    if (cn_inside_arcs(mx, my)) {
                        x0 = mx;
                        y0 = my;
                    } else {
                        x1 = mx;
                        y1 = my;
                    }
                }
                if (n < CN_GUIDE_SAMPLES) {
                    cn_guides[g][n].x = (lv_value_precise_t)(x0 + 0.5f);
                    cn_guides[g][n].y = (lv_value_precise_t)(y0 + 0.5f);
                    n++;
                }
                break;
            }
        }
        cn_guide_len[g] = n;
    }
    cn_guides_ready = true;
}

static void cn_draw_line(lv_layer_t *layer, float x1, float y1, float x2, float y2,
                         lv_color_t color, int32_t width, lv_opa_t opa) {
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = color;
    dsc.width = width;
    dsc.opa = opa;
    dsc.round_start = 1;
    dsc.round_end = 1;
    dsc.p1.x = (lv_value_precise_t)(x1 + 0.5f);
    dsc.p1.y = (lv_value_precise_t)(y1 + 0.5f);
    dsc.p2.x = (lv_value_precise_t)(x2 + 0.5f);
    dsc.p2.y = (lv_value_precise_t)(y2 + 0.5f);
    lv_draw_line(layer, &dsc);
}

static void cn_draw_label_at(lv_layer_t *layer, const char *txt, float cx, float cy,
                             const lv_font_t *font, lv_color_t color) {
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.text = txt;
    dsc.font = font;
    dsc.color = color;
    dsc.align = LV_TEXT_ALIGN_CENTER;
    dsc.opa = LV_OPA_COVER;

    lv_point_t sz;
    lv_text_get_size(&sz, txt, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_area_t box;
    box.x1 = (int32_t)(cx - sz.x * 0.5f);
    box.y1 = (int32_t)(cy - sz.y * 0.5f);
    box.x2 = box.x1 + sz.x;
    box.y2 = box.y1 + sz.y;
    lv_draw_label(layer, &dsc, &box);
}

static float cn_scale_angle(bool fwd, float watts) {
    float fs = fwd ? CN_FWD_FS : CN_REF_FS;
    float a0 = fwd ? CN_FWD_A0 : CN_REF_A0;
    float a1 = fwd ? CN_FWD_A1 : CN_REF_A1;
    float p = watts / fs;
    if (p < 0.0f) p = 0.0f;
    if (p > 1.0f) p = 1.0f;
    return cn_lerp_angle(a0, a1, cn_response(p));
}

static void cn_stroke_scale_span(lv_layer_t *layer, bool fwd, float w0, float w1,
                                 lv_color_t color, int32_t width, lv_opa_t opa) {
    if (w1 <= w0 + 0.01f) return;
    float a_lo = cn_scale_angle(fwd, w0);
    float a_hi = cn_scale_angle(fwd, w1);
    float start = fwd ? a_lo : a_hi;
    float end   = fwd ? a_hi : a_lo;
    if (fabsf(end - start) < 0.4f) return;

    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.color = color;
    arc.width = width;
    arc.opa = opa;
    arc.center.x = (int32_t)((fwd ? CN_FWD_CX : CN_REF_CX) + 0.5f);
    arc.center.y = (int32_t)((fwd ? CN_FWD_CY : CN_REF_CY) + 0.5f);
    arc.radius = (uint16_t)CN_R;   // outer edge; width fills inward over the ticks
    arc.start_angle = start;
    arc.end_angle = end;
    lv_draw_arc(layer, &arc);
}

static int cn_band_from_swr(float swr, float alarm, int prev) {
    float yel = alarm * 0.8f;
    if (yel < 1.05f) yel = 1.05f;
    if (prev == 2) {
        if (swr < alarm - 0.08f) return (swr >= yel) ? 1 : 0;
        return 2;
    }
    if (prev == 1) {
        if (swr >= alarm) return 2;
        if (swr < yel - 0.05f) return 0;
        return 1;
    }
    if (swr >= alarm) return 2;
    if (swr >= yel) return 1;
    return 0;
}

static void cn_draw_scale(lv_layer_t *layer, bool fwd) {
    const float cx = fwd ? CN_FWD_CX : CN_REF_CX;
    const float cy = fwd ? CN_FWD_CY : CN_REF_CY;
    const float a0 = fwd ? CN_FWD_A0 : CN_REF_A0;
    const float a1 = fwd ? CN_FWD_A1 : CN_REF_A1;
    const float fs = fwd ? CN_FWD_FS : CN_REF_FS;
    const lv_color_t ink = lv_color_hex(0x1C1C1A);
    const lv_color_t dim = lv_color_hex(0x4A4844);
    const lv_color_t grn = lv_color_hex(0x2E8B3A);
    const lv_color_t yel = lv_color_hex(0xD4A017);
    const lv_color_t red = lv_color_hex(0xC42820);

    // Color wash fills the major-tick depth (14 px), translucent so ink still reads.
    const int32_t band_w = 14;
    const lv_opa_t band_opa = LV_OPA_50;
    if (fwd) {
        cn_stroke_scale_span(layer, true, 0.0f, 9.0f, grn, band_w, band_opa);
        cn_stroke_scale_span(layer, true, 9.0f, 10.0f, yel, band_w, band_opa);
    } else {
        lv_color_t refc = grn;
        if (cn_ref_band == 1) refc = yel;
        else if (cn_ref_band == 2) refc = red;
        cn_stroke_scale_span(layer, false, 0.0f, CN_REF_FS, refc, band_w, band_opa);
    }

    lv_draw_arc_dsc_t arc;
    lv_draw_arc_dsc_init(&arc);
    arc.color = dim;
    arc.width = 1;
    arc.opa = LV_OPA_COVER;
    arc.center.x = (int32_t)(cx + 0.5f);
    arc.center.y = (int32_t)(cy + 0.5f);
    arc.radius = (uint16_t)(CN_R - 14.0f);
    if (fwd) {
        arc.start_angle = a0;
        arc.end_angle = a1;
    } else {
        arc.start_angle = a1;
        arc.end_angle = a0;
    }
    lv_draw_arc(layer, &arc);

    const int steps = fwd ? 21 : 9;          // 0.5 W / 0.25 W
    const float stepVal = fwd ? 0.5f : 0.25f;
    const int majorEvery = fwd ? 4 : 2;

    char buf[8];
    for (int i = 0; i < steps; i++) {
        float val = i * stepVal;
        float p = val / fs;
        float ang = cn_lerp_angle(a0, a1, cn_response(p));
        bool major = (i % majorEvery) == 0;
        if (fwd && fabsf(val - 1.0f) < 0.01f) major = true;
        float x0, y0, x1, y1;
        cn_ray_point(cx, cy, ang, CN_R, &x0, &y0);
        cn_ray_point(cx, cy, ang, CN_R - (major ? 14.0f : 7.0f), &x1, &y1);
        cn_draw_line(layer, x0, y0, x1, y1, major ? ink : dim, major ? 2 : 1, LV_OPA_COVER);

        if (major) {
            float lx, ly;
            cn_ray_point(cx, cy, ang, CN_R + 13.0f, &lx, &ly);
            if (fwd) {
                if (val < 0.1f) snprintf(buf, sizeof(buf), "0");
                else snprintf(buf, sizeof(buf), "%d", (int)(val + 0.1f));
            } else {
                if (val < 0.05f) snprintf(buf, sizeof(buf), "0");
                else if (fabsf(val - (int)val) < 0.05f) snprintf(buf, sizeof(buf), "%d", (int)(val + 0.1f));
                else snprintf(buf, sizeof(buf), "%.1f", (double)val);
            }
            cn_draw_label_at(layer, buf, lx, ly, &lv_font_montserrat_14, ink);
        }
    }
}

static void cn_face_draw_cb(lv_event_t *e) {
    lv_layer_t *layer = lv_event_get_layer(e);
    cn_build_guides();

    const lv_color_t copper = lv_color_hex(0xC42820);
    const lv_color_t cream  = lv_color_hex(0x1C1C1A);

    cn_draw_scale(layer, true);
    cn_draw_scale(layer, false);

    for (int g = 0; g < CN_GUIDE_COUNT; g++) {
        int n = cn_guide_len[g];
        if (n < 2) continue;
        lv_draw_line_dsc_t dsc;
        lv_draw_line_dsc_init(&dsc);
        dsc.color = copper;
        dsc.width = 2;
        dsc.opa = LV_OPA_COVER;
        dsc.round_start = 1;
        dsc.round_end = 1;
        for (int i = 1; i < n; i++) {
            dsc.p1 = cn_guides[g][i - 1];
            dsc.p2 = cn_guides[g][i];
            lv_draw_line(layer, &dsc);
        }

        int idx = (int)((n - 1) * CN_GUIDE_LBL_FRAC[g] + 0.5f);
        if (idx < 1) idx = 1;
        if (idx > n - 2) idx = n - 2;
        float lx = (float)cn_guides[g][idx].x;
        float ly = (float)cn_guides[g][idx].y;
        float txd = (float)cn_guides[g][idx + 1].x - (float)cn_guides[g][idx - 1].x;
        float tyd = (float)cn_guides[g][idx + 1].y - (float)cn_guides[g][idx - 1].y;
        float tlen = hypotf(txd, tyd);
        if (tlen > 1.0f) {
            float nx = -tyd / tlen;
            float ny = txd / tlen;
            if (nx * (240.0f - lx) + ny * (190.0f - ly) < 0.0f) {
                nx = -nx;
                ny = -ny;
            }
            lx += nx * 12.0f;
            ly += ny * 12.0f;
        }
        cn_draw_label_at(layer, CN_GUIDE_LBL[g], lx, ly, &lv_font_montserrat_14, copper);
    }

    float tx, ty;
    const float title_r = CN_R + 52.0f;
    const float title_a_fwd = CN_FWD_A0 + 0.28f * (CN_FWD_A1 - CN_FWD_A0);
    const float title_a_ref = CN_REF_A0 + 0.28f * (CN_REF_A1 - CN_REF_A0);
    cn_ray_point(CN_FWD_CX, CN_FWD_CY, title_a_fwd, title_r, &tx, &ty);
    cn_draw_label_at(layer, "FWD", tx, ty, &lv_font_montserrat_18, cream);
    cn_ray_point(CN_REF_CX, CN_REF_CY, title_a_ref, title_r, &tx, &ty);
    cn_draw_label_at(layer, "REF", tx, ty, &lv_font_montserrat_18, cream);
}

static const char *cn_range_text(int mult) {
    if (mult == 100) return "x100";
    if (mult == 10)  return "x10";
    return "x1";
}

static void cn_refresh_range_label(void) {
    if (cn_range_lbl == NULL) return;
    lv_label_set_text(cn_range_lbl, cn_range_text(cn_range_mult));
    if (cn_range_btn == NULL) return;
    if (cn_last_alarm) {
        lv_obj_set_style_border_color(cn_range_btn, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_set_style_text_color(cn_range_lbl, lv_palette_main(LV_PALETTE_RED), 0);
    } else if (cn_range_locked) {
        lv_obj_set_style_border_color(cn_range_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_obj_set_style_text_color(cn_range_lbl, lv_palette_main(LV_PALETTE_GREEN), 0);
    } else {
        lv_obj_set_style_border_color(cn_range_btn, lv_color_hex(0x3A3834), 0);
        lv_obj_set_style_text_color(cn_range_lbl, lv_color_hex(0x1C1C1A), 0);
    }
}

static void cn_apply_auto_range(float fwdW) {
    if (cn_range_locked) {
        cn_idle_tick = 0;
        return;
    }
    if (fwdW >= 0.1f) {
        cn_idle_tick = 0;
        int next = cn_range_mult;
        if (cn_range_mult == 1 && fwdW > 11.0f) next = 10;
        else if (cn_range_mult == 10 && fwdW > 110.0f) next = 100;
        if (next != cn_range_mult) {
            cn_range_mult = next;
            cn_refresh_range_label();
        }
        return;
    }
    if (cn_range_mult == 1) {
        cn_idle_tick = 0;
        return;
    }
    if (cn_idle_tick == 0) cn_idle_tick = lv_tick_get();
    if (lv_tick_elaps(cn_idle_tick) >= 1000) {
        cn_range_mult = 1;
        cn_idle_tick = 0;
        cn_refresh_range_label();
    }
}

static void cn_range_clicked(lv_event_t *e) {
    LV_UNUSED(e);
    if (cn_range_long_fired) {
        cn_range_long_fired = false;
        return;
    }
    cn_range_locked = true;
    if (cn_range_mult == 1) cn_range_mult = 10;
    else if (cn_range_mult == 10) cn_range_mult = 100;
    else cn_range_mult = 1;
    cn_refresh_range_label();
}

static void cn_range_long(lv_event_t *e) {
    LV_UNUSED(e);
    cn_range_long_fired = true;
    cn_range_locked = false;
    cn_refresh_range_label();
}

void crossNeedle_resetTracking(void) {
    cn_last_fwd_x = -1;
    cn_last_fwd_y = -1;
    cn_last_ref_x = -1;
    cn_last_ref_y = -1;
    cn_last_alarm = false;
}

void crossNeedle_update(float fwdW, float refW, bool alarm, float alarmSwr) {
    if (cn_needles == NULL) return;
    if (fwdW < 0.0f) fwdW = 0.0f;
    if (refW < 0.0f) refW = 0.0f;
    if (fwdW < 0.1f) refW = 0.0f;

    if (alarmSwr >= 1.1f) cn_swr_alarm = alarmSwr;

    float live_swr = 1.0f;
    if (fwdW >= 0.1f && refW >= 0.0f) {
        float rho = sqrtf(refW / fwdW);
        live_swr = (rho >= 0.98f) ? 99.0f : (1.0f + rho) / (1.0f - rho);
    }
    int band = cn_band_from_swr(live_swr, cn_swr_alarm, cn_ref_band);
    if (band != cn_ref_band) {
        cn_ref_band = band;
        if (cn_face != NULL) lv_obj_invalidate(cn_face);
    }

    cn_apply_auto_range(fwdW);

    float fullF = CN_FWD_FS * (float)cn_range_mult;
    float fullR = CN_REF_FS * (float)cn_range_mult;
    float angF = cn_angle_fwd(fwdW, fullF, true);
    float angR = cn_angle_ref(refW, fullR, true);

    if (fabsf(angF - cn_ang_fwd) > 0.08f || fabsf(angR - cn_ang_ref) > 0.08f) {
        lv_area_t old_a, new_a;
        cn_needle_bbox(CN_FWD_CX, CN_FWD_CY, cn_ang_fwd, &old_a);
        cn_needle_bbox(CN_REF_CX, CN_REF_CY, cn_ang_ref, &new_a);
        lv_obj_invalidate_area(cn_needles, &old_a);
        lv_obj_invalidate_area(cn_needles, &new_a);
        cn_ang_fwd = angF;
        cn_ang_ref = angR;
        cn_needle_bbox(CN_FWD_CX, CN_FWD_CY, cn_ang_fwd, &old_a);
        cn_needle_bbox(CN_REF_CX, CN_REF_CY, cn_ang_ref, &new_a);
        lv_obj_invalidate_area(cn_needles, &old_a);
        lv_obj_invalidate_area(cn_needles, &new_a);
    }

    if (alarm != cn_last_alarm) {
        cn_last_alarm = alarm;
        cn_refresh_range_label();
    }
}

void crossNeedle_build(lv_obj_t *parent) {
    cn_face = NULL;
    cn_needles = NULL;
    cn_range_btn = NULL;
    cn_range_lbl = NULL;
    cn_range_mult = 1;
    cn_range_locked = false;
    cn_range_long_fired = false;
    cn_idle_tick = 0;
    cn_ref_band = 0;
    cn_guides_ready = false;
    crossNeedle_resetTracking();
    cn_build_guides();

    lv_obj_set_style_bg_color(parent, lv_color_hex(0xE4D2A8), 0);

    cn_face = lv_obj_create(parent);
    lv_obj_set_size(cn_face, 480, 320);
    lv_obj_set_pos(cn_face, 0, 0);
    lv_obj_set_style_bg_color(cn_face, lv_color_hex(0xE4D2A8), 0);
    lv_obj_set_style_border_width(cn_face, 0, 0);
    lv_obj_set_style_pad_all(cn_face, 0, 0);
    lv_obj_set_style_radius(cn_face, 0, 0);
    lv_obj_set_scrollbar_mode(cn_face, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(cn_face, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(cn_face, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(cn_face, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_event_cb(cn_face, cn_face_draw_cb, LV_EVENT_DRAW_MAIN_END, NULL);

    cn_ang_fwd = CN_FWD_A0;
    cn_ang_ref = CN_REF_A0;
    cn_needles = lv_obj_create(cn_face);
    lv_obj_set_size(cn_needles, 480, 320);
    lv_obj_set_pos(cn_needles, 0, 0);
    lv_obj_set_style_bg_opa(cn_needles, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cn_needles, 0, 0);
    lv_obj_set_style_pad_all(cn_needles, 0, 0);
    lv_obj_set_style_radius(cn_needles, 0, 0);
    lv_obj_set_scrollbar_mode(cn_needles, LV_SCROLLBAR_MODE_OFF);
    lv_obj_remove_flag(cn_needles, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(cn_needles, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(cn_needles, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_event_cb(cn_needles, cn_needles_draw_cb, LV_EVENT_DRAW_MAIN, NULL);

    cn_range_btn = lv_button_create(parent);
    lv_obj_set_size(cn_range_btn, 70, 34);
    lv_obj_align(cn_range_btn, LV_ALIGN_TOP_RIGHT, -6, 6);
    lv_obj_set_style_bg_opa(cn_range_btn, 0, 0);
    lv_obj_set_style_border_width(cn_range_btn, 3, 0);
    lv_obj_set_style_border_color(cn_range_btn, lv_color_hex(0x3A3834), 0);
    lv_obj_set_style_radius(cn_range_btn, 3, 0);
    lv_obj_set_style_shadow_width(cn_range_btn, 0, 0);
    cn_range_lbl = lv_label_create(cn_range_btn);
    lv_obj_set_style_text_font(cn_range_lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(cn_range_lbl);
    lv_obj_add_event_cb(cn_range_btn, cn_range_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(cn_range_btn, cn_range_long, LV_EVENT_LONG_PRESSED, NULL);
    cn_refresh_range_label();
}

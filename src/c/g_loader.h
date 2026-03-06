#pragma once
#include <pebble.h>

/**
 * G Loader Animation Component
 *
 * Dots snake along a G-shaped path through a 3x3 grid.
 * Works on both color and B&W Pebble displays.
 */

typedef struct GLoaderLayer GLoaderLayer;

GLoaderLayer* g_loader_layer_create(GRect frame);
void g_loader_layer_destroy(GLoaderLayer *loader);
Layer* g_loader_get_layer(GLoaderLayer *loader);
void g_loader_start_animation(GLoaderLayer *loader);
void g_loader_stop_animation(GLoaderLayer *loader);
void g_loader_set_frame(GLoaderLayer *loader, int frame_index);
bool g_loader_is_animating(GLoaderLayer *loader);

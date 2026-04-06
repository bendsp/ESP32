#ifndef SCENE_ANIM_H
#define SCENE_ANIM_H

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "scene_core.h"

void resetTextAnimationState(TextElement& element);
bool blinkPhaseIsVisible(const SceneState& scene, unsigned long nowMs);
bool updateSceneAnimations(SceneState& scene, MatrixPanel_I2S_DMA* matrix, unsigned long nowMs);

#endif

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/test/test_fullscreen_controller_observer.h"

#import "ios/chrome/browser/ui/fullscreen/fullscreen_animator.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void TestFullscreenControllerObserver::FullscreenViewportInsetRangeChanged(
    FullscreenController* controller) {
  min_viewport_insets_ = controller->GetMinViewportInsets();
  max_viewport_insets_ = controller->GetMaxViewportInsets();
  current_viewport_insets_ = controller->GetCurrentViewportInsets();
}

void TestFullscreenControllerObserver::FullscreenProgressUpdated(
    FullscreenController* controller,
    CGFloat progress) {
  progress_ = progress;
}

void TestFullscreenControllerObserver::FullscreenEnabledStateChanged(
    FullscreenController* controller,
    bool enabled) {
  enabled_ = enabled;
}

void TestFullscreenControllerObserver::FullscreenWillAnimate(
    FullscreenController* controller,
    FullscreenAnimator* animator) {
  animator_ = animator;
}

void TestFullscreenControllerObserver::FullscreenControllerWillShutDown(
    FullscreenController* controller) {
  is_shut_down_ = true;
  controller->RemoveObserver(this);
}

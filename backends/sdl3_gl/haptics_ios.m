// Copyright (c) 2026 Wavestorm Software
// iOS haptic feedback for rd::sdl3::play_haptic. Built only for iOS.
#import <UIKit/UIKit.h>

void rd_ios_haptic(int strength) {
    if (strength <= 0) return;
    if (strength == 1) {
        UISelectionFeedbackGenerator* g = [[UISelectionFeedbackGenerator alloc] init];
        [g selectionChanged];
        return;
    }
    UIImpactFeedbackGenerator* g = [[UIImpactFeedbackGenerator alloc]
        initWithStyle:(strength == 2 ? UIImpactFeedbackStyleLight : UIImpactFeedbackStyleMedium)];
    [g impactOccurred];
}

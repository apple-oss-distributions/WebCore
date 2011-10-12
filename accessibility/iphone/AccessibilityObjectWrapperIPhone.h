/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * Permission is granted by Apple to use this file to the extent
 * necessary to relink with LGPL WebKit files.
 *
 * No license or rights are granted by Apple expressly or by
 * implication, estoppel, or otherwise, to Apple patents and
 * trademarks. For the sake of clarity, no license or rights are
 * granted by Apple expressly or by implication, estoppel, or otherwise,
 * under any Apple patents, copyrights and trademarks to underlying
 * implementations of any application programming interfaces (APIs)
 * or to any functionality that is invoked by calling any API.
 */

#ifndef AccessibilityObjectWrapperIPhone_h
#define AccessibilityObjectWrapperIPhone_h


#include "AXObjectCache.h"
#include "AccessibilityObject.h"

@class WAKView;

@interface AccessibilityObjectWrapper : NSObject {
    WebCore::AccessibilityObject* m_object;
    
    // Cached data to avoid frequent re-computation.
    int m_isAccessibilityElement;
    uint64_t m_accessibilityTraitsFromAncestor;
}

- (id)initWithAccessibilityObject:(WebCore::AccessibilityObject*)axObject;
- (void)detach;
- (WebCore::AccessibilityObject*)accessibilityObject;

- (id)accessibilityHitTest:(CGPoint)point;
- (AccessibilityObjectWrapper *)accessibilityPostProcessHitTest:(CGPoint)point;
- (BOOL)accessibilityCanFuzzyHitTest;

- (BOOL)isAccessibilityElement;
- (NSString *)accessibilityLabel;
- (CGRect)accessibilityFrame;
- (NSString *)accessibilityValue;

- (NSInteger)accessibilityElementCount;
- (id)accessibilityElementAtIndex:(NSInteger)index;
- (NSInteger)indexOfAccessibilityElement:(id)element;

- (BOOL)isAttachment;
- (WAKView *)attachmentView;

- (void)postFocusChangeNotification;
- (void)postSelectedTextChangeNotification;
- (void)postLayoutChangeNotification;
- (void)postLiveRegionChangeNotification;
- (void)postLoadCompleteNotification;
- (void)postChildrenChangedNotification;

// Used to inform an element when a notification is posted for it. Used by DRT.
- (void)accessibilityPostedNotification:(WebCore::AXObjectCache::AXNotification)notification;

@end


#endif // AccessibilityObjectWrapperIPhone_h

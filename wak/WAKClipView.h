//
//  WAKClipView.h
//  WebCore
//
//  Copyright 2008, 2009 Apple.  All rights reserved.
//

#ifndef WAKClipView_h
#define WAKClipView_h

#import "WAKView.h"
#import <Foundation/Foundation.h>

@interface WAKClipView : WAKView
{
}
- (void)setDocumentView:(WAKView *)aView;
- (id)documentView;
- (BOOL)copiesOnScroll;
- (void)setCopiesOnScroll:(BOOL)flag;
- (CGRect)documentVisibleRect;
@end

#endif // WAKClipView_h

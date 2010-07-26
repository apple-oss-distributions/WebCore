//
//  NSURLConnectionIPhone.h
//  WebKit
//
//  Copyright (C) 2006, 2007, 2008, Apple Inc. All rights reserved.
//

#import <Foundation/NSURLConnection.h>
#import <Foundation/NSURLConnectionPrivate.h>


@interface NSURLConnectionIPhone : NSObject {
    NSURLRequest *__request;
    id __delegate;
    BOOL __usesCacheFlag;
    long long __maxContentLength;
    BOOL __startImmediatelyFlag;
    NSDictionary *__connectionProperties;
    NSURLConnection *_connection;
}

- (void)start;
-(id)_initWithRequest:(NSURLRequest *)request delegate:(id)delegate usesCache:(BOOL)usesCacheFlag maxContentLength:(long long)maxContentLength startImmediately:(BOOL)startImmediately connectionProperties:(NSDictionary *)connectionProperties;
@end

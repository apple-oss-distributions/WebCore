//
//  MobileQuickLook.h
//  WebCore
//
//  Copyright 2009 Apple Inc. All rights reserved.
//

#ifndef MobileQuickLook_h
#define MobileQuickLook_h


#import "ResourceRequest.h"
#import <wtf/PassOwnPtr.h>

#ifdef __OBJC__
@class NSSet;
@class NSString;
@class NSURL;
#else
class NSSet;
class NSString;
class NSURL;
typedef struct objc_class *Class;
typedef struct objc_object *id;
#endif

namespace WebCore {

    class KURL;
    class String;
    class SubstituteData;

    Class QLPreviewConverterClass();
    NSString *QLTypeCopyBestMimeTypeForFileNameAndMimeType(NSString *fileName, NSString *mimeType);
    NSString *QLTypeCopyBestMimeTypeForURLAndMimeType(NSURL *, NSString *mimeType);

    NSSet *QLPreviewGetSupportedMIMETypesSet();
    
    // Used for setting the permissions on the saved QL content
    NSDictionary *QLFileAttributes();
    NSDictionary *QLDirectoryAttributes();

    void addQLPreviewConverterWithFileForURL(NSURL *, id converter, NSString* content);
    NSString *qlPreviewConverterFileNameForURL(NSURL *);
    NSString *qlPreviewConverterUTIForURL(NSURL *);
    void removeQLPreviewConverterForURL(NSURL *);

    PassOwnPtr<ResourceRequest> registerQLPreviewConverterIfNeeded(const KURL& url, const String& mimeType, const SubstituteData& data);

    const KURL safeQLURLForDocumentURLAndResourceURL(const KURL& documentURL, const String& resourceURL);

    const char* QLPreviewProtocol();

} // namespace WebCore


#endif // MobileQuickLook_h

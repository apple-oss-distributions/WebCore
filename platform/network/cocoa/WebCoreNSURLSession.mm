/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"
#import "WebCoreNSURLSession.h"

#import "CachedResourceRequest.h"
#import "PlatformMediaResourceLoader.h"
#import "SubresourceLoader.h"
#import <wtf/BlockPtr.h>
#import <wtf/CompletionHandler.h>
#import <wtf/cocoa/VectorCocoa.h>

using namespace WebCore;

#pragma mark - Private declarations

NS_ASSUME_NONNULL_BEGIN

static NSDate * __nullable networkLoadMetricsDate(Seconds fetchStart, Seconds delta)
{
    if (!fetchStart.value())
        return nil;
    if (delta.value() == -1)
        return nil;
    return [NSDate dateWithTimeIntervalSince1970:fetchStart.value() + delta.value()];
}

@interface WebCoreNSURLSessionTaskTransactionMetrics : NSObject
- (instancetype)_initWithMetrics:(const WebCore::NetworkLoadMetrics&)metrics;
@property (nullable, copy, readonly) NSDate *fetchStartDate;
@property (nullable, copy, readonly) NSDate *domainLookupStartDate;
@property (nullable, copy, readonly) NSDate *domainLookupEndDate;
@property (nullable, copy, readonly) NSDate *connectStartDate;
@property (nullable, copy, readonly) NSDate *secureConnectionStartDate;
@property (nullable, copy, readonly) NSDate *connectEndDate;
@property (nullable, copy, readonly) NSDate *requestStartDate;
@property (nullable, copy, readonly) NSDate *responseStartDate;
@property (nullable, copy, readonly) NSDate *responseEndDate;
@property (nullable, copy, readonly) NSString *networkProtocolName;
@property (assign, readonly, getter=isReusedConnection) BOOL reusedConnection;
@property (readonly, getter=isCellular) BOOL cellular;
@property (readonly, getter=isExpensive) BOOL expensive;
@property (readonly, getter=isConstrained) BOOL constrained;
@property (readonly, getter=isMultipath) BOOL multipath;
@end

@implementation WebCoreNSURLSessionTaskTransactionMetrics {
    WebCore::NetworkLoadMetrics _metrics;
}

- (instancetype)_initWithMetrics:(const WebCore::NetworkLoadMetrics&)metrics
{
    if (!(self = [super init]))
        return nil;
    _metrics = metrics;
    return self;
}

@dynamic fetchStartDate;
- (nullable NSDate *)fetchStartDate
{
    return networkLoadMetricsDate(_metrics.fetchStart, Seconds(0));
}

@dynamic domainLookupStartDate;
- (nullable NSDate *)domainLookupStartDate
{
    return networkLoadMetricsDate(_metrics.fetchStart, _metrics.domainLookupStart);
}

@dynamic domainLookupEndDate;
- (nullable NSDate *)domainLookupEndDate
{
    return networkLoadMetricsDate(_metrics.fetchStart, _metrics.domainLookupEnd);
}

@dynamic connectStartDate;
- (nullable NSDate *)connectStartDate
{
    return networkLoadMetricsDate(_metrics.fetchStart, _metrics.connectStart);
}

@dynamic secureConnectionStartDate;
- (nullable NSDate *)secureConnectionStartDate
{
    return networkLoadMetricsDate(_metrics.fetchStart, _metrics.secureConnectionStart);
}

@dynamic connectEndDate;
- (nullable NSDate *)connectEndDate
{
    return networkLoadMetricsDate(_metrics.fetchStart, _metrics.connectEnd);
}

@dynamic requestStartDate;
- (nullable NSDate *)requestStartDate
{
    return networkLoadMetricsDate(_metrics.fetchStart, _metrics.requestStart);
}

@dynamic responseStartDate;
- (nullable NSDate *)responseStartDate
{
    return networkLoadMetricsDate(_metrics.fetchStart, _metrics.responseStart);
}

@dynamic responseEndDate;
- (nullable NSDate *)responseEndDate
{
    return networkLoadMetricsDate(_metrics.fetchStart, _metrics.responseEnd);
}

@dynamic networkProtocolName;
- (nullable NSString *)networkProtocolName
{
    return _metrics.protocol;
}

@dynamic reusedConnection;
- (BOOL)isReusedConnection
{
    return _metrics.isReusedConnection;
}

@dynamic cellular;
- (BOOL)cellular
{
    return _metrics.cellular;
}

@dynamic expensive;
- (BOOL)expensive
{
    return _metrics.expensive;
}

@dynamic constrained;
- (BOOL)constrained
{
    return _metrics.constrained;
}

@dynamic multipath;
- (BOOL)multipath
{
    return _metrics.multipath;
}

@end

@interface WebCoreNSURLSessionTaskMetrics : NSObject
- (instancetype)_initWithMetrics:(const WebCore::NetworkLoadMetrics&)metrics;
@property (copy, readonly) NSArray<NSURLSessionTaskTransactionMetrics *> *transactionMetrics;
@end

@implementation WebCoreNSURLSessionTaskMetrics {
    RetainPtr<WebCoreNSURLSessionTaskTransactionMetrics> _transactionMetrics;
}

- (instancetype)_initWithMetrics:(const WebCore::NetworkLoadMetrics&)metrics
{
    if (!(self = [super init]))
        return nil;
    _transactionMetrics = adoptNS([[WebCoreNSURLSessionTaskTransactionMetrics alloc] _initWithMetrics:metrics]);
    return self;
}

@dynamic transactionMetrics;
- (NSArray<NSURLSessionTaskTransactionMetrics *> *)transactionMetrics
{
    return @[ (NSURLSessionTaskTransactionMetrics *)self->_transactionMetrics.get() ];
}

@end

@interface WebCoreNSURLSession ()
@property (readonly) PlatformMediaResourceLoader& loader;
@property (readwrite, retain) id<NSURLSessionTaskDelegate> delegate;
- (void)taskCompleted:(WebCoreNSURLSessionDataTask *)task;
- (void)addDelegateOperation:(Function<void()>&&)operation;
- (void)task:(WebCoreNSURLSessionDataTask *)task didReceiveCORSAccessCheckResult:(BOOL)result;
- (void)task:(WebCoreNSURLSessionDataTask *)task didReceiveResponseFromOrigin:(Ref<WebCore::SecurityOrigin>&&)origin;
@end

@interface WebCoreNSURLSessionDataTask ()
- (id)initWithSession:(WebCoreNSURLSession *)session identifier:(NSUInteger)identifier request:(NSURLRequest *)request;
- (id)initWithSession:(WebCoreNSURLSession *)session identifier:(NSUInteger)identifier URL:(NSURL *)url;
- (void)_restart;
- (void)_cancel;
- (void)_finish;
@property (assign) WebCoreNSURLSession * _Nullable session;

- (void)resource:(PlatformMediaResource&)resource sentBytes:(unsigned long long)bytesSent totalBytesToBeSent:(unsigned long long)totalBytesToBeSent;
- (void)resource:(PlatformMediaResource&)resource receivedResponse:(const ResourceResponse&)response completionHandler:(CompletionHandler<void(ShouldContinuePolicyCheck)>&&)completionHandler;
- (BOOL)resource:(PlatformMediaResource&)resource shouldCacheResponse:(const ResourceResponse&)response;
- (void)resource:(PlatformMediaResource&)resource receivedData:(const char*)data length:(int)length;
- (void)resource:(PlatformMediaResource&)resource receivedRedirect:(const ResourceResponse&)response request:(ResourceRequest&&)request completionHandler:(CompletionHandler<void(ResourceRequest&&)>&&)completionHandler;
- (void)resource:(PlatformMediaResource&)resource accessControlCheckFailedWithError:(const ResourceError&)error;
- (void)resource:(PlatformMediaResource&)resource loadFailedWithError:(const ResourceError&)error;
- (void)resourceFinished:(PlatformMediaResource&)resource metrics:(const NetworkLoadMetrics&)metrics;
@end

NS_ASSUME_NONNULL_END

#pragma mark - WebCoreNSURLSession

@implementation WebCoreNSURLSession
- (id)initWithResourceLoader:(PlatformMediaResourceLoader&)loader delegate:(id<NSURLSessionTaskDelegate>)inDelegate delegateQueue:(NSOperationQueue*)inQueue
{
    self = [super init];
    if (!self)
        return nil;

    ASSERT(_corsResults == WebCoreNSURLSessionCORSAccessCheckResults::Unknown);
    ASSERT(!_invalidated);

    _loader = &loader;
    self.delegate = inDelegate;
    _queue = inQueue ? inQueue : [NSOperationQueue mainQueue];
    _internalQueue = adoptOSObject(dispatch_queue_create("WebCoreNSURLSession _internalQueue", DISPATCH_QUEUE_SERIAL));

    return self;
}

- (void)dealloc
{
    {
        Locker<Lock> locker(_dataTasksLock);
        for (auto& task : _dataTasks)
            [task setSession:nil];
    }

    callOnMainThread([loader = WTFMove(_loader)] {
    });
    [super dealloc];
}

- (id)copyWithZone:(NSZone *)zone
{
    UNUSED_PARAM(zone);
    return [self retain];
}

#pragma mark - Internal Methods

- (void)taskCompleted:(WebCoreNSURLSessionDataTask *)task
{
    task.session = nil;

    {
        Locker<Lock> locker(_dataTasksLock);

        ASSERT(_dataTasks.contains(task));
        _dataTasks.remove(task);
        if (!_dataTasks.isEmpty() || !_invalidated)
            return;
    }

    RetainPtr<WebCoreNSURLSession> strongSelf { self };
    [self addDelegateOperation:[strongSelf] {
        if ([strongSelf.get().delegate respondsToSelector:@selector(URLSession:didBecomeInvalidWithError:)])
            [strongSelf.get().delegate URLSession:(NSURLSession *)strongSelf.get() didBecomeInvalidWithError:nil];
    }];
}

- (void)addDelegateOperation:(Function<void()>&&)function
{
    RetainPtr<WebCoreNSURLSession> strongSelf { self };
    RetainPtr<NSBlockOperation> operation = [NSBlockOperation blockOperationWithBlock:makeBlockPtr(WTFMove(function)).get()];
    dispatch_async(_internalQueue.get(), [strongSelf, operation] {
        [strongSelf.get().delegateQueue addOperation:operation.get()];
        [operation waitUntilFinished];
    });
}

- (void)task:(WebCoreNSURLSessionDataTask *)task didReceiveCORSAccessCheckResult:(BOOL)result
{
    UNUSED_PARAM(task);
    if (!result)
        _corsResults = WebCoreNSURLSessionCORSAccessCheckResults::Fail;
    else if (_corsResults != WebCoreNSURLSessionCORSAccessCheckResults::Fail)
        _corsResults = WebCoreNSURLSessionCORSAccessCheckResults::Pass;
}

- (void)task:(WebCoreNSURLSessionDataTask *)task didReceiveResponseFromOrigin:(Ref<WebCore::SecurityOrigin>&&)origin
{
    UNUSED_PARAM(task);
    _origins.add(WTFMove(origin));
}

#pragma mark - NSURLSession API
@dynamic delegate;
- (__nullable id<NSURLSessionDelegate>)delegate
{
    return _delegate.getAutoreleased();
}

- (void)setDelegate:(id<NSURLSessionDelegate>)delegate
{
    _delegate = delegate;
}

@dynamic delegateQueue;
- (NSOperationQueue *)delegateQueue
{
    return _queue.get();
}

@dynamic configuration;
- (NSURLSessionConfiguration *)configuration
{
    return nil;
}

- (NSString *)sessionDescription
{
    return _sessionDescription.get();
}

- (void)setSessionDescription:(NSString *)sessionDescription
{
    _sessionDescription = adoptNS([sessionDescription copy]);
}

@dynamic loader;
- (PlatformMediaResourceLoader&)loader
{
    return *_loader;
}

@dynamic didPassCORSAccessChecks;
- (BOOL)didPassCORSAccessChecks
{
    return _corsResults == WebCoreNSURLSessionCORSAccessCheckResults::Pass;
}

- (BOOL)wouldTaintOrigin:(const WebCore::SecurityOrigin &)origin
{
    for (auto& responseOrigin : _origins) {
        if (!origin.canAccess(*responseOrigin))
            return true;
    }
    return false;
}

- (void)finishTasksAndInvalidate
{
    _invalidated = YES;
    {
        Locker<Lock> locker(_dataTasksLock);
        if (!_dataTasks.isEmpty())
            return;
    }

    [self addDelegateOperation:[strongSelf = retainPtr(self)] {
        auto delegate = strongSelf.get().delegate;
        if ([delegate respondsToSelector:@selector(URLSession:didBecomeInvalidWithError:)])
            [delegate URLSession:(NSURLSession *)strongSelf.get() didBecomeInvalidWithError:nil];
    }];
}

- (void)invalidateAndCancel
{
    Vector<RetainPtr<WebCoreNSURLSessionDataTask>> tasksCopy;
    {
        Locker<Lock> locker(_dataTasksLock);
        tasksCopy = copyToVector(_dataTasks);
    }

    for (auto& task : tasksCopy)
        [task cancel];

    [self finishTasksAndInvalidate];
}

- (void)resetWithCompletionHandler:(void (^)(void))completionHandler
{
    // FIXME: This cannot currently be implemented. We cannot guarantee that the next connection will happen on a new socket.
    [self addDelegateOperation:[completionHandler = BlockPtr<void()>(completionHandler)] {
        completionHandler();
    }];
}

- (void)flushWithCompletionHandler:(void (^)(void))completionHandler
{
    // FIXME: This cannot currently be implemented. We cannot guarantee that the next connection will happen on a new socket.
    [self addDelegateOperation:[completionHandler = BlockPtr<void()>(completionHandler)] {
        completionHandler();
    }];
}

- (void)getTasksWithCompletionHandler:(void (^)(NSArray<NSURLSessionDataTask *> *dataTasks, NSArray<NSURLSessionUploadTask *> *uploadTasks, NSArray<NSURLSessionDownloadTask *> *downloadTasks))completionHandler
{
    RetainPtr<NSArray> array;
    {
        Locker<Lock> locker(_dataTasksLock);
        array = createNSArray(_dataTasks, [] (auto& task) {
            return task.get();
        });
    }
    [self addDelegateOperation:^{
        completionHandler(array.get(), nil, nil);
    }];
}

- (void)getAllTasksWithCompletionHandler:(void (^)(NSArray<__kindof NSURLSessionTask *> *tasks))completionHandler
{
    RetainPtr<NSArray> array;
    {
        Locker<Lock> locker(_dataTasksLock);
        array = createNSArray(_dataTasks, [] (auto& task) {
            return task.get();
        });
    }
    [self addDelegateOperation:^{
        completionHandler(array.get());
    }];
}

- (NSURLSessionDataTask *)dataTaskWithRequest:(NSURLRequest *)request
{
    if (_invalidated)
        return nil;

    WebCoreNSURLSessionDataTask *task = [[WebCoreNSURLSessionDataTask alloc] initWithSession:self identifier:_nextTaskIdentifier++ request:request];
    {
        Locker<Lock> locker(_dataTasksLock);
        _dataTasks.add(task);
    }
    return (NSURLSessionDataTask *)[task autorelease];
}

- (NSURLSessionDataTask *)dataTaskWithURL:(NSURL *)url
{
    if (_invalidated)
        return nil;

    WebCoreNSURLSessionDataTask *task = [[WebCoreNSURLSessionDataTask alloc] initWithSession:self identifier:_nextTaskIdentifier++ URL:url];
    {
        Locker<Lock> locker(_dataTasksLock);
        _dataTasks.add(task);
    }
    return (NSURLSessionDataTask *)[task autorelease];
}

- (void)sendH2Ping:(NSURL *)url pongHandler:(void (^)(NSError *error, NSTimeInterval interval))pongHandler
{
    callOnMainThread([self, strongSelf = retainPtr(self), url = retainPtr(url), pongHandler = makeBlockPtr(pongHandler)] () mutable {

        if (_invalidated)
            return pongHandler(adoptNS([[NSError alloc] initWithDomain:NSURLErrorDomain code:NSURLErrorUnknown userInfo:nil]).get(), 0);

        self.loader.sendH2Ping(url.get(), [self, strongSelf = WTFMove(strongSelf), pongHandler = WTFMove(pongHandler)] (Expected<Seconds, ResourceError>&& result) mutable {
            NSTimeInterval interval = 0;
            RetainPtr<NSError> error;
            if (result)
                interval = result.value().value();
            else
                error = result.error();
            [self addDelegateOperation:[pongHandler = WTFMove(pongHandler), error = WTFMove(error), interval] {
                pongHandler(error.get(), interval);
            }];
        });
    });
}

- (NSURLSessionUploadTask *)uploadTaskWithRequest:(NSURLRequest *)request fromFile:(NSURL *)fileURL
{
    UNUSED_PARAM(request);
    UNUSED_PARAM(fileURL);
    return nil;
}

- (NSURLSessionUploadTask *)uploadTaskWithRequest:(NSURLRequest *)request fromData:(NSData *)bodyData
{
    UNUSED_PARAM(request);
    UNUSED_PARAM(bodyData);
    return nil;
}

- (NSURLSessionUploadTask *)uploadTaskWithStreamedRequest:(NSURLRequest *)request
{
    UNUSED_PARAM(request);
    return nil;
}

- (NSURLSessionDownloadTask *)downloadTaskWithRequest:(NSURLRequest *)request
{
    UNUSED_PARAM(request);
    return nil;
}

- (NSURLSessionDownloadTask *)downloadTaskWithURL:(NSURL *)url
{
    UNUSED_PARAM(url);
    return nil;
}

- (NSURLSessionDownloadTask *)downloadTaskWithResumeData:(NSData *)resumeData
{
    UNUSED_PARAM(resumeData);
    return nil;
}

- (NSURLSessionStreamTask *)streamTaskWithHostName:(NSString *)hostname port:(NSInteger)port
{
    UNUSED_PARAM(hostname);
    UNUSED_PARAM(port);
    return nil;
}

- (NSURLSessionStreamTask *)streamTaskWithNetService:(NSNetService *)service
{
    UNUSED_PARAM(service);
    return nil;
}

- (BOOL)isKindOfClass:(Class)aClass
{
    if (aClass == [NSURLSession class])
        return YES;
    return [super isKindOfClass:aClass];
}
@end

#pragma mark - WebCoreNSURLSessionDataTaskClient

namespace WebCore {

class WebCoreNSURLSessionDataTaskClient : public PlatformMediaResourceClient {
    WTF_MAKE_FAST_ALLOCATED;
public:
    WebCoreNSURLSessionDataTaskClient(WebCoreNSURLSessionDataTask *task)
        : m_task(task)
    {
    }

    void clearTask();

    void responseReceived(PlatformMediaResource&, const ResourceResponse&, CompletionHandler<void(ShouldContinuePolicyCheck)>&&) override;
    void redirectReceived(PlatformMediaResource&, ResourceRequest&&, const ResourceResponse&, CompletionHandler<void(ResourceRequest&&)>&&) override;
    bool shouldCacheResponse(PlatformMediaResource&, const ResourceResponse&) override;
    void dataSent(PlatformMediaResource&, unsigned long long, unsigned long long) override;
    void dataReceived(PlatformMediaResource&, const char* /* data */, int /* length */) override;
    void accessControlCheckFailed(PlatformMediaResource&, const ResourceError&) override;
    void loadFailed(PlatformMediaResource&, const ResourceError&) override;
    void loadFinished(PlatformMediaResource&, const NetworkLoadMetrics&) override;

private:
    Lock m_taskLock;
    WebCoreNSURLSessionDataTask *m_task;
};

void WebCoreNSURLSessionDataTaskClient::clearTask()
{
    LockHolder locker(m_taskLock);
    m_task = nullptr;
}

void WebCoreNSURLSessionDataTaskClient::dataSent(PlatformMediaResource& resource, unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    LockHolder locker(m_taskLock);
    if (!m_task)
        return;

    [m_task resource:resource sentBytes:bytesSent totalBytesToBeSent:totalBytesToBeSent];
}

void WebCoreNSURLSessionDataTaskClient::responseReceived(PlatformMediaResource& resource, const ResourceResponse& response, CompletionHandler<void(ShouldContinuePolicyCheck)>&& completionHandler)
{
    LockHolder locker(m_taskLock);
    if (!m_task)
        return completionHandler(ShouldContinuePolicyCheck::No);

    [m_task resource:resource receivedResponse:response completionHandler:WTFMove(completionHandler)];
}

bool WebCoreNSURLSessionDataTaskClient::shouldCacheResponse(PlatformMediaResource& resource, const ResourceResponse& response)
{
    LockHolder locker(m_taskLock);
    if (!m_task)
        return false;

    return [m_task resource:resource shouldCacheResponse:response];
}

void WebCoreNSURLSessionDataTaskClient::dataReceived(PlatformMediaResource& resource, const char* data, int length)
{
    LockHolder locker(m_taskLock);
    if (!m_task)
        return;

    [m_task resource:resource receivedData:data length:length];
}

void WebCoreNSURLSessionDataTaskClient::redirectReceived(PlatformMediaResource& resource, ResourceRequest&& request, const ResourceResponse& response, CompletionHandler<void(ResourceRequest&&)>&& completionHandler)
{
    LockHolder locker(m_taskLock);
    if (!m_task)
        return;

    [m_task resource:resource receivedRedirect:response request:WTFMove(request) completionHandler: [completionHandler = WTFMove(completionHandler)] (auto&& request) mutable {
        callOnMainThread([request = request.isolatedCopy(), completionHandler = WTFMove(completionHandler)] () mutable {
            completionHandler(WTFMove(request));
        });
    }];
}

void WebCoreNSURLSessionDataTaskClient::accessControlCheckFailed(PlatformMediaResource& resource, const ResourceError& error)
{
    LockHolder locker(m_taskLock);
    if (!m_task)
        return;

    [m_task resource:resource accessControlCheckFailedWithError:error];
}

void WebCoreNSURLSessionDataTaskClient::loadFailed(PlatformMediaResource& resource, const ResourceError& error)
{
    LockHolder locker(m_taskLock);
    if (!m_task)
        return;

    [m_task resource:resource loadFailedWithError:error];
}

void WebCoreNSURLSessionDataTaskClient::loadFinished(PlatformMediaResource& resource, const NetworkLoadMetrics& metrics)
{
    LockHolder locker(m_taskLock);
    if (!m_task)
        return;

    [m_task resourceFinished:resource metrics:metrics];
}

}

#pragma mark - WebCoreNSURLSessionDataTask

@implementation WebCoreNSURLSessionDataTask
- (id)initWithSession:(WebCoreNSURLSession *)session identifier:(NSUInteger)identifier URL:(NSURL *)url
{
    self.taskIdentifier = identifier;
    self.session = session;
    self.state = NSURLSessionTaskStateSuspended;
    self.priority = NSURLSessionTaskPriorityDefault;
    self.originalRequest = self.currentRequest = [NSURLRequest requestWithURL:url];

    return self;
}

- (id)initWithSession:(WebCoreNSURLSession *)session identifier:(NSUInteger)identifier request:(NSURLRequest *)request
{
    self.taskIdentifier = identifier;
    self.session = session;
    self.state = NSURLSessionTaskStateSuspended;
    self.priority = NSURLSessionTaskPriorityDefault;

    // CoreMedia will explicitly add a user agent header. Remove if present.
    if (auto* userAgentValue = [request valueForHTTPHeaderField:@"User-Agent"]) {
        NSMutableURLRequest* mutableRequest = [request mutableCopyWithZone:nil];
        [mutableRequest setValue:nil forHTTPHeaderField:@"User-Agent"];
        request = [mutableRequest autorelease];
    }

    self.originalRequest = self.currentRequest = request;

    return self;
}

- (id)copyWithZone:(NSZone *)zone
{
    UNUSED_PARAM(zone);
    return [self retain];
}

#pragma mark - Internal methods

- (void)_restart
{
    ASSERT(isMainThread());

    if (!self.session)
        return;

    [self _cancel];

    _resource = self.session.loader.requestResource(self.originalRequest, PlatformMediaResourceLoader::LoadOption::DisallowCaching);
    if (_resource)
        _resource->setClient(makeUnique<WebCoreNSURLSessionDataTaskClient>(self));
}

- (void)_cancel
{
    ASSERT(isMainThread());
    if (_resource) {
        _resource->stop();
        _resource->setClient(nullptr);
        _resource = nil;
    }
}

- (void)_finish
{
    ASSERT(isMainThread());
    if (_resource)
        [self resourceFinished:*_resource metrics:NetworkLoadMetrics { }];
}

#pragma mark - NSURLSession API
@synthesize session = _session;
@synthesize taskIdentifier = _taskIdentifier;
@synthesize originalRequest = _originalRequest;
@synthesize currentRequest = _currentRequest;
@synthesize countOfBytesReceived = _countOfBytesReceived;
@synthesize countOfBytesSent = _countOfBytesSent;
@synthesize countOfBytesExpectedToSend = _countOfBytesExpectedToSend;
@synthesize countOfBytesExpectedToReceive = _countOfBytesExpectedToReceive;
@synthesize state = _state;
@synthesize error = _error;
@synthesize taskDescription = _taskDescription;
@synthesize priority = _priority;

- (NSURLResponse *)response
{
    return _response.get();
}

- (void)cancel
{
    self.state = NSURLSessionTaskStateCanceling;
    callOnMainThread([protectedSelf = RetainPtr<WebCoreNSURLSessionDataTask>(self)] {
        [protectedSelf _cancel];
        [protectedSelf _finish];
    });
}

- (void)suspend
{
    callOnMainThread([protectedSelf = RetainPtr<WebCoreNSURLSessionDataTask>(self)] {
        // NSURLSessionDataTasks must start over after suspending, so while
        // we could defer loading at this point, instead cancel and restart
        // upon resume so as to adhere to NSURLSessionDataTask semantics.
        [protectedSelf _cancel];
        protectedSelf.get().state = NSURLSessionTaskStateSuspended;
    });
}

- (void)resume
{
    callOnMainThread([protectedSelf = RetainPtr<WebCoreNSURLSessionDataTask>(self)] {
        if (protectedSelf.get().state != NSURLSessionTaskStateSuspended)
            return;

        [protectedSelf _restart];
        protectedSelf.get().state = NSURLSessionTaskStateRunning;
    });
}

- (void)dealloc
{
    [_originalRequest release];
    [_currentRequest release];
    [_error release];
    [_taskDescription release];

    if (!isMainThread() && _resource) {
        if (auto* client = _resource->client())
            static_cast<WebCoreNSURLSessionDataTaskClient*>(client)->clearTask();
        callOnMainThread([resource = WTFMove(_resource)] { });
    }

    [super dealloc];
}

#pragma mark - NSURLSession SPI

- (NSDictionary *)_timingData
{
    // FIXME: return a dictionary sourced from ResourceHandle::getConnectionTimingData().
    return @{ };
}

#pragma mark - PlatformMediaResourceClient callbacks

- (void)resource:(PlatformMediaResource&)resource sentBytes:(unsigned long long)bytesSent totalBytesToBeSent:(unsigned long long)totalBytesToBeSent
{
    ASSERT_UNUSED(resource, &resource == _resource);
    UNUSED_PARAM(bytesSent);
    UNUSED_PARAM(totalBytesToBeSent);
    // No-op.
}

- (void)resource:(PlatformMediaResource&)resource receivedResponse:(const ResourceResponse&)response completionHandler:(CompletionHandler<void(ShouldContinuePolicyCheck)>&&)completionHandler
{
    ASSERT(response.source() == ResourceResponse::Source::Network || response.source() == ResourceResponse::Source::DiskCache || response.source() == ResourceResponse::Source::DiskCacheAfterValidation || response.source() == ResourceResponse::Source::ServiceWorker);
    ASSERT_UNUSED(resource, &resource == _resource);
    ASSERT(isMainThread());
    [self.session task:self didReceiveResponseFromOrigin:SecurityOrigin::create(response.url())];
    [self.session task:self didReceiveCORSAccessCheckResult:resource.didPassAccessControlCheck()];
    self.countOfBytesExpectedToReceive = response.expectedContentLength();
    RetainPtr<NSURLResponse> strongResponse { response.nsURLResponse() };
    RetainPtr<WebCoreNSURLSessionDataTask> strongSelf { self };
    if (!self.session)
        return completionHandler(ShouldContinuePolicyCheck::No);
    [self.session addDelegateOperation:[strongSelf, strongResponse, completionHandler = WTFMove(completionHandler)] () mutable {
        strongSelf->_response = strongResponse.get();

        id<NSURLSessionDataDelegate> dataDelegate = (id<NSURLSessionDataDelegate>)strongSelf.get().session.delegate;
        if (![dataDelegate respondsToSelector:@selector(URLSession:dataTask:didReceiveResponse:completionHandler:)]) {
            callOnMainThread([strongSelf, completionHandler = WTFMove(completionHandler)] () mutable {
                completionHandler(ShouldContinuePolicyCheck::Yes);
            });
            return;
        }

        [dataDelegate URLSession:(NSURLSession *)strongSelf.get().session dataTask:(NSURLSessionDataTask *)strongSelf.get() didReceiveResponse:strongResponse.get() completionHandler:makeBlockPtr([strongSelf, completionHandler = WTFMove(completionHandler)] (NSURLSessionResponseDisposition disposition) mutable {
            callOnMainThread([strongSelf, disposition, completionHandler = WTFMove(completionHandler)] () mutable {
                if (disposition == NSURLSessionResponseCancel)
                    completionHandler(ShouldContinuePolicyCheck::No);
                else {
                    ASSERT(disposition == NSURLSessionResponseAllow);
                    completionHandler(ShouldContinuePolicyCheck::Yes);
                }
            });
        }).get()];
    }];
}

- (BOOL)resource:(PlatformMediaResource&)resource shouldCacheResponse:(const ResourceResponse&)response
{
    ASSERT_UNUSED(resource, &resource == _resource);

    ASSERT(isMainThread());

    // FIXME: remove if <rdar://problem/20001985> is ever resolved.
    return response.httpHeaderField(HTTPHeaderName::ContentRange).isEmpty();
}

- (void)resource:(PlatformMediaResource&)resource receivedData:(const char*)data length:(int)length
{
    ASSERT_UNUSED(resource, &resource == _resource);
    RetainPtr<NSData> nsData = adoptNS([[NSData alloc] initWithBytes:data length:length]);
    RetainPtr<WebCoreNSURLSessionDataTask> strongSelf { self };
    [self.session addDelegateOperation:[strongSelf, length, nsData] {
        strongSelf.get().countOfBytesReceived += length;
        id<NSURLSessionDataDelegate> dataDelegate = (id<NSURLSessionDataDelegate>)strongSelf.get().session.delegate;
        if ([dataDelegate respondsToSelector:@selector(URLSession:dataTask:didReceiveData:)])
            [dataDelegate URLSession:(NSURLSession *)strongSelf.get().session dataTask:(NSURLSessionDataTask *)strongSelf.get() didReceiveData:nsData.get()];
    }];
}

- (void)resource:(PlatformMediaResource&)resource receivedRedirect:(const ResourceResponse&)response request:(ResourceRequest&&)request completionHandler:(CompletionHandler<void(ResourceRequest&&)>&&)completionHandler
{
    ASSERT_UNUSED(resource, &resource == _resource);
    [self.session addDelegateOperation:[strongSelf = retainPtr(self), response = retainPtr(response.nsURLResponse()), request = request.isolatedCopy(), completionHandler = WTFMove(completionHandler)] () mutable {
        if (![response isKindOfClass:[NSHTTPURLResponse class]]) {
            ASSERT_NOT_REACHED();
            callOnMainThread([request = WTFMove(request), completionHandler = WTFMove(completionHandler)] () mutable {
                completionHandler(WTFMove(request));
            });
            return;
        }
        
        id<NSURLSessionDataDelegate> dataDelegate = (id<NSURLSessionDataDelegate>)strongSelf.get().session.delegate;
        if ([dataDelegate respondsToSelector:@selector(URLSession:task:willPerformHTTPRedirection:newRequest:completionHandler:)]) {
            auto completionHandlerBlock = makeBlockPtr([completionHandler = WTFMove(completionHandler)](NSURLRequest *newRequest) mutable {
                if (!isMainThread()) {
                    callOnMainThread([request = ResourceRequest { newRequest }, completionHandler = WTFMove(completionHandler)] () mutable {
                        completionHandler(WTFMove(request));
                    });
                    return;
                }
                completionHandler(newRequest);
            });
            [dataDelegate URLSession:(NSURLSession *)strongSelf.get().session task:(NSURLSessionTask *)strongSelf.get() willPerformHTTPRedirection:(NSHTTPURLResponse *)response.get() newRequest:request.nsURLRequest(HTTPBodyUpdatePolicy::DoNotUpdateHTTPBody) completionHandler:completionHandlerBlock.get()];
        } else {
            callOnMainThread([request = WTFMove(request), completionHandler = WTFMove(completionHandler)] () mutable {
                completionHandler(WTFMove(request));
            });
        }
    }];
}

- (void)_resource:(PlatformMediaResource&)resource loadFinishedWithError:(NSError *)error metrics:(const NetworkLoadMetrics&)metrics
{
    ASSERT_UNUSED(resource, &resource == _resource);
    if (self.state == NSURLSessionTaskStateCompleted)
        return;
    self.state = NSURLSessionTaskStateCompleted;

    RetainPtr<WebCoreNSURLSessionDataTask> strongSelf { self };
    RetainPtr<WebCoreNSURLSession> strongSession { self.session };
    RetainPtr<NSError> strongError { error };
    [self.session addDelegateOperation:[strongSelf, strongSession, strongError, metrics = metrics.isolatedCopy()] {
        id<NSURLSessionTaskDelegate> delegate = (id<NSURLSessionTaskDelegate>)strongSession.get().delegate;

        if ([delegate respondsToSelector:@selector(URLSession:task:didFinishCollectingMetrics:)])
            [delegate URLSession:(NSURLSession *)strongSession.get() task:(NSURLSessionDataTask *)strongSelf.get() didFinishCollectingMetrics:(NSURLSessionTaskMetrics *)adoptNS([[WebCoreNSURLSessionTaskMetrics alloc] _initWithMetrics:metrics]).get()];

        if ([delegate respondsToSelector:@selector(URLSession:task:didCompleteWithError:)])
            [delegate URLSession:(NSURLSession *)strongSession.get() task:(NSURLSessionDataTask *)strongSelf.get() didCompleteWithError:strongError.get()];

        callOnMainThread([strongSelf, strongSession] {
            [strongSession taskCompleted:strongSelf.get()];
        });
    }];
}

- (void)resource:(PlatformMediaResource&)resource accessControlCheckFailedWithError:(const ResourceError&)error
{
    [self _resource:resource loadFinishedWithError:error.nsError() metrics:NetworkLoadMetrics { }];
}

- (void)resource:(PlatformMediaResource&)resource loadFailedWithError:(const ResourceError&)error
{
    [self _resource:resource loadFinishedWithError:error.nsError() metrics:NetworkLoadMetrics { }];
}

- (void)resourceFinished:(PlatformMediaResource&)resource metrics:(const NetworkLoadMetrics&)metrics
{
    [self _resource:resource loadFinishedWithError:nil metrics:metrics];
}
@end

/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#include "config.h"
#include "CrossOriginAccessControl.h"

#include "CachedResourceRequest.h"
#include "CrossOriginPreflightResultCache.h"
#include "HTTPHeaderNames.h"
#include "HTTPParsers.h"
#include "LegacySchemeRegistry.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"
#include <mutex>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/AtomString.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

bool isOnAccessControlSimpleRequestMethodWhitelist(const String& method)
{
    return method == "GET" || method == "HEAD" || method == "POST";
}

bool isSimpleCrossOriginAccessRequest(const String& method, const HTTPHeaderMap& headerMap)
{
    if (!isOnAccessControlSimpleRequestMethodWhitelist(method))
        return false;

    for (const auto& header : headerMap) {
        if (!header.keyAsHTTPHeaderName || !isCrossOriginSafeRequestHeader(header.keyAsHTTPHeaderName.value(), header.value))
            return false;
    }

    return true;
}

void updateRequestReferrer(ResourceRequest& request, ReferrerPolicy referrerPolicy, const String& outgoingReferrer)
{
    String newOutgoingReferrer = SecurityPolicy::generateReferrerHeader(referrerPolicy, request.url(), outgoingReferrer);
    if (newOutgoingReferrer.isEmpty())
        request.clearHTTPReferrer();
    else
        request.setHTTPReferrer(newOutgoingReferrer);
}

void updateRequestForAccessControl(ResourceRequest& request, SecurityOrigin& securityOrigin, StoredCredentialsPolicy storedCredentialsPolicy)
{
    request.removeCredentials();
    request.setAllowCookies(storedCredentialsPolicy == StoredCredentialsPolicy::Use);
    request.setHTTPOrigin(securityOrigin.toString());
}

ResourceRequest createAccessControlPreflightRequest(const ResourceRequest& request, SecurityOrigin& securityOrigin, const String& referrer)
{
    ResourceRequest preflightRequest(request.url());
    static const double platformDefaultTimeout = 0;
    preflightRequest.setTimeoutInterval(platformDefaultTimeout);
    updateRequestForAccessControl(preflightRequest, securityOrigin, StoredCredentialsPolicy::DoNotUse);
    preflightRequest.setHTTPMethod("OPTIONS");
    preflightRequest.setHTTPHeaderField(HTTPHeaderName::AccessControlRequestMethod, request.httpMethod());
    preflightRequest.setPriority(request.priority());
    if (!referrer.isNull())
        preflightRequest.setHTTPReferrer(referrer);

    const HTTPHeaderMap& requestHeaderFields = request.httpHeaderFields();

    if (!requestHeaderFields.isEmpty()) {
        Vector<String> unsafeHeaders;
        for (auto& headerField : requestHeaderFields) {
            if (!headerField.keyAsHTTPHeaderName || !isCrossOriginSafeRequestHeader(*headerField.keyAsHTTPHeaderName, headerField.value))
                unsafeHeaders.append(headerField.key.convertToASCIILowercase());
        }

        std::sort(unsafeHeaders.begin(), unsafeHeaders.end(), WTF::codePointCompareLessThan);

        StringBuilder headerBuffer;

        bool appendComma = false;
        for (const auto& headerField : unsafeHeaders) {
            if (appendComma)
                headerBuffer.append(',');
            else
                appendComma = true;

            headerBuffer.append(headerField);
        }
        if (!headerBuffer.isEmpty())
            preflightRequest.setHTTPHeaderField(HTTPHeaderName::AccessControlRequestHeaders, headerBuffer.toString());
    }

    return preflightRequest;
}

// https://html.spec.whatwg.org/multipage/urls-and-fetching.html#create-a-potential-cors-request
CachedResourceRequest createPotentialAccessControlRequest(ResourceRequest&& request, ResourceLoaderOptions&& options, Document& document, const String& crossOriginAttribute, SameOriginFlag sameOriginFlag)
{
    ASSERT(options.mode == FetchOptions::Mode::NoCors);
    if (!crossOriginAttribute.isNull())
        options.mode = FetchOptions::Mode::Cors;
    else if (sameOriginFlag == SameOriginFlag::Yes)
        options.mode = FetchOptions::Mode::SameOrigin;

    if (crossOriginAttribute.isNull()) {
        CachedResourceRequest cachedRequest { WTFMove(request), WTFMove(options) };
        cachedRequest.setOrigin(document.securityOrigin());
        return cachedRequest;
    }

    FetchOptions::Credentials credentials = equalLettersIgnoringASCIICase(crossOriginAttribute, "omit")
        ? FetchOptions::Credentials::Omit : equalLettersIgnoringASCIICase(crossOriginAttribute, "use-credentials")
        ? FetchOptions::Credentials::Include : FetchOptions::Credentials::SameOrigin;
    options.credentials = credentials;
    options.storedCredentialsPolicy = credentials == FetchOptions::Credentials::Include ? StoredCredentialsPolicy::Use : StoredCredentialsPolicy::DoNotUse;

    CachedResourceRequest cachedRequest { WTFMove(request), WTFMove(options) };
    updateRequestForAccessControl(cachedRequest.resourceRequest(), document.securityOrigin(), options.storedCredentialsPolicy);
    return cachedRequest;
}

String validateCrossOriginRedirectionURL(const URL& redirectURL)
{
    if (!LegacySchemeRegistry::shouldTreatURLSchemeAsCORSEnabled(redirectURL.protocol().toStringWithoutCopying()))
        return makeString("not allowed to follow a cross-origin CORS redirection with non CORS scheme");

    if (redirectURL.hasUsername() || redirectURL.hasPassword())
        return makeString("redirection URL ", redirectURL.string(), " has credentials");

    return { };
}

OptionSet<HTTPHeadersToKeepFromCleaning> httpHeadersToKeepFromCleaning(const HTTPHeaderMap& headers)
{
    OptionSet<HTTPHeadersToKeepFromCleaning> headersToKeep;
    if (headers.contains(HTTPHeaderName::ContentType))
        headersToKeep.add(HTTPHeadersToKeepFromCleaning::ContentType);
    if (headers.contains(HTTPHeaderName::Referer))
        headersToKeep.add(HTTPHeadersToKeepFromCleaning::Referer);
    if (headers.contains(HTTPHeaderName::Origin))
        headersToKeep.add(HTTPHeadersToKeepFromCleaning::Origin);
    if (headers.contains(HTTPHeaderName::UserAgent))
        headersToKeep.add(HTTPHeadersToKeepFromCleaning::UserAgent);
    if (headers.contains(HTTPHeaderName::AcceptEncoding))
        headersToKeep.add(HTTPHeadersToKeepFromCleaning::AcceptEncoding);
    return headersToKeep;
}

void cleanHTTPRequestHeadersForAccessControl(ResourceRequest& request, OptionSet<HTTPHeadersToKeepFromCleaning> headersToKeep)
{
    // Remove headers that may have been added by the network layer that cause access control to fail.
    if (!headersToKeep.contains(HTTPHeadersToKeepFromCleaning::ContentType)) {
        auto contentType = request.httpContentType();
        if (!contentType.isNull() && !isCrossOriginSafeRequestHeader(HTTPHeaderName::ContentType, contentType))
            request.clearHTTPContentType();
    }
    if (!headersToKeep.contains(HTTPHeadersToKeepFromCleaning::Referer))
        request.clearHTTPReferrer();
    if (!headersToKeep.contains(HTTPHeadersToKeepFromCleaning::Origin))
        request.clearHTTPOrigin();
    if (!headersToKeep.contains(HTTPHeadersToKeepFromCleaning::UserAgent))
        request.clearHTTPUserAgent();
    if (!headersToKeep.contains(HTTPHeadersToKeepFromCleaning::AcceptEncoding))
        request.clearHTTPAcceptEncoding();
}

bool passesAccessControlCheck(const ResourceResponse& response, StoredCredentialsPolicy storedCredentialsPolicy, SecurityOrigin& securityOrigin, String& errorDescription)
{
    // A wildcard Access-Control-Allow-Origin can not be used if credentials are to be sent,
    // even with Access-Control-Allow-Credentials set to true.
    const String& accessControlOriginString = response.httpHeaderField(HTTPHeaderName::AccessControlAllowOrigin);
    if (accessControlOriginString == "*" && storedCredentialsPolicy == StoredCredentialsPolicy::DoNotUse)
        return true;

    String securityOriginString = securityOrigin.toString();
    if (accessControlOriginString != securityOriginString) {
        if (accessControlOriginString == "*")
            errorDescription = "Cannot use wildcard in Access-Control-Allow-Origin when credentials flag is true."_s;
        else if (accessControlOriginString.find(',') != notFound)
            errorDescription = "Access-Control-Allow-Origin cannot contain more than one origin."_s;
        else
            errorDescription = makeString("Origin ", securityOriginString, " is not allowed by Access-Control-Allow-Origin.");
        return false;
    }

    if (storedCredentialsPolicy == StoredCredentialsPolicy::Use) {
        const String& accessControlCredentialsString = response.httpHeaderField(HTTPHeaderName::AccessControlAllowCredentials);
        if (accessControlCredentialsString != "true") {
            errorDescription = "Credentials flag is true, but Access-Control-Allow-Credentials is not \"true\".";
            return false;
        }
    }

    return true;
}

bool validatePreflightResponse(const ResourceRequest& request, const ResourceResponse& response, StoredCredentialsPolicy storedCredentialsPolicy, SecurityOrigin& securityOrigin, String& errorDescription)
{
    if (!response.isSuccessful()) {
        errorDescription = "Preflight response is not successful"_s;
        return false;
    }

    if (!passesAccessControlCheck(response, storedCredentialsPolicy, securityOrigin, errorDescription))
        return false;

    auto result = makeUnique<CrossOriginPreflightResultCacheItem>(storedCredentialsPolicy);
    if (!result->parse(response)
        || !result->allowsCrossOriginMethod(request.httpMethod(), storedCredentialsPolicy, errorDescription)
        || !result->allowsCrossOriginHeaders(request.httpHeaderFields(), storedCredentialsPolicy, errorDescription)) {
        return false;
    }

    CrossOriginPreflightResultCache::singleton().appendEntry(securityOrigin.toString(), request.url(), WTFMove(result));
    return true;
}

static inline bool shouldCrossOriginResourcePolicyCancelLoad(const SecurityOrigin& origin, const ResourceResponse& response)
{
    if (origin.canRequest(response.url()))
        return false;

    auto policy = parseCrossOriginResourcePolicyHeader(response.httpHeaderField(HTTPHeaderName::CrossOriginResourcePolicy));

    if (policy == CrossOriginResourcePolicy::SameOrigin)
        return true;

    if (policy == CrossOriginResourcePolicy::SameSite) {
        if (origin.isUnique())
            return true;
#if ENABLE(PUBLIC_SUFFIX_LIST)
        if (!RegistrableDomain::uncheckedCreateFromHost(origin.host()).matches(response.url()))
            return true;
#endif
        if (origin.protocol() == "http" && response.url().protocol() == "https")
            return true;
    }

    return false;
}

Optional<ResourceError> validateCrossOriginResourcePolicy(const SecurityOrigin& origin, const URL& requestURL, const ResourceResponse& response)
{
    if (shouldCrossOriginResourcePolicyCancelLoad(origin, response))
        return ResourceError { errorDomainWebKitInternal, 0, requestURL, makeString("Cancelled load to ", response.url().stringCenterEllipsizedToLength(), " because it violates the resource's Cross-Origin-Resource-Policy response header."), ResourceError::Type::AccessControl };
    return WTF::nullopt;
}

Optional<ResourceError> validateRangeRequestedFlag(const ResourceRequest& request, const ResourceResponse& response)
{
    if (response.isRangeRequested() && response.httpStatusCode() == 206 && response.type() == ResourceResponse::Type::Opaque && !request.hasHTTPHeaderField(HTTPHeaderName::Range))
        return ResourceError({ }, 0, response.url(), { }, ResourceError::Type::General);
    return WTF::nullopt;
}

} // namespace WebCore

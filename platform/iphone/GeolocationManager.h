/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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
 */

#ifndef GeolocationManager_h
#define GeolocationManager_h

#include "GeolocationService.h"
#include "Geoposition.h"
#include "PositionError.h"
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>

#ifdef __OBJC__
@class GeolocationCoreLocationDelegate;
#else
class GeolocationCoreLocationDelegate;
#endif


namespace WebCore {

class GeolocationServiceCoreLocation;
class GeolocationLink;

class GeolocationManager : public Noncopyable {
public:
    static GeolocationManager& shared();

    PassRefPtr<GeolocationLink> linkService(GeolocationServiceCoreLocation*, PositionOptions*);

    // GeolocationManager::suspend() turns off location services completely and releases the CLLocationManager.
    void suspend();
    void resume();

private:
    GeolocationManager() : m_locationDelegate(0), m_linkedServices(), m_suspended(false), m_lastPosition(0), m_lastError(0) { }

    void updatePositionOptions();

    RetainPtr<GeolocationCoreLocationDelegate> m_locationDelegate;
    HashSet<GeolocationLink*> m_linkedServices;

    bool m_suspended;

    RefPtr<Geoposition> m_lastPosition;
    RefPtr<PositionError> m_lastError;
    friend class GeolocationLink;

public:
    void positionChanged(PassRefPtr<Geoposition>);
    void errorOccurred(PassRefPtr<PositionError>);
};


class GeolocationLink : public RefCounted<GeolocationLink> {
public:
    void suspend();
    void resume();
    virtual ~GeolocationLink();
private:
    GeolocationLink(GeolocationServiceCoreLocation* service,
                    GeolocationManager* manager,
                    bool highAccuracy)
    : m_service(service), m_geolocationManager(manager), m_suspended(false), m_wantsHighAccuracy(highAccuracy) { }

    GeolocationServiceCoreLocation* m_service;
    GeolocationManager* m_geolocationManager;
    RefPtr<Geoposition> m_positionWaitingForResume;
    RefPtr<PositionError> m_errorWaitingForResume;

    bool m_suspended;
    bool m_wantsHighAccuracy;

    friend class GeolocationManager;
};

} // namespace WebCore


#endif // GeolocationManager_h

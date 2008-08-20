/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AnimationController.h"

#include "CSSPropertyNames.h"
#include "Document.h"
#include "FloatConversion.h"
#include "Frame.h"
#include "RenderObject.h"
#include "RenderStyle.h"
#include "SystemTime.h"
#include "Timer.h"
#include "EventNames.h"
#include "CString.h"

namespace WebCore {

static void setChanged(Node* node)
{
    ASSERT(!node || (node->document() && !node->document()->inPageCache()));
    node->setChanged(AnimationStyleChange);
}

static const double cAnimationTimerDelay = 0.025;

struct CurveData {
    CurveData(double p1x, double p1y, double p2x, double p2y)
    {
        // Calculate the polynomial coefficients, implicit first and last control points are (0,0) and (1,1).
        cx = 3.0 * p1x;
        bx = 3.0 * (p2x - p1x) - cx;
        ax = 1.0 - cx -bx;
         
        cy = 3.0 * p1y;
        by = 3.0 * (p2y - p1y) - cy;
        ay = 1.0 - cy - by;
    }
    
    double sampleCurveX(double t)
    {
        // `ax t^3 + bx t^2 + cx t' expanded using Horner's rule.
        return ((ax * t + bx) * t + cx) * t;
    }
    
    double sampleCurveY(double t)
    {
        return ((ay * t + by) * t + cy) * t;
    }
    
    double sampleCurveDerivativeX(double t)
    {
        return (3.0 * ax * t + 2.0 * bx) * t + cx;
    }
    
    // Given an x value, find a parametric value it came from.
    double solveCurveX(double x, double epsilon)
    {
        double t0;
        double t1;
        double t2;
        double x2;
        double d2;
        int i;

        // First try a few iterations of Newton's method -- normally very fast.
        for (t2 = x, i = 0; i < 8; i++) {
            x2 = sampleCurveX(t2) - x;
            if (fabs (x2) < epsilon)
                return t2;
            d2 = sampleCurveDerivativeX(t2);
            if (fabs(d2) < 1e-6)
                break;
            t2 = t2 - x2 / d2;
        }

        // Fall back to the bisection method for reliability.
        t0 = 0.0;
        t1 = 1.0;
        t2 = x;

        if (t2 < t0)
            return t0;
        if (t2 > t1)
            return t1;

        while (t0 < t1) {
            x2 = sampleCurveX(t2);
            if (fabs(x2 - x) < epsilon)
                return t2;
            if (x > x2)
                t0 = t2;
            else
                t1 = t2;
            t2 = (t1 - t0) * .5 + t0;
        }

        // Failure.
        return t2;
    }
    
private:
    double ax;
    double bx;
    double cx;
    
    double ay;
    double by;
    double cy;
};

// The epsilon value we pass to solveCurveX given that the animation is going to run over |dur| seconds. The longer the
// animation, the more precision we need in the timing function result to avoid ugly discontinuities.
static inline double solveEpsilon(double duration) { return 1. / (200. * duration); }

static inline double solveCubicBezierFunction(double p1x, double p1y, double p2x, double p2y, double t, double duration)
{
    // Convert from input time to parametric value in curve, then from
    // that to output time.
    CurveData c(p1x, p1y, p2x, p2y);
    t = c.solveCurveX(t, solveEpsilon(duration));
    t = c.sampleCurveY(t);
    return t;
}

class CompositeAnimation;
class AnimationBase;

class AnimationTimerBase {
public:
    AnimationTimerBase(AnimationBase* anim)
    : m_timer(this, &AnimationTimerBase::timerFired)
    , m_anim(anim)
    {
        m_timer.startOneShot(0);
    }
    virtual ~AnimationTimerBase()
    {
    }
    
    void startTimer(double timeout=0)
    {
        m_timer.startOneShot(timeout);
    }
    
    void cancelTimer()
    {
        m_timer.stop();
    }
    
    virtual void timerFired(Timer<AnimationTimerBase>*) = 0;

private:
    Timer<AnimationTimerBase> m_timer;
    
protected:
    AnimationBase* m_anim;
};

class AnimationEventDispatcher : public AnimationTimerBase {
public:
    AnimationEventDispatcher(AnimationBase* anim) 
    : AnimationTimerBase(anim)
    , m_property(-1)
    , m_reset(false)
    , m_elapsedTime(-1)
    {
    }
    
    virtual ~AnimationEventDispatcher()
    {
    }
    
    void startTimer(HTMLElement* element, const AtomicString& name, int property, bool reset, 
                                                    const AtomicString& eventType, double elapsedTime)
    {
        m_element = element;
        m_name = name;
        m_property = property;
        m_reset = reset;
        m_eventType = eventType;
        m_elapsedTime = elapsedTime;
        AnimationTimerBase::startTimer();
    }
    
    virtual void timerFired(Timer<AnimationTimerBase>*);
    
private:
    RefPtr<HTMLElement> m_element;
    AtomicString m_name;
    int m_property;
    bool m_reset;
    AtomicString m_eventType;
    double m_elapsedTime;
};

class AnimationTimerCB : public AnimationTimerBase {
public:
    AnimationTimerCB(AnimationBase* anim) 
    : AnimationTimerBase(anim)
    , m_elapsedTime(0)
    { }
    virtual ~AnimationTimerCB() { }
    
    virtual void timerFired(Timer<AnimationTimerBase>*);
    
    void startTimer(double timeout, const AtomicString& eventType, double elapsedTime)
    {
        m_eventType = eventType;
        m_elapsedTime = elapsedTime;
        AnimationTimerBase::startTimer(timeout);
    }

private:
    AtomicString m_eventType;
    double m_elapsedTime;
};

#pragma mark -

class ImplicitAnimation;
class KeyframeAnimation;
class AnimationControllerPrivate;

class CompositeAnimation : public Noncopyable {
public:
    CompositeAnimation(bool suspended, AnimationControllerPrivate* animationController)
     : m_suspended(suspended)
     , m_animationController(animationController)
     , m_numStyleIsSetupWaiters(0)
    { }
    
    ~CompositeAnimation()
    {
        deleteAllValues(m_transitions);
        deleteAllValues(m_keyframeAnimations);
    }

    RenderStyle* animate(RenderObject*, RenderStyle* currentStyle, RenderStyle* targetStyle);

    void setAnimating(bool inAnimating);
    bool animating();

    bool hasAnimationForProperty(int prop) const { return m_transitions.contains(prop); }

    void resetTransitions(RenderObject*);
    void resetAnimations(RenderObject*);

    void cleanupFinishedAnimations(RenderObject*);
    
    void setAnimationStartTime(double t);
    void setTransitionStartTime(int property, double t);
    
    void suspendAnimations();
    void resumeAnimations();
    bool suspended() const      { return m_suspended; }
    
    void overrideImplicitAnimations(int property);
    void resumeOverriddenImplicitAnimations(int property);
    
    void styleIsSetup();
    
    bool isAnimatingProperty(int property) const;
    
    void setWaitingForStyleIsSetup(bool waiting);
protected:
    void updateTransitions(RenderObject* renderer, RenderStyle* currentStyle, RenderStyle* targetStyle);
    void updateKeyframeAnimations(RenderObject* renderer, RenderStyle* currentStyle, RenderStyle* targetStyle);
    
    int findKeyframeAnimation(const AtomicString& name);

private:
    typedef HashMap<int, ImplicitAnimation*>    CSSPropertyTransitionsMap;

    CSSPropertyTransitionsMap   m_transitions;
    Vector<KeyframeAnimation*>  m_keyframeAnimations;
    bool                        m_suspended;
    AnimationControllerPrivate* m_animationController;
    uint32_t                    m_numStyleIsSetupWaiters;
};

#pragma mark -

class AnimationBase : public Noncopyable {
public:
    AnimationBase(const Transition* transition, RenderObject* renderer, CompositeAnimation* compAnim);    
    virtual ~AnimationBase()
    {
        if (m_animState == STATE_START_WAIT_STYLE_SETUP)
            m_compAnim->setWaitingForStyleIsSetup(false);
    }
    
    RenderObject* renderer() const { return m_object; }
    double startTime() const { return m_startTime; }
    double duration() const  { return m_animation->transitionDuration(); }
    
    void cancelTimers()
    {
        m_animationTimerCB.cancelTimer();
        m_animationEventDispatcher.cancelTimer();
    }

    // Animations and Transitions go through the states below. When entering the STARTED state
    // the animation is started. This may or may not require deferred response from the animator.
    // If so, we stay in this state until that response is received (and it returns the start time).
    // Otherwise, we use the current time as the start time and go immediately to the LOOPING or
    // ENDING state.
    enum AnimState { 
        STATE_NEW,                      // animation just created, animation not running yet
        STATE_START_WAIT_TIMER,         // start timer running, waiting for fire
        STATE_START_WAIT_STYLE_SETUP,   // waiting for style setup so we can start animations
        STATE_START_WAIT_RESPONSE,      // animation started, waiting for response
        STATE_LOOPING,                  // response received, animation running, loop timer running, waiting for fire
        STATE_ENDING,                   // response received, animation running, end timer running, waiting for fire
        STATE_PAUSED_WAIT_TIMER,        // animation in pause mode when animation started
        STATE_PAUSED_WAIT_RESPONSE,     // animation paused when in STARTING state
        STATE_PAUSED_RUN,               // animation paused when in LOOPING or ENDING state
        STATE_DONE                      // end timer fired, animation finished and removed
    };
    
    enum AnimStateInput {
        STATE_INPUT_MAKE_NEW,           // reset back to new from any state
        STATE_INPUT_START_ANIMATION,    // animation requests a start
        STATE_INPUT_RESTART_ANIMATION,  // force a restart from any state
        STATE_INPUT_START_TIMER_FIRED,  // start timer fired
        STATE_INPUT_STYLE_SETUP,        // style is setup, ready to start animating
        STATE_INPUT_START_TIME_SET,     // m_startTime was set
        STATE_INPUT_LOOP_TIMER_FIRED,   // loop timer fired
        STATE_INPUT_END_TIMER_FIRED,    // end timer fired
        STATE_INPUT_PAUSE_OVERRIDE,     // pause an animation due to override
        STATE_INPUT_RESUME_OVERRIDE,    // resume an overridden animation
        STATE_INPUT_PLAY_STATE_RUNNING, // play state paused -> running
        STATE_INPUT_PLAY_STATE_PAUSED,  // play state running -> paused
        STATE_INPUT_END_ANIMATION       // force an end from any state
    };
    
    // called when animation is in NEW state to start animation
    void updateStateMachine(AnimStateInput input, double param);
    
    // animation has actually started, at passed time
    // This is a callback and is only received when RenderObject::startAnimation() or RenderObject::startTransition() 
    // returns true. If RenderObject::
    void onAnimationStartResponse(double startTime);

    // called to change to or from paused state
    void updatePlayState(bool running);
    bool playStatePlaying() const       { return m_animation && m_animation->animationPlayState() == AnimPlayStatePlaying; }
    
    bool waitingToStart() const { return m_animState == STATE_NEW || m_animState == STATE_START_WAIT_TIMER; }
    bool preactive() const      { return m_animState == STATE_NEW || m_animState == STATE_START_WAIT_TIMER ||
                                         m_animState == STATE_START_WAIT_STYLE_SETUP || m_animState == STATE_START_WAIT_RESPONSE; }
    bool postactive() const     { return m_animState == STATE_DONE; }
    bool active() const         { return !postactive() && !preactive(); }
    bool running() const        { return m_pauseTime < 0; }
    bool isnew() const          { return m_animState == STATE_NEW; }
    bool waitingForStartTime() const { return m_animState == STATE_START_WAIT_RESPONSE; }
    bool waitingForStyleSetup() const { return m_animState == STATE_START_WAIT_STYLE_SETUP; }
    bool waitingForEndEvent() const  { return m_waitingForEndEvent; }
    
    // "animating" means that something is running that requires a timer to keep firing
    // (e.g. a software animation)
    void setAnimating(bool inAnimating = true)  { m_animating = inAnimating; }
    bool animating() const                      { return m_animating; }

    double progress(double scale, double offset) const;

    virtual void animate(CompositeAnimation*, RenderObject*, const RenderStyle* currentStyle, 
                                    const RenderStyle* targetStyle, RenderStyle*& ioAnimatedStyle) { }
    virtual void reset(RenderObject* renderer, const RenderStyle* from = 0, const RenderStyle* to = 0) { }
    
    virtual bool shouldFireEvents() const   { return false; }
    
    void animationTimerCBFired(const AtomicString& eventType, double elapsedTime);
    void animationEventDispatcherFired(HTMLElement* element, const AtomicString& name, int property, bool reset, 
                                                                const AtomicString& eventType, double elapsedTime);

    bool animationsMatch(const Transition* anim) const { return m_animation->transitionsMatch(anim, false); }
    
    void setAnimation(const Transition* anim) { m_animation = const_cast<Transition*>(anim); }
    
    // return true if this animation is overridden. This will only be the case for
    // ImplicitAnimations and is used to determine whether or not we should force
    // set the start time. If an animation is overridden, it will probably not get
    // back the START_TIME event.
    virtual bool overridden() const { return false; }
    
    // does this animation/transition involve the given property?
    virtual bool affectsProperty(int property) const { return false; }
    bool isAnimatingProperty(int property) const
    {
        return (!waitingToStart() && !postactive()) && affectsProperty(property);
    }
    
protected:
    HTMLElement* elementForEventDispatch()
    {
        if (m_object->node() && m_object->node()->isHTMLElement())
            return static_cast<HTMLElement*>(m_object->node());
        return 0;
    }
    
    // these can be overridden by the KeyframeAnimation subclass to override ImplicitAnimations
    virtual void overrideAnimations()                       { }
    virtual void resumeOverriddenAnimations()               { }
    
    CompositeAnimation* compositeAnimation()                { return m_compAnim; }

    // these are called when the corresponding timer fires so subclasses can do any extra work
    virtual void onAnimationStart(double elapsedTime)       { }
    virtual void onAnimationIteration(double elapsedTime)   { }
    virtual void onAnimationEnd(double elapsedTime)         { }
    virtual bool startAnimation(double beginTime)           { return false; }
    virtual void endAnimation(bool reset)                   { }
    
    void primeEventTimers();
    
protected:
    AnimState m_animState;
    int m_iteration;
    
    bool m_animating;       // transition/animation requires continual timer firing
    bool m_waitedForResponse;
    double m_startTime;
    double m_pauseTime;
    RenderObject*   m_object;

    AnimationTimerCB m_animationTimerCB;
    AnimationEventDispatcher m_animationEventDispatcher;
    RefPtr<Transition> m_animation;
    CompositeAnimation* m_compAnim;
    bool m_waitingForEndEvent;
};

#pragma mark -

class ImplicitAnimation : public AnimationBase {
public:
    ImplicitAnimation(const Transition* transition, RenderObject* renderer, CompositeAnimation* compAnim)
        : AnimationBase(transition, renderer, compAnim)
        , m_property(transition->transitionProperty())
        , m_overridden(false)
        , m_fromStyle(0)
        , m_toStyle(0)
    {
    }
    
    virtual ~ImplicitAnimation()
    {
         ASSERT(!m_fromStyle && !m_toStyle);
         
        // if we were waiting for an end event, we need to finish the animation to make sure no old
        // animations stick around in the lower levels
        if (waitingForEndEvent() && m_object)
            m_object->transitionFinished(m_property);
            
        // do the cleanup here instead of in the base class so the specialized methods get called
        if (!postactive())
            updateStateMachine(STATE_INPUT_END_ANIMATION, -1);     
    }

    int transitionProperty() const { return m_property; }

    virtual void onAnimationEnd(double inElapsedTime);
    virtual bool startAnimation(double beginTime);
    virtual void endAnimation(bool reset);

    virtual void animate(CompositeAnimation*, RenderObject*, const RenderStyle* currentStyle, 
                                    const RenderStyle* targetStyle, RenderStyle*& ioAnimatedStyle);
    virtual void reset(RenderObject* renderer, const RenderStyle* from = 0, const RenderStyle* to = 0);
    
    void setOverridden(bool b);
    virtual bool overridden() const { return m_overridden; }

    virtual bool shouldFireEvents() const   { return true; }

    virtual bool affectsProperty(int property) const;
    
    bool hasStyle() const { return m_fromStyle && m_toStyle; }
    
protected:
    bool shouldSendEventForListener(Document::ListenerType inListenerType)
    {
        return m_object->document()->hasListenerType(inListenerType);
    }
    
    bool sendTransitionEvent(const AtomicString& inEventType, double inElapsedTime);    
    
private:
    int m_property;
    bool m_overridden;

    // The two styles that we are blending.
    RenderStyle* m_fromStyle;
    RenderStyle* m_toStyle;
};

#pragma mark -

class KeyframeAnimation : public AnimationBase {
public:
    KeyframeAnimation(const Transition* transition, RenderObject* renderer, int index, CompositeAnimation* compAnim)
        : AnimationBase(transition, renderer, compAnim)
        , m_keyframes(transition->keyframeList())
        , m_name(transition->animationName())
        , m_index(index)
    {
    }
    virtual ~KeyframeAnimation()
    {
        // if we were waiting for an end event, we need to finish the animation to make sure no old
        // animations stick around in the lower levels
        if (waitingForEndEvent() && m_object) 
            m_object->animationFinished(m_name, 0, true);

        // do the cleanup here instead of in the base class so the specialized methods get called
        if (!postactive())
            updateStateMachine(STATE_INPUT_END_ANIMATION, -1);     
    }

    virtual void animate(CompositeAnimation*, RenderObject*, const RenderStyle* currentStyle, 
                                    const RenderStyle* targetStyle, RenderStyle*& ioAnimatedStyle);
    
    void setName(const String& s)           { m_name = s; }
    const AtomicString& name() const        { return m_name; }

    virtual bool shouldFireEvents() const   { return true; }

protected:
    virtual void onAnimationStart(double inElapsedTime);
    virtual void onAnimationIteration(double inElapsedTime);
    virtual void onAnimationEnd(double inElapsedTime);
    virtual bool startAnimation(double beginTime);
    virtual void endAnimation(bool reset);

    virtual void overrideAnimations();
    virtual void resumeOverriddenAnimations();
    
    bool shouldSendEventForListener(Document::ListenerType inListenerType)
    {
        return m_object->document()->hasListenerType(inListenerType);
    }
    
    bool sendAnimationEvent(const AtomicString& inEventType, double inElapsedTime);
    
    virtual bool affectsProperty(int property) const;

private:
    // The keyframes that we are blending.
    RefPtr<KeyframeList> m_keyframes;
    AtomicString m_name;
    int m_index;
};

#pragma mark -

void AnimationTimerCB::timerFired(Timer<AnimationTimerBase>*)
{
    m_anim->animationTimerCBFired(m_eventType, m_elapsedTime);
}

void AnimationEventDispatcher::timerFired(Timer<AnimationTimerBase>*)
{
    m_anim->animationEventDispatcherFired(m_element.get(), m_name, m_property, m_reset, m_eventType, m_elapsedTime);
}

#pragma mark -

AnimationBase::AnimationBase(const Transition* transition, RenderObject* renderer, CompositeAnimation* compAnim)
    : m_animState(STATE_NEW)
    , m_iteration(0)
    , m_animating(false)
    , m_waitedForResponse(false)
    , m_startTime(0)
    , m_pauseTime(-1)
    , m_object(renderer)
    , m_animationTimerCB(const_cast<AnimationBase*>(this))
    , m_animationEventDispatcher(const_cast<AnimationBase*>(this))
    , m_animation(const_cast<Transition*>(transition))
    , m_compAnim(compAnim)
    , m_waitingForEndEvent(false)
{
}

//#define DEBUG_STATE_MACHINE

#ifdef DEBUG_STATE_MACHINE
#define DSM_PRINT_FORCE(input, next) \
    fprintf(stderr, "*** got %s --> %s at %f for animation %p\n", input, next, currentTime(), this)

#define DSM_PRINT(state, input, next) \
    fprintf(stderr, "*** got %s in %s --> %s at %f for animation %p\n", input, state, next, currentTime(), this)

#define DSM_PRINT_PARAM(state, input, name, param, next) \
    fprintf(stderr, "*** got %s in %s --> %s (%s=%f) at %f for animation %p\n", input, state, next, name, param, currentTime(), this)
#else
#define DSM_PRINT_FORCE(input, next)
#define DSM_PRINT(state, input, next)
#define DSM_PRINT_PARAM(state, input, name, param, next)
#endif

void AnimationBase::updateStateMachine(AnimStateInput input, double param)
{
    // if we get a RESTART then we force a new animation, regardless of state
    if (input == STATE_INPUT_MAKE_NEW) {
        DSM_PRINT_FORCE("STATE_INPUT_MAKE_NEW", "STATE_NEW");
        if (m_animState == STATE_START_WAIT_STYLE_SETUP)
            m_compAnim->setWaitingForStyleIsSetup(false);
        m_animState = STATE_NEW;
        m_startTime = 0;
        m_pauseTime = -1;
        m_waitedForResponse = false;
        endAnimation(false);
        return;
    }
    else if (input == STATE_INPUT_RESTART_ANIMATION) {
        DSM_PRINT_FORCE("STATE_INPUT_RESTART_ANIMATION", "STATE_NEW");
        cancelTimers();
        if (m_animState == STATE_START_WAIT_STYLE_SETUP)
            m_compAnim->setWaitingForStyleIsSetup(false);
        m_animState = STATE_NEW;
        m_startTime = 0;
        m_pauseTime = -1;
        endAnimation(false);
        
        if (running())
            updateStateMachine(STATE_INPUT_START_ANIMATION, -1);
        return;
    }
    else if (input == STATE_INPUT_END_ANIMATION) {
        DSM_PRINT_FORCE("STATE_INPUT_END_ANIMATION", "STATE_DONE");
        cancelTimers();
        if (m_animState == STATE_START_WAIT_STYLE_SETUP)
            m_compAnim->setWaitingForStyleIsSetup(false);
        m_animState = STATE_DONE;
        endAnimation(true);
        return;
    }
    else if (input == STATE_INPUT_PAUSE_OVERRIDE) {
#ifdef DEBUG_STATE_MACHINE
        fprintf(stderr, "*** got STATE_INPUT_PAUSE_OVERRIDE at %f for animation %p\n", currentTime(), this);
#endif

        if (m_animState == STATE_START_WAIT_RESPONSE) {
            // If we are in the WAIT_RESPONSE state, the animation will get canceled before 
            // we get a response, so move to the next state
            endAnimation(false);
            updateStateMachine(STATE_INPUT_START_TIME_SET, currentTime());
        }
        return;
    }
    else if (input == STATE_INPUT_RESUME_OVERRIDE) {
#ifdef DEBUG_STATE_MACHINE
        fprintf(stderr, "*** got STATE_INPUT_RESUME_OVERRIDE at %f for animation %p\n", currentTime(), this);
#endif

        if (m_animState == STATE_LOOPING || m_animState == STATE_ENDING) {
            // start the animation
            startAnimation(m_startTime);
        }
        return;
    }
    
    // execute state machine
    switch(m_animState) {
        case STATE_NEW:
            ASSERT(input == STATE_INPUT_START_ANIMATION || input == STATE_INPUT_PLAY_STATE_RUNNING || input == STATE_INPUT_PLAY_STATE_PAUSED);
            if (input == STATE_INPUT_START_ANIMATION || input == STATE_INPUT_PLAY_STATE_RUNNING) {
                DSM_PRINT_PARAM("STATE_NEW",
                                (input == STATE_INPUT_START_ANIMATION) ? "STATE_INPUT_START_ANIMATION": "STATE_INPUT_PLAY_STATE_RUNNING",
                                "delay", m_animation->animationDelay(), "STATE_START_WAIT_TIMER");
                // set the start timer to the initial delay (0 if no delay)
                m_waitedForResponse = false;
                m_animState = STATE_START_WAIT_TIMER;
                m_animationTimerCB.startTimer(m_animation->animationDelay(), 
                                        EventNames::webkitAnimationStartEvent, m_animation->animationDelay());
            }
            else {
                // if input is STATE_INPUT_PLAY_STATE_PAUSED we don't do anything yet
                DSM_PRINT("STATE_NEW", "STATE_INPUT_PLAY_STATE_PAUSED", "STATE_NEW");
            }
            break;
        case STATE_START_WAIT_TIMER:
            ASSERT(input == STATE_INPUT_START_TIMER_FIRED || input == STATE_INPUT_PLAY_STATE_PAUSED);
            
            if (input == STATE_INPUT_START_TIMER_FIRED) {
                ASSERT(param >= 0);
                DSM_PRINT_PARAM("STATE_START_WAIT_TIMER", "STATE_INPUT_START_TIMER_FIRED", "elapsedTime", param, 
                                                                                "STATE_START_WAIT_STYLE_SETUP");
                // start timer has fired, tell the animation to start and wait for it to respond with start time
                m_animState = STATE_START_WAIT_STYLE_SETUP;
                m_compAnim->setWaitingForStyleIsSetup(true);
                
                // trigger a render so we can start the animation
                setChanged(m_object->element());
                m_object->animationController()->startUpdateRenderingDispatcher();
            }
            else {
                ASSERT(running());
                DSM_PRINT("STATE_START_WAIT_TIMER", "STATE_INPUT_PLAY_STATE_PAUSED", "STATE_PAUSED_WAIT_TIMER");
                
                // we're waiting for the start timer to fire and we got a pause. Cancel the timer, pause and wait
                m_pauseTime = currentTime();
                cancelTimers();
                m_animState = STATE_PAUSED_WAIT_TIMER;
            }
            break;
        case STATE_START_WAIT_STYLE_SETUP:
            ASSERT(input == STATE_INPUT_STYLE_SETUP || input == STATE_INPUT_PLAY_STATE_PAUSED);
            
            m_compAnim->setWaitingForStyleIsSetup(false);
            
            if (input == STATE_INPUT_STYLE_SETUP) {
                DSM_PRINT("STATE_START_WAIT_STYLE_SETUP", "STATE_INPUT_STYLE_SETUP", "STATE_START_WAIT_RESPONSE");
                // start timer has fired, tell the animation to start and wait for it to respond with start time
                m_animState = STATE_START_WAIT_RESPONSE;
                
                overrideAnimations();
                
                // send start event, if needed
                onAnimationStart(0.0f); // the elapsedTime is always 0 here

                // start the animation
                if (overridden() || !startAnimation(0)) {
#ifdef DEBUG_STATE_MACHINE
                    fprintf(stderr, "***         startAnimation returned false, setting start time immediately\n");
#endif
                    // we're not going to get a startTime callback, so fire the start time here
                    m_animState = STATE_START_WAIT_RESPONSE;
                    updateStateMachine(STATE_INPUT_START_TIME_SET, currentTime());
                }
                else
                    m_waitedForResponse = true;
            }
            else {
                ASSERT(running());
                DSM_PRINT("STATE_START_WAIT_TIMER", "STATE_INPUT_PLAY_STATE_PAUSED", "STATE_START_WAIT_RESPONSE");
                
                // we're waiting for the a notification that the style has been setup. If we're asked to wait
                // at this point, the style must have been processed, so we can deal with this like we would
                // for WAIT_RESPONSE, except that we don't need to do an endAnimation().
                m_pauseTime = 0;
                m_animState = STATE_START_WAIT_RESPONSE;
            }
            break;
        case STATE_START_WAIT_RESPONSE:
            ASSERT(input == STATE_INPUT_START_TIME_SET || input == STATE_INPUT_PLAY_STATE_PAUSED);

            if (input == STATE_INPUT_START_TIME_SET) {
                ASSERT(param >= 0);
                // we have a start time, set it, unless the startTime is already set
                if (m_startTime <= 0)
                    m_startTime = param;
                
                // decide when the end or loop event needs to fire
                primeEventTimers();
                
                DSM_PRINT_PARAM("STATE_START_WAIT_RESPONSE", "STATE_INPUT_START_TIME_SET", "startTime", param, 
                                (m_animState==STATE_LOOPING)?"STATE_LOOPING":"STATE_ENDING");

                // trigger a render so we can start the animation
                setChanged(m_object->element());
                m_object->animationController()->startUpdateRenderingDispatcher();
            }
            else {
                DSM_PRINT("STATE_START_WAIT_RESPONSE", "STATE_INPUT_PLAY_STATE_PAUSED", "STATE_PAUSED_WAIT_RESPONSE");
                // We are pausing while waiting for a start response. Cancel the animation and wait. When 
                // we unpause, we will act as though the start timer just fired
                m_pauseTime = 0;
                endAnimation(false);
                m_animState = STATE_PAUSED_WAIT_RESPONSE;
            }
            break;
        case STATE_LOOPING:
            ASSERT(input == STATE_INPUT_LOOP_TIMER_FIRED || input == STATE_INPUT_PLAY_STATE_PAUSED);

            if (input == STATE_INPUT_LOOP_TIMER_FIRED) {
                ASSERT(param >= 0);
                // loop timer fired, loop again or end.
                onAnimationIteration(param);
                primeEventTimers();
                DSM_PRINT_PARAM("STATE_LOOPING", "STATE_INPUT_LOOP_TIMER_FIRED", "elapsedTime", param, 
                                (m_animState==STATE_LOOPING)?"STATE_LOOPING":"STATE_ENDING");
            }
            else {
                DSM_PRINT("STATE_LOOPING", "STATE_INPUT_PLAY_STATE_PAUSED", "STATE_PAUSED_RUN");
                // we are pausing while running. Cancel the animation and wait
                m_pauseTime = currentTime();
                cancelTimers();
                endAnimation(false);
                m_animState = STATE_PAUSED_RUN;
            }
            break;
        case STATE_ENDING:
            ASSERT(input == STATE_INPUT_END_TIMER_FIRED || input == STATE_INPUT_PLAY_STATE_PAUSED);

            if (input == STATE_INPUT_END_TIMER_FIRED) {
                ASSERT(param >= 0);
                DSM_PRINT_PARAM("STATE_ENDING", "STATE_INPUT_END_TIMER_FIRED", "elapsedTime", param, "STATE_DONE");
                // end timer fired, finish up
                onAnimationEnd(param);
                
                resumeOverriddenAnimations();
                
                // fire off another style change so we can set the final value
                setChanged(m_object->element());
                m_animState = STATE_DONE;
                m_object->animationController()->startUpdateRenderingDispatcher();
                // |this| may be deleted here when we've been called from timerFired()
            }
            else {
                DSM_PRINT("STATE_ENDING", "STATE_INPUT_PLAY_STATE_PAUSED", "STATE_PAUSED_RUN");
                // we are pausing while running. Cancel the animation and wait
                m_pauseTime = currentTime();
                cancelTimers();
                endAnimation(false);
                m_animState = STATE_PAUSED_RUN;
            }
            // |this| may be deleted here
            break;
        case STATE_PAUSED_WAIT_TIMER:
            ASSERT(input == STATE_INPUT_PLAY_STATE_RUNNING);
            ASSERT(!running());
            DSM_PRINT("STATE_PAUSED_WAIT_TIMER", "STATE_INPUT_PLAY_STATE_RUNNING", "STATE_NEW");
            
            // update the times
            m_startTime += currentTime() - m_pauseTime;
            m_pauseTime = -1;

            // we were waiting for the start timer to fire, go back and wait again
            m_animState = STATE_NEW;
            updateStateMachine(STATE_INPUT_START_ANIMATION, 0);
            break;
        case STATE_PAUSED_WAIT_RESPONSE:
        case STATE_PAUSED_RUN:
            // we treat these two cases the same. The only difference is that, when we are in the WAIT_RESPONSE
            // state, we don't yet have a valid startTime, so we send 0 to startAnimation. When the START_TIME
            // event comes in qnd we were in the RUN state, we will notice that we have already set the 
            // startTime and will ignore it.
            ASSERT(input == STATE_INPUT_PLAY_STATE_RUNNING);
            ASSERT(!running());
            DSM_PRINT("STATE_PAUSED_WAIT_RESPONSE", "STATE_INPUT_PLAY_STATE_RUNNING", "STATE_START_WAIT_RESPONSE");
            
            // update the times
            if (m_animState == STATE_PAUSED_RUN)
                m_startTime += currentTime() - m_pauseTime;
            else
                m_startTime = 0;
            m_pauseTime = -1;

            // we were waiting for a begin time response from the animation, go back and wait again
            m_animState = STATE_START_WAIT_RESPONSE;
                
            // start the animation
            if (overridden() || !startAnimation(m_startTime)) {
                // we're not going to get a startTime callback, so fire the start time here
                updateStateMachine(STATE_INPUT_START_TIME_SET, currentTime());
            }
            else
                m_waitedForResponse = true;
            break;
        case STATE_DONE:
            DSM_PRINT_FORCE("STATE_DONE", "STATE_DONE");
            // we're done. Stay in this state until we are deleted
            break;
    }
    // |this| may be deleted here if we came out of STATE_ENDING when we've been called from timerFired()
}

void AnimationBase::animationTimerCBFired(const AtomicString& eventType, double elapsedTime)
{
    ASSERT(m_object->document() && !m_object->document()->inPageCache());
    
    // FIXME: use an enum
    if (eventType == EventNames::webkitAnimationStartEvent)
        updateStateMachine(STATE_INPUT_START_TIMER_FIRED, elapsedTime);
    else if (eventType == EventNames::webkitAnimationIterationEvent)
        updateStateMachine(STATE_INPUT_LOOP_TIMER_FIRED, elapsedTime);
    else if (eventType == EventNames::webkitAnimationEndEvent) {
        updateStateMachine(STATE_INPUT_END_TIMER_FIRED, elapsedTime);
        // |this| may be deleted here
    }
}

void AnimationBase::animationEventDispatcherFired(HTMLElement* element, const AtomicString& name, int property, 
                                                    bool reset, const AtomicString& eventType, double elapsedTime)
{
    m_waitingForEndEvent = false;
    
    // keep an atomic string on the stack to keep it alive until we exit this method
    // (since dispatching the event may cause |this| to be deleted, therefore removing
    // the last ref to the atomic string).
    AtomicString        animName(name);
    AtomicString        animEventType(eventType);
    // make sure the element sticks around too
    RefPtr<HTMLElement> elementRetainer(element);
    
    ASSERT(!element || (element->document() && !element->document()->inPageCache()));
    if (!element)
        return;

    if (eventType == EventNames::webkitTransitionEndEvent) {
        element->dispatchWebKitTransitionEvent(eventType, name, elapsedTime);
    } 
    else {
        element->dispatchWebKitAnimationEvent(eventType, name, elapsedTime);
    }
    // WARNING: |this| may have been deleted here

    
    // We didn't make the endAnimation call if we dispatched an event. This is so
    // we can call the animation event handler then remove the animation. This
    // solves the problem on the phone where the object snaps into its unanimated state
    // before the author has a change to change it (make it hidden, move it offscreen, etc.)
    // We can't call endAnimation() here because it is specialized depending on whether
    // we are doing Transitions or Animations. So we do the equivalent to it here.
    if (animEventType == EventNames::webkitTransitionEndEvent) {
        if (element->renderer())
            element->renderer()->transitionFinished(property);
    }
    else if (animEventType == EventNames::webkitAnimationEndEvent) {
        if (element->renderer()) {
            element->renderer()->animationFinished(animName, 0, reset);
    
            // restore the original (unanimated) style
            setChanged(element->renderer()->element());
        }
    }
}

void AnimationBase::updatePlayState(bool run)
{
    if (running() != run || isnew()) {
        if (m_object->document()->loadComplete())
            updateStateMachine(run ? STATE_INPUT_PLAY_STATE_RUNNING : STATE_INPUT_PLAY_STATE_PAUSED, -1);
    }
}

double AnimationBase::progress(double scale, double offset) const
{
    if (preactive())
        return 0;
        
    double elapsedTime = running() ? (currentTime() - m_startTime) : (m_pauseTime - m_startTime);
    if (running() && elapsedTime < 0)
        return 0;
        
    double dur = m_animation->transitionDuration();
    if (m_animation->transitionIterationCount() > 0)
        dur *= m_animation->transitionIterationCount();
        
    if (postactive() || !m_animation->transitionDuration() || (m_animation->transitionIterationCount() > 0 && elapsedTime >= dur))
        return 1.0;

    // Compute the fractional time, taking into account direction.
    // There is no need to worry about iterations, we assume that we would have 
    // short circuited above if we were done
    double t = elapsedTime / m_animation->transitionDuration();
    int i = (int) t;
    t -= i;
    if (m_animation->animationDirection() && (i & 1))
        t = 1 - t;
        
    if (scale != 1 || offset != 0)
        t = (t - offset) * scale;
        
    if (m_animation->transitionTimingFunction().type() == LinearTimingFunction)
        return t;
    
    // Cubic bezier.
    double tt = solveCubicBezierFunction(m_animation->transitionTimingFunction().x1(), 
                                         m_animation->transitionTimingFunction().y1(), 
                                         m_animation->transitionTimingFunction().x2(), 
                                         m_animation->transitionTimingFunction().y2(),
                                         t, m_animation->transitionDuration());
    return tt;
}

void AnimationBase::primeEventTimers()
{
    // decide when the end or loop event needs to fire
    double ct = currentTime();
    const double elapsedDuration = ct - m_startTime;
    ASSERT(elapsedDuration >= 0);
    
    double totalDuration = -1;
    if (m_animation->transitionIterationCount() > 0)
        totalDuration = m_animation->transitionDuration() * m_animation->transitionIterationCount();
        
    double durationLeft = 0;
    double nextIterationTime = totalDuration;
    if (totalDuration < 0 || elapsedDuration < totalDuration) {
        durationLeft = m_animation->transitionDuration() - fmod(elapsedDuration, m_animation->transitionDuration());
        nextIterationTime = elapsedDuration + durationLeft;
    }

    // At this point, we may have 0 durationLeft, if we've gotten the event late and we are already
    // past totalDuration. In this case we still fire an end timer before processing the end. 
    // This defers the call to sendAnimationEvents to avoid re-entrant calls that destroy
    // the RenderObject, and therefore |this| before we're done with it.
    if (totalDuration < 0 || nextIterationTime < totalDuration) {
        // we are not at the end yet, send a loop event
        ASSERT(nextIterationTime > 0);
        m_animState = STATE_LOOPING;
        m_animationTimerCB.startTimer(durationLeft, EventNames::webkitAnimationIterationEvent, nextIterationTime);
    }
    else {
        // we are at the end, send an end event
        m_animState = STATE_ENDING;
        m_animationTimerCB.startTimer(durationLeft, EventNames::webkitAnimationEndEvent, nextIterationTime);
    }
}

#pragma mark -

static inline int blendFunc(int from, int to, double progress)
{  
    return int(from + (to - from) * progress);
}

static inline double blendFunc(double from, double to, double progress)
{  
    return from + (to - from) * progress;
}

static inline float blendFunc(float from, float to, double progress)
{  
    return narrowPrecisionToFloat(from + (to - from) * progress);
}

static inline Color blendFunc(const Color& from, const Color& to, double progress)
{  
    return Color(blendFunc(from.red(), to.red(), progress),
                 blendFunc(from.green(), to.green(), progress),
                 blendFunc(from.blue(), to.blue(), progress),
                 blendFunc(from.alpha(), to.alpha(), progress));
}

static inline Length blendFunc(const Length& from, const Length& to, double progress)
{  
    return to.blend(from, progress);
}

static inline IntSize blendFunc(const IntSize& from, const IntSize& to, double progress)
{  
    return IntSize(blendFunc(from.width(), to.width(), progress),
                   blendFunc(from.height(), to.height(), progress));
}

static inline ShadowData* blendFunc(const ShadowData* from, const ShadowData* to, double progress)
{  
    ASSERT(from && to);
    return new ShadowData(blendFunc(from->x, to->x, progress), blendFunc(from->y, to->y, progress), 
                            blendFunc(from->blur, to->blur, progress), blendFunc(from->color, to->color, progress));
}

static inline TransformOperations blendFunc(const TransformOperations& from, const TransformOperations& to, double progress)
{    
    // Blend any operations whose types actually match up.  Otherwise don't bother.
    unsigned fromSize = from.size();
    unsigned toSize = to.size();
    unsigned size = max(fromSize, toSize);
    TransformOperations result;
    for (unsigned i = 0; i < size; i++) {
        TransformOperation* fromOp = i < fromSize ? from[i].get() : 0;
        TransformOperation* toOp = i < toSize ? to[i].get() : 0;
        TransformOperation* blendedOp = toOp ? toOp->blend(fromOp, progress) : fromOp->blend(0, progress, true);
        if (blendedOp)
            result.append(blendedOp);
        else {
            // some of the transform ops (like perspective and matrix3D) do not blend, because they never expect
            // to be used by the software animator, since software animation of transforms only deals with 2D.
            // But we do come here at the start and end of an animation, so we need to return valid values for 
            // those cases. We don't expect to come here for any other values of 'progress', but try to return
            // something reasonable if we do. We actually won't normally get a progress of 0, we'll get a value
            // that is just slightly greater than 0.
            if (progress > 0.5)
                result.append(toOp ? toOp : new IdentityTransformOperation());
            else
                result.append( fromOp ? fromOp : new IdentityTransformOperation());
        }
    }
    return result;
}

static inline EVisibility blendFunc(EVisibility from, EVisibility to, double progress)
{
    // Any non-zero result means we consider the object to be visible.  Only at 0 do we consider the object to be
    // invisible.   The invisible value we use (HIDDEN vs. COLLAPSE) depends on the specified from/to values.
    double fromVal = from == VISIBLE ? 1. : 0.;
    double toVal = to == VISIBLE ? 1. : 0.;
    if (fromVal == toVal)
        return to;
    double result = blendFunc(fromVal, toVal, progress);
    return result > 0. ? VISIBLE : (to != VISIBLE ? to : from);
}

#pragma mark -

// return true if there is anything to do
void CompositeAnimation::updateTransitions(RenderObject* renderer, RenderStyle* currentStyle, RenderStyle* targetStyle)
{
    const Transition* anim = targetStyle->transitions();
    
    // nothing to do if we don't have any transitions or if the current and target transitions are the same
    if (m_transitions.isEmpty() && !anim)
        return;
    
    int numAnims = 0;
    bool transitionsChanged = false;
    
    // see if the lists match
    for ( ; anim; anim = anim->next()) {
        ++numAnims;
        ImplicitAnimation* implAnim = (anim->transitionProperty() == CSS_PROP_INVALID) ? 0 :
                                                m_transitions.get(anim->transitionProperty());
        if (!implAnim || !implAnim->animationsMatch(anim)) {
            transitionsChanged = true;
            break;
        }
    }
    
    if (!transitionsChanged && m_transitions.size() != numAnims)
        transitionsChanged = true;

    if (!transitionsChanged)
        return;

    // transitions have changed, update the list
    resetTransitions(renderer);
    
    if (!targetStyle->transitions())
        return;

    // add all the new animations
    for (anim = targetStyle->transitions(); anim; anim = anim->next()) {

        int property        = anim->transitionProperty();
        double duration     = anim->transitionDuration();
        int iterationCount  = anim->transitionIterationCount();
        double delay        = anim->animationDelay();

        if (property && (duration || delay) && iterationCount && !m_transitions.contains(property)) {
            ImplicitAnimation* animation = new ImplicitAnimation(const_cast<Transition*>(anim), renderer, this);
            m_transitions.set(property, animation);
        }
    }
}

// return true if there is anything to do
void CompositeAnimation::updateKeyframeAnimations(RenderObject* renderer, RenderStyle* currentStyle, RenderStyle* targetStyle)
{
    const Transition* anim = targetStyle->animations();

    // nothing to do if we don't have any animations or if the current and target animations are the same
    if ((m_keyframeAnimations.isEmpty() && !anim) || 
                (currentStyle && currentStyle->animations() && *(currentStyle->animations()) == *anim))
        return;
        
    size_t numAnims = 0;
    bool animsChanged = false;
    
    // see if the lists match
    for ( ; anim; anim = anim->next()) {
        if (!anim->isValidAnimation()) {
            animsChanged = true;
        } else {
            KeyframeAnimation* kfAnim = (numAnims < m_keyframeAnimations.size()) ? m_keyframeAnimations[numAnims] : 0;
            if (!kfAnim || !kfAnim->animationsMatch(anim))
                animsChanged = true;
            else {
                if (anim) {
                    // animations match, but play states may differ. update if needed
                    kfAnim->updatePlayState(anim->animationPlayState() == AnimPlayStatePlaying);
                    
                    // set the saved animation to this new one, just in case the play state has changed
                    kfAnim->setAnimation(anim);
                }
            }
        }
        ++numAnims;
    }
    
    if (!animsChanged && m_keyframeAnimations.size() != numAnims)
        animsChanged = true;

    if (!animsChanged)
        return;

    // animations have changed, update the list
    resetAnimations(renderer);

    if (!targetStyle->animations())
        return;

    // add all the new animations
    int index = 0;
    for (anim = targetStyle->animations(); anim; anim = anim->next(), ++index) {

        if (!anim->isValidAnimation())
            continue;
            
        // don't bother adding the animation if it has no keyframes or won't animate
        if ((anim->transitionDuration() || anim->animationDelay()) && anim->transitionIterationCount() &&
                                            anim->keyframeList().get() && !anim->keyframeList()->isEmpty()) {
            m_keyframeAnimations.append(new KeyframeAnimation(const_cast<Transition*>(anim), renderer, index, this));
        }
    }
}

int CompositeAnimation::findKeyframeAnimation(const AtomicString& name)
{
    for (int i = 0; i < (int) m_keyframeAnimations.size(); ++i)
        if (m_keyframeAnimations[i]->name() == name)
            return i;
    return -1;
}

RenderStyle* CompositeAnimation::animate(RenderObject* renderer, RenderStyle* currentStyle, RenderStyle* targetStyle)
{
    RenderStyle* resultStyle = 0;

    // we don't do any transitions if we don't have a currentStyle (on startup)
    updateTransitions(renderer, currentStyle, targetStyle);
        
    if (currentStyle) {
        // Now that we have transition objects ready, let them know about the new goal state.  We want them
        // to fill in a RenderStyle*& only if needed.
        CSSPropertyTransitionsMap::const_iterator end = m_transitions.end();
        for (CSSPropertyTransitionsMap::const_iterator it = m_transitions.begin(); it != end; ++it) {
            ImplicitAnimation*  anim = it->second;
            if (anim)
                anim->animate(this, renderer, currentStyle, targetStyle, resultStyle);
        }
    }

    updateKeyframeAnimations(renderer, currentStyle, targetStyle);
    
    // Now that we have animation objects ready, let them know about the new goal state.  We want them
    // to fill in a RenderStyle*& only if needed.
    Vector<KeyframeAnimation*>::const_iterator kfend = m_keyframeAnimations.end();
    for (Vector<KeyframeAnimation*>::const_iterator it = m_keyframeAnimations.begin(); it != kfend; ++it) {
        KeyframeAnimation* anim = *it;
        if (anim)
            anim->animate(this, renderer, currentStyle, targetStyle, resultStyle);
    }
    
    return resultStyle ? resultStyle : targetStyle;
}

// "animating" means that something is running that requires the timer to keep firing
// (e.g. a software animation)
void CompositeAnimation::setAnimating(bool inAnimating)
{
    CSSPropertyTransitionsMap::const_iterator end = m_transitions.end();
    for (CSSPropertyTransitionsMap::const_iterator it = m_transitions.begin(); it != end; ++it) {
        ImplicitAnimation*  transition = it->second;
        transition->setAnimating(inAnimating);
    }    

    Vector<KeyframeAnimation*>::const_iterator kfend = m_keyframeAnimations.end();
    for (Vector<KeyframeAnimation*>::const_iterator it = m_keyframeAnimations.begin(); it != kfend; ++it) {
        KeyframeAnimation* anim = *it;
        anim->setAnimating(inAnimating);
    }
}

bool CompositeAnimation::animating()
{
    CSSPropertyTransitionsMap::const_iterator end = m_transitions.end();
    for (CSSPropertyTransitionsMap::const_iterator it = m_transitions.begin(); it != end; ++it) {
        ImplicitAnimation*  transition = it->second;
        if (transition && transition->animating() && transition->running())
            return true;
    }    

    Vector<KeyframeAnimation*>::const_iterator kfend = m_keyframeAnimations.end();
    for (Vector<KeyframeAnimation*>::const_iterator it = m_keyframeAnimations.begin(); it != kfend; ++it) {
        KeyframeAnimation* anim = *it;
        if (anim && anim->running() && anim->animating() && anim->active())
            return true;
    }
    return false;
}

void CompositeAnimation::resetTransitions(RenderObject* renderer)
{
    CSSPropertyTransitionsMap::const_iterator end = m_transitions.end();
    for (CSSPropertyTransitionsMap::const_iterator it = m_transitions.begin(); it != end; ++it) {
        ImplicitAnimation*  transition = it->second;
        transition->reset(renderer);
        delete transition;
    }
    m_transitions.clear();
}

void CompositeAnimation::resetAnimations(RenderObject* renderer)
{
    Vector<KeyframeAnimation*>::const_iterator kfend = m_keyframeAnimations.end();
    for (Vector<KeyframeAnimation*>::const_iterator it = m_keyframeAnimations.begin(); it != kfend; ++it) {
        KeyframeAnimation* anim = *it;
        delete anim;
    }
    m_keyframeAnimations.clear();
}

void CompositeAnimation::cleanupFinishedAnimations(RenderObject* renderer)
{
    if (suspended())
        return;
        
    // reset transitions
    CSSPropertyTransitionsMap::const_iterator end = m_transitions.end();
    for (CSSPropertyTransitionsMap::const_iterator it = m_transitions.begin(); it != end; ++it) {
        ImplicitAnimation* anim = it->second;
        if (!anim)
            continue;
        if (anim->postactive() && !anim->waitingForEndEvent()) {
            anim->updateStateMachine(anim->STATE_INPUT_MAKE_NEW, -1);
            anim->reset(renderer);
        }
    }
    
    // make a list of animations to be deleted
    Vector<int> finishedAnimations;

    for (int i = 0; i < (int) m_keyframeAnimations.size(); ++i) {
        KeyframeAnimation* anim = m_keyframeAnimations[i];
        if (!anim)
            continue;
        if (anim->postactive() && !anim->waitingForEndEvent())
            finishedAnimations.append(i);
    }
    
    // delete them
    for (Vector<int>::iterator it = finishedAnimations.begin(); it != finishedAnimations.end(); ++it) {
        m_keyframeAnimations[*it]->reset(renderer);
        delete m_keyframeAnimations[*it];
        m_keyframeAnimations.remove(*it);
    }
}

void CompositeAnimation::setAnimationStartTime(double t)
{
    // set start time on all animations waiting for it
    Vector<KeyframeAnimation*>::const_iterator kfend = m_keyframeAnimations.end();
    for (Vector<KeyframeAnimation*>::const_iterator it = m_keyframeAnimations.begin(); it != kfend; ++it) {
        KeyframeAnimation* anim = *it;
        if (anim && anim->waitingForStartTime())
            anim->updateStateMachine(AnimationBase::STATE_INPUT_START_TIME_SET, t);
    }
}

void CompositeAnimation::setTransitionStartTime(int property, double t)
{
    // set start time for given property transition
    CSSPropertyTransitionsMap::const_iterator end = m_transitions.end();
    for (CSSPropertyTransitionsMap::const_iterator it = m_transitions.begin(); it != end; ++it) {
        ImplicitAnimation* anim = it->second;
        if (anim && anim->waitingForStartTime() && 
                    (anim->transitionProperty() == property || anim->transitionProperty() == cAnimateAll))
            anim->updateStateMachine(AnimationBase::STATE_INPUT_START_TIME_SET, t);
    }
}

void CompositeAnimation::suspendAnimations()
{
    if (m_suspended)
        return;
    
    m_suspended = true;
    
    Vector<KeyframeAnimation*>::const_iterator kfend = m_keyframeAnimations.end();
    for (Vector<KeyframeAnimation*>::const_iterator it = m_keyframeAnimations.begin(); it != kfend; ++it) {
        KeyframeAnimation* anim = *it;
        if (anim) {
            anim->updatePlayState(false);
            anim->renderer()->suspendAnimations();
        }
    }
    
    CSSPropertyTransitionsMap::const_iterator end = m_transitions.end();
    for (CSSPropertyTransitionsMap::const_iterator it = m_transitions.begin(); it != end; ++it) {
        ImplicitAnimation* anim = it->second;
        if (anim && anim->hasStyle()) {
            anim->updatePlayState(false);
            anim->renderer()->suspendAnimations();
        }
    }
}

void CompositeAnimation::resumeAnimations()
{
    if (!m_suspended)
        return;
    
    m_suspended = false;
    
    Vector<KeyframeAnimation*>::const_iterator kfend = m_keyframeAnimations.end();
    for (Vector<KeyframeAnimation*>::const_iterator it = m_keyframeAnimations.begin(); it != kfend; ++it) {
        KeyframeAnimation* anim = *it;
        if (anim && anim->playStatePlaying()) {
            anim->updatePlayState(true);
            anim->renderer()->resumeAnimations();
        }
    }
    
    CSSPropertyTransitionsMap::const_iterator end = m_transitions.end();
    for (CSSPropertyTransitionsMap::const_iterator it = m_transitions.begin(); it != end; ++it) {
        ImplicitAnimation* anim = it->second;
        if (anim && anim->hasStyle()) {
            anim->updatePlayState(true);
            anim->renderer()->resumeAnimations();
        }
    }
}

void CompositeAnimation::overrideImplicitAnimations(int property)
{
    CSSPropertyTransitionsMap::const_iterator end = m_transitions.end();
    for (CSSPropertyTransitionsMap::const_iterator it = m_transitions.begin(); it != end; ++it) {
        ImplicitAnimation* anim = it->second;
        if (anim && (anim->transitionProperty() == property || anim->transitionProperty() == cAnimateAll))
            anim->setOverridden(true);
    }
}

void CompositeAnimation::resumeOverriddenImplicitAnimations(int property)
{
    CSSPropertyTransitionsMap::const_iterator end = m_transitions.end();
    for (CSSPropertyTransitionsMap::const_iterator it = m_transitions.begin(); it != end; ++it) {
        ImplicitAnimation* anim = it->second;
        if (anim && (anim->transitionProperty() == property || anim->transitionProperty() == cAnimateAll))
            anim->setOverridden(false);
    }
}

void CompositeAnimation::styleIsSetup()
{
    if (m_numStyleIsSetupWaiters == 0)
        return;
        
    Vector<KeyframeAnimation*>::const_iterator kfend = m_keyframeAnimations.end();
    for (Vector<KeyframeAnimation*>::const_iterator it = m_keyframeAnimations.begin(); it != kfend; ++it) {
        KeyframeAnimation* anim = *it;
        if (anim && anim->waitingForStyleSetup())
            anim->updateStateMachine(AnimationBase::STATE_INPUT_STYLE_SETUP, -1);
    }
    
    CSSPropertyTransitionsMap::const_iterator end = m_transitions.end();
    for (CSSPropertyTransitionsMap::const_iterator it = m_transitions.begin(); it != end; ++it) {
        ImplicitAnimation* anim = it->second;
        if (anim && anim->waitingForStyleSetup())
            anim->updateStateMachine(AnimationBase::STATE_INPUT_STYLE_SETUP, -1);
    }
}

bool CompositeAnimation::isAnimatingProperty(int property) const
{
    Vector<KeyframeAnimation*>::const_iterator kfend = m_keyframeAnimations.end();
    for (Vector<KeyframeAnimation*>::const_iterator it = m_keyframeAnimations.begin(); it != kfend; ++it) {
        KeyframeAnimation* anim = *it;
        if (anim && anim->isAnimatingProperty(property))
            return true;
    }
    
    CSSPropertyTransitionsMap::const_iterator end = m_transitions.end();
    for (CSSPropertyTransitionsMap::const_iterator it = m_transitions.begin(); it != end; ++it) {
        ImplicitAnimation* anim = it->second;
        if (anim && anim->isAnimatingProperty(property))
            return true;
    }
    return false;
}

#pragma mark -

class PropertyWrapperBase {
public:
    PropertyWrapperBase(int prop)
    : m_prop(prop)
    { }
    virtual ~PropertyWrapperBase() { }
    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const=0;
    virtual void blend(RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double prog) const=0;
    
    int property() const { return m_prop; }
    virtual bool hardwareAnimated() const { return false; }

private:
    int m_prop;
};

template <typename T>
class PropertyWrapperGetter : public PropertyWrapperBase {
public:
    PropertyWrapperGetter(int prop, T (RenderStyle::*getter)() const)
    : PropertyWrapperBase(prop)
    , m_getter(getter)
    { }
    
    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        return (a->*m_getter)() == (b->*m_getter)();
    }
    
protected:
    T (RenderStyle::*m_getter)() const;
};

template <typename T>
class PropertyWrapper : public PropertyWrapperGetter<T> {
public:
    PropertyWrapper(int prop, T (RenderStyle::*getter)() const, void (RenderStyle::*setter)(T))
    : PropertyWrapperGetter<T>(prop, getter)
    , m_setter(setter)
    { }
    
    virtual void blend(RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double prog) const
    {
        (dst->*m_setter)(blendFunc((a->*PropertyWrapperGetter<T>::m_getter)(), (b->*PropertyWrapperGetter<T>::m_getter)(), prog));
    }
    
protected:
    void (RenderStyle::*m_setter)(T);
};

class PropertyWrapperHWOpacity : public PropertyWrapper<float> {
public:
    PropertyWrapperHWOpacity()
    : PropertyWrapper<float>(CSS_PROP_OPACITY, &RenderStyle::opacity, &RenderStyle::setOpacity)
    { }
    
    virtual bool hardwareAnimated() const { return true; }

    virtual void blend(RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double prog) const
    {
        float fromOpacity = a->opacity();
        
        // this makes sure we put the object being animated into a RenderLayer during the animation
        dst->setOpacity(blendFunc((fromOpacity == 1) ? 0.999999f : fromOpacity, b->opacity(), prog));
    }
};

class PropertyWrapperHWTransform : public PropertyWrapper<const TransformOperations&> {
public:
    PropertyWrapperHWTransform()
    : PropertyWrapper<const TransformOperations&>(CSS_PROP__WEBKIT_TRANSFORM, &RenderStyle::transform, &RenderStyle::setTransform)
    { }
    
    virtual bool hardwareAnimated() const { return true; }

    virtual void blend(RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double prog) const
    {
        const TransformOperations& t = a->transform();

        // this makes sure we put the object being animated into a RenderLayer during the animation
        dst->setTransform(blendFunc(t.isEmpty() ? TransformOperations(true) : t, b->transform(), prog));
    }
};

class PropertyWrapperShadow : public PropertyWrapperGetter<ShadowData*> {
public:
    PropertyWrapperShadow(int prop, ShadowData* (RenderStyle::*getter)() const, void (RenderStyle::*setter)(ShadowData*, bool))
    : PropertyWrapperGetter<ShadowData*>(prop, getter)
    , m_setter(setter)
    { }
    
    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        ShadowData* shadowa = (a->*m_getter)();
        ShadowData* shadowb = (b->*m_getter)();

        if (!shadowa && shadowb || shadowa && !shadowb)
            return false;
        if (shadowa && shadowb && (*shadowa != *shadowb))
            return false;
        return true;
    }
    
    virtual void blend(RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double prog) const
    {
        ShadowData* shadowa = (a->*m_getter)();
        ShadowData* shadowb = (b->*m_getter)();
        ShadowData defaultShadowData;
        
        if (!shadowa)
            shadowa = &defaultShadowData;
        if (!shadowb)
            shadowb = &defaultShadowData;
        
        (dst->*m_setter)(blendFunc(shadowa, shadowb, prog), false);
    }
    
private:
    void (RenderStyle::*m_setter)(ShadowData*, bool);
};

class PropertyWrapperMaybeInvalidColor : public PropertyWrapperBase {
public:
    PropertyWrapperMaybeInvalidColor(int prop, const Color& (RenderStyle::*getter)() const, void (RenderStyle::*setter)(const Color&))
    : PropertyWrapperBase(prop)
    , m_getter(getter)
    , m_setter(setter)
    { }
    
    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        Color fromColor = (a->*m_getter)();
        Color toColor = (b->*m_getter)();
        if (!fromColor.isValid())
            fromColor = a->color();
        if (!toColor.isValid())
            toColor = b->color();
            
        return fromColor == toColor;
    }
    
    virtual void blend(RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double prog) const
    {
        Color fromColor = (a->*m_getter)();
        Color toColor = (b->*m_getter)();
        if (!fromColor.isValid())
            fromColor = a->color();
        if (!toColor.isValid())
            toColor = b->color();
        (dst->*m_setter)(blendFunc(fromColor, toColor, prog));
    }
    
private:
    const Color& (RenderStyle::*m_getter)() const;
    void (RenderStyle::*m_setter)(const Color&);
};

class AnimationControllerPrivate {
public:
    AnimationControllerPrivate(Frame*);
    ~AnimationControllerPrivate();

    CompositeAnimation* accessCompositeAnimation(RenderObject*);
    bool clear(RenderObject*);
    
    void swAnimationTimerFired(Timer<AnimationControllerPrivate>*);
    void updateSWAnimationTimer();
    
    void updateRenderingDispatcherFired(Timer<AnimationControllerPrivate>*);
    void startUpdateRenderingDispatcher();

    bool hasAnimations() const { return !m_compositeAnimations.isEmpty(); }

    void suspendAnimations(Document* document);
    void resumeAnimations(Document* document);
    
    void cleanupFinishedAnimations();
    
    void styleIsSetup();

    bool isAnimatingPropertyOnRenderer(RenderObject* obj, int property) const;

    static bool propertiesEqual(int prop, const RenderStyle* a, const RenderStyle* b);
    
    // return true if we need to start software animation timers
    static bool blendProperties(int prop, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double prog);
    
    void setWaitingForStyleIsSetup(bool waiting)    { if (waiting) m_numStyleIsSetupWaiters++; else m_numStyleIsSetupWaiters--; }
    
private:
    static void ensurePropertyMap();
    
    typedef HashMap<RenderObject*, CompositeAnimation*> RenderObjectAnimationMap;
    
    RenderObjectAnimationMap            m_compositeAnimations;
    Timer<AnimationControllerPrivate>   m_swAnimationTimer;
    Timer<AnimationControllerPrivate>   m_updateRenderingDispatcher;
    Frame* m_frame;
    uint32_t                            m_numStyleIsSetupWaiters;
    
    static Vector<PropertyWrapperBase*>* g_propertyWrappers;
    static int g_propertyWrapperMap[numCSSProperties];
};

#pragma mark -

Vector<PropertyWrapperBase*>* AnimationControllerPrivate::g_propertyWrappers = 0;
int AnimationControllerPrivate::g_propertyWrapperMap[];

AnimationControllerPrivate::AnimationControllerPrivate(Frame* frame)
    : m_swAnimationTimer(this, &AnimationControllerPrivate::swAnimationTimerFired)
    , m_updateRenderingDispatcher(this, &AnimationControllerPrivate::updateRenderingDispatcherFired)
    , m_frame(frame)
    , m_numStyleIsSetupWaiters(0)
{
    ensurePropertyMap();
}

AnimationControllerPrivate::~AnimationControllerPrivate()
{
    deleteAllValues(m_compositeAnimations);
}

// static
void AnimationControllerPrivate::ensurePropertyMap()
{
    // FIXME: This data is never destroyed. Maybe we should ref count it and toss it when the last AnimationController is destroyed?
    if (g_propertyWrappers == 0) {
        g_propertyWrappers = new Vector<PropertyWrapperBase*>();
        
        // build the list of property wrappers to do the comparisons and blends
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_LEFT, &RenderStyle::left, &RenderStyle::setLeft));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_RIGHT, &RenderStyle::right, &RenderStyle::setRight));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_TOP, &RenderStyle::top, &RenderStyle::setTop));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_BOTTOM, &RenderStyle::bottom, &RenderStyle::setBottom));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_WIDTH, &RenderStyle::width, &RenderStyle::setWidth));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_HEIGHT, &RenderStyle::height, &RenderStyle::setHeight));
        g_propertyWrappers->append(new PropertyWrapper<unsigned short>(CSS_PROP_BORDER_LEFT_WIDTH, &RenderStyle::borderLeftWidth, &RenderStyle::setBorderLeftWidth));
        g_propertyWrappers->append(new PropertyWrapper<unsigned short>(CSS_PROP_BORDER_RIGHT_WIDTH, &RenderStyle::borderRightWidth, &RenderStyle::setBorderRightWidth));
        g_propertyWrappers->append(new PropertyWrapper<unsigned short>(CSS_PROP_BORDER_TOP_WIDTH, &RenderStyle::borderTopWidth, &RenderStyle::setBorderTopWidth));
        g_propertyWrappers->append(new PropertyWrapper<unsigned short>(CSS_PROP_BORDER_BOTTOM_WIDTH, &RenderStyle::borderBottomWidth, &RenderStyle::setBorderBottomWidth));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_MARGIN_LEFT, &RenderStyle::marginLeft, &RenderStyle::setMarginLeft));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_MARGIN_RIGHT, &RenderStyle::marginRight, &RenderStyle::setMarginRight));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_MARGIN_TOP, &RenderStyle::marginTop, &RenderStyle::setMarginTop));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_MARGIN_BOTTOM, &RenderStyle::marginBottom, &RenderStyle::setMarginBottom));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_PADDING_LEFT, &RenderStyle::paddingLeft, &RenderStyle::setPaddingLeft));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_PADDING_RIGHT, &RenderStyle::paddingRight, &RenderStyle::setPaddingRight));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_PADDING_TOP, &RenderStyle::paddingTop, &RenderStyle::setPaddingTop));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_PADDING_BOTTOM, &RenderStyle::paddingBottom, &RenderStyle::setPaddingBottom));
        g_propertyWrappers->append(new PropertyWrapper<const Color&>(CSS_PROP_COLOR, &RenderStyle::color, &RenderStyle::setColor));
        g_propertyWrappers->append(new PropertyWrapper<const Color&>(CSS_PROP_BACKGROUND_COLOR, &RenderStyle::backgroundColor, &RenderStyle::setBackgroundColor));
        g_propertyWrappers->append(new PropertyWrapper<unsigned short>(CSS_PROP__WEBKIT_COLUMN_RULE_WIDTH, &RenderStyle::columnRuleWidth, &RenderStyle::setColumnRuleWidth));
        g_propertyWrappers->append(new PropertyWrapper<float>(CSS_PROP__WEBKIT_COLUMN_GAP, &RenderStyle::columnGap, &RenderStyle::setColumnGap));
        g_propertyWrappers->append(new PropertyWrapper<unsigned short>(CSS_PROP__WEBKIT_COLUMN_COUNT, &RenderStyle::columnCount, &RenderStyle::setColumnCount));
        g_propertyWrappers->append(new PropertyWrapper<float>(CSS_PROP__WEBKIT_COLUMN_WIDTH, &RenderStyle::columnWidth, &RenderStyle::setColumnWidth));
        g_propertyWrappers->append(new PropertyWrapper<short>(CSS_PROP__WEBKIT_BORDER_HORIZONTAL_SPACING, &RenderStyle::horizontalBorderSpacing, &RenderStyle::setHorizontalBorderSpacing));
        g_propertyWrappers->append(new PropertyWrapper<short>(CSS_PROP__WEBKIT_BORDER_VERTICAL_SPACING, &RenderStyle::verticalBorderSpacing, &RenderStyle::setVerticalBorderSpacing));
        g_propertyWrappers->append(new PropertyWrapper<int>(CSS_PROP_Z_INDEX, &RenderStyle::zIndex, &RenderStyle::setZIndex));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP_LINE_HEIGHT, &RenderStyle::lineHeight, &RenderStyle::setLineHeight));
        g_propertyWrappers->append(new PropertyWrapper<int>(CSS_PROP_OUTLINE_OFFSET, &RenderStyle::outlineOffset, &RenderStyle::setOutlineOffset));
        g_propertyWrappers->append(new PropertyWrapper<unsigned short>(CSS_PROP_OUTLINE_WIDTH, &RenderStyle::outlineWidth, &RenderStyle::setOutlineWidth));
        g_propertyWrappers->append(new PropertyWrapper<float>(CSS_PROP_LETTER_SPACING, &RenderStyle::letterSpacing, &RenderStyle::setLetterSpacing));
        g_propertyWrappers->append(new PropertyWrapper<int>(CSS_PROP_WORD_SPACING, &RenderStyle::wordSpacing, &RenderStyle::setWordSpacing));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP__WEBKIT_TRANSFORM_ORIGIN_X, &RenderStyle::transformOriginX, &RenderStyle::setTransformOriginX));
        g_propertyWrappers->append(new PropertyWrapper<Length>(CSS_PROP__WEBKIT_TRANSFORM_ORIGIN_Y, &RenderStyle::transformOriginY, &RenderStyle::setTransformOriginY));
        g_propertyWrappers->append(new PropertyWrapper<float>(CSS_PROP__WEBKIT_TRANSFORM_ORIGIN_Z, &RenderStyle::transformOriginZ, &RenderStyle::setTransformOriginZ));
        g_propertyWrappers->append(new PropertyWrapper<const IntSize&>(CSS_PROP__WEBKIT_BORDER_TOP_LEFT_RADIUS, &RenderStyle::borderTopLeftRadius, &RenderStyle::setBorderTopLeftRadius));
        g_propertyWrappers->append(new PropertyWrapper<const IntSize&>(CSS_PROP__WEBKIT_BORDER_TOP_RIGHT_RADIUS, &RenderStyle::borderTopRightRadius, &RenderStyle::setBorderTopRightRadius));
        g_propertyWrappers->append(new PropertyWrapper<const IntSize&>(CSS_PROP__WEBKIT_BORDER_BOTTOM_LEFT_RADIUS, &RenderStyle::borderBottomLeftRadius, &RenderStyle::setBorderBottomLeftRadius));
        g_propertyWrappers->append(new PropertyWrapper<const IntSize&>(CSS_PROP__WEBKIT_BORDER_BOTTOM_RIGHT_RADIUS, &RenderStyle::borderBottomRightRadius, &RenderStyle::setBorderBottomRightRadius));
        g_propertyWrappers->append(new PropertyWrapper<EVisibility>(CSS_PROP_VISIBILITY, &RenderStyle::visibility, &RenderStyle::setVisibility));

        // these might be hardware accelerated
#if ENABLE(HW_COMP)
        g_propertyWrappers->append(new PropertyWrapperHWOpacity());
        g_propertyWrappers->append(new PropertyWrapperHWTransform());
#else
        g_propertyWrappers->append(new PropertyWrapper<float>(CSS_PROP_OPACITY, &RenderStyle::opacity, &RenderStyle::setOpacity));
        g_propertyWrappers->append(new PropertyWrapper<const TransformOperations&>(CSS_PROP__WEBKIT_TRANSFORM, &RenderStyle::transform, &RenderStyle::setTransform));
#endif

        // FIXME: these might be invalid colors, need to check for that
        g_propertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSS_PROP__WEBKIT_COLUMN_RULE_COLOR, &RenderStyle::columnRuleColor, &RenderStyle::setColumnRuleColor));
        g_propertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSS_PROP__WEBKIT_TEXT_STROKE_COLOR, &RenderStyle::textStrokeColor, &RenderStyle::setTextStrokeColor));
        g_propertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSS_PROP__WEBKIT_TEXT_FILL_COLOR, &RenderStyle::textFillColor, &RenderStyle::setTextFillColor));
        g_propertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSS_PROP_BORDER_LEFT_COLOR, &RenderStyle::borderLeftColor, &RenderStyle::setBorderLeftColor));
        g_propertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSS_PROP_BORDER_RIGHT_COLOR, &RenderStyle::borderRightColor, &RenderStyle::setBorderRightColor));
        g_propertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSS_PROP_BORDER_TOP_COLOR, &RenderStyle::borderTopColor, &RenderStyle::setBorderTopColor));
        g_propertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSS_PROP_BORDER_BOTTOM_COLOR, &RenderStyle::borderBottomColor, &RenderStyle::setBorderBottomColor));
        g_propertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSS_PROP_OUTLINE_COLOR, &RenderStyle::outlineColor, &RenderStyle::setOutlineColor));

        // these are for shadows
        g_propertyWrappers->append(new PropertyWrapperShadow(CSS_PROP__WEBKIT_BOX_SHADOW, &RenderStyle::boxShadow, &RenderStyle::setBoxShadow));
        g_propertyWrappers->append(new PropertyWrapperShadow(CSS_PROP_TEXT_SHADOW, &RenderStyle::textShadow, &RenderStyle::setTextShadow));

        // make sure unused slots have a value
        for (unsigned int i = 0; i < (unsigned int) numCSSProperties; ++i)
            g_propertyWrapperMap[i] = -1;
        
        size_t n = g_propertyWrappers->size();
        for (unsigned int i = 0; i < n; ++i)
            g_propertyWrapperMap[(*g_propertyWrappers)[i]->property()] = i;
    }
}

// static
bool AnimationControllerPrivate::propertiesEqual(int prop, const RenderStyle* a, const RenderStyle* b)
{
    if (prop == cAnimateAll) {
        size_t n = g_propertyWrappers->size();
        for (unsigned int i = 0; i < n; ++i) {
            if (!(*g_propertyWrappers)[i]->equals(a, b))
                return false;
        }
    }
    else if (prop >= 0 && prop < numCSSProperties) {
        int i = g_propertyWrapperMap[prop];
        return (i >= 0) ? (*g_propertyWrappers)[i]->equals(a, b) : true;
    }
    return true;
}

// static - return true if we need to start software animation timers
bool AnimationControllerPrivate::blendProperties(int prop, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double prog)
{
    if (prop == cAnimateAll) {
        bool needSWAnim = false;
    
        size_t n = g_propertyWrappers->size();
        for (unsigned int i = 0; i < n; ++i) {
            PropertyWrapperBase* wrapper = (*g_propertyWrappers)[i];
            if (!wrapper->equals(a, b)) {
                wrapper->blend(dst, a, b, prog);
                needSWAnim |= !wrapper->hardwareAnimated();
            }
        }
        return needSWAnim;
    }
    
    if (prop >= 0 && prop < numCSSProperties) {
        int i = g_propertyWrapperMap[prop];
        if (i >= 0) {
            PropertyWrapperBase* wrapper = (*g_propertyWrappers)[i];
            wrapper->blend(dst, a, b, prog);
            return !wrapper->hardwareAnimated();
        }
    }
    
    // return true, even though we didn't animate anything, so we don't try to start any SW animation timers
    return false;
}

CompositeAnimation* AnimationControllerPrivate::accessCompositeAnimation(RenderObject* renderer)
{
    CompositeAnimation* animation = m_compositeAnimations.get(renderer);
    if (!animation) {
        animation = new CompositeAnimation(!renderer->document()->loadComplete(), this);
        m_compositeAnimations.set(renderer, animation);
    }
    return animation;
}

bool AnimationControllerPrivate::clear(RenderObject* renderer)
{
    // return false if we didn't do anything OR we are suspended (so we don't try to
    // do a setChanged() when suspended
    CompositeAnimation* animation = m_compositeAnimations.take(renderer);
    if (!animation)
        return false;
    animation->resetTransitions(renderer);
    bool wasSuspended = animation->suspended();
    delete animation;
    return !wasSuspended;
}

void AnimationControllerPrivate::cleanupFinishedAnimations()
{
    RenderObjectAnimationMap::const_iterator animationsEnd = m_compositeAnimations.end();
    for (RenderObjectAnimationMap::const_iterator it = m_compositeAnimations.begin(); 
                                                  it != animationsEnd; ++it) {
        RenderObject* renderer = it->first;
        CompositeAnimation* compAnim = it->second;
        compAnim->cleanupFinishedAnimations(renderer);
    }
}

void AnimationControllerPrivate::styleIsSetup()
{
    if (m_numStyleIsSetupWaiters == 0)
        return;

    RenderObjectAnimationMap::const_iterator animationsEnd = m_compositeAnimations.end();
    for (RenderObjectAnimationMap::const_iterator it = m_compositeAnimations.begin(); 
                                                  it != animationsEnd; ++it) {
        it->second->styleIsSetup();
    }
}

void AnimationControllerPrivate::updateSWAnimationTimer()
{
    bool animating = false;

    RenderObjectAnimationMap::const_iterator animationsEnd = m_compositeAnimations.end();
    for (RenderObjectAnimationMap::const_iterator it = m_compositeAnimations.begin(); 
                                                  it != animationsEnd; ++it) {
        CompositeAnimation* compAnim = it->second;
        if (!compAnim->suspended() && compAnim->animating()) {
            animating = true;
            break;
        }
    }
    
    if (animating) {
        if (!m_swAnimationTimer.isActive())
            m_swAnimationTimer.startRepeating(cAnimationTimerDelay);
    } else if (m_swAnimationTimer.isActive())
        m_swAnimationTimer.stop();
}

void AnimationControllerPrivate::updateRenderingDispatcherFired(Timer<AnimationControllerPrivate>*)
{
    if (m_frame && m_frame->document())
        m_frame->document()->updateRendering();
}

void AnimationControllerPrivate::startUpdateRenderingDispatcher()
{
    if (!m_updateRenderingDispatcher.isActive())
        m_updateRenderingDispatcher.startOneShot(0);
}

void AnimationControllerPrivate::swAnimationTimerFired(Timer<AnimationControllerPrivate>* timer)
{
    //cleanupFinishedAnimations();
    
    // When the timer fires, all we do is call setChanged on all DOM nodes with running animations and then do an immediate
    // updateRendering.  It will then call back to us with new information.
    bool animating = false;
    RenderObjectAnimationMap::const_iterator animationsEnd = m_compositeAnimations.end();
    for (RenderObjectAnimationMap::const_iterator it = m_compositeAnimations.begin(); 
                                                  it != animationsEnd; ++it) {
        RenderObject* renderer = it->first;
        CompositeAnimation* compAnim = it->second;
        if (!compAnim->suspended() && compAnim->animating()) {
            animating = true;
            compAnim->setAnimating(false);
            setChanged(renderer->element());
        }
    }
    
    m_frame->document()->updateRendering();
    
    updateSWAnimationTimer();
}

bool AnimationControllerPrivate::isAnimatingPropertyOnRenderer(RenderObject* obj, int property) const
{
    CompositeAnimation* animation = m_compositeAnimations.get(obj);
    if (!animation) return false;

    return animation->isAnimatingProperty(property);
}

void AnimationControllerPrivate::suspendAnimations(Document* document)
{
    RenderObjectAnimationMap::const_iterator animationsEnd = m_compositeAnimations.end();
    for (RenderObjectAnimationMap::const_iterator it = m_compositeAnimations.begin(); 
                                                  it != animationsEnd; ++it) {
        RenderObject* renderer = it->first;
        CompositeAnimation* compAnim = it->second;
        if (renderer->document() == document)
            compAnim->suspendAnimations();
    }

    updateSWAnimationTimer();
}

void AnimationControllerPrivate::resumeAnimations(Document* document)
{
    RenderObjectAnimationMap::const_iterator animationsEnd = m_compositeAnimations.end();
    for (RenderObjectAnimationMap::const_iterator it = m_compositeAnimations.begin(); 
                                                  it != animationsEnd; ++it) {
        RenderObject* renderer = it->first;
        CompositeAnimation* compAnim = it->second;
        if (renderer->document() == document)
            compAnim->resumeAnimations();
    }

    updateSWAnimationTimer();
}

#pragma mark -

void ImplicitAnimation::animate(CompositeAnimation* animation, RenderObject* renderer, const RenderStyle* currentStyle, 
                                    const RenderStyle* targetStyle, RenderStyle*& ioAnimatedStyle)
{
    if (!running() ||  !m_object->document()->loadComplete())
        return;
        
    // If we're not running any animations and there is no change in our prop, just return 
    if (isnew() && AnimationControllerPrivate::propertiesEqual(m_property, currentStyle, targetStyle))
        return;
        
    // If we get this far and the animation is done, it means we are cleaning up a just finished animation.
    // If so, we need to reset and send back the targetStyle
    if (postactive()) {
        updateStateMachine(STATE_INPUT_MAKE_NEW, -1);
        reset(renderer);

        if (!ioAnimatedStyle)
            ioAnimatedStyle = const_cast<RenderStyle*>(targetStyle);
        return;
    }

    // If we are running and the targetStyle value has changed from our current toStyle value, we need to reset.
    // We also need to reset if we are new.
    if (isnew() || !m_toStyle || !AnimationControllerPrivate::propertiesEqual(m_property, m_toStyle, targetStyle)) {
        // Opacity is special. If we are animating it with hardware, The value at this moment will not be in
        // currentStyle because the hardware is doing the animation and the RenderStyle is not getting updated.
        // This is not a problem for transforms, which can also be animated in hardware, because when transforms
        // are animating they always have a hardware layer, which remembers the current transform value. But opacity
        // pops in and out of using hardware layers, so it forgets the current opacity value. So we need to compute
        // the blended opacity value between the previous from and to styles and put that in the currentStyle, which
        // will become the new fromStyle. This is changing a const RenderStyle, but we know what we are doing, really :-)
        if (!isnew() && m_fromStyle && (m_property == CSS_PROP_OPACITY || m_property == cAnimateAll)) {
            // get the blended value of opacity into the currentStyle (which will be the new fromStyle)
            double prog = progress(1, 0);
            AnimationControllerPrivate::blendProperties(CSS_PROP_OPACITY, const_cast<RenderStyle*>(currentStyle), m_fromStyle, m_toStyle, prog);
        }
        reset(renderer, currentStyle, targetStyle);
    }
        
    // run a cycle of animation.
    // We know we will need a new render style, so make one if needed
    if (!ioAnimatedStyle)
        ioAnimatedStyle = new (renderer->renderArena()) RenderStyle(*targetStyle);
    
    double prog = progress(1, 0);
    bool needsSWAnim = AnimationControllerPrivate::blendProperties(m_property, ioAnimatedStyle, m_fromStyle, m_toStyle, prog);
    if (needsSWAnim)
        setAnimating();
    //ASSERT(needsSWAnim != m_waitedForResponse || preactive());
}

bool ImplicitAnimation::startAnimation(double beginTime)
{
    // send transition to object
    return m_object ? m_object->startTransition(beginTime, m_property, m_fromStyle, m_toStyle) : false;
}

void ImplicitAnimation::endAnimation(bool reset)
{
    if (m_object)
        m_object->transitionFinished(m_property);
}

void ImplicitAnimation::onAnimationEnd(double inElapsedTime)
{
    // we're converting the animation into a transition here
    if (!sendTransitionEvent(EventNames::webkitTransitionEndEvent, inElapsedTime)) {
        // we didn't dispatch an event, which would call endAnimation(), so we'll just end
        // it here.
        endAnimation(true);
    }
}

bool ImplicitAnimation::sendTransitionEvent(const AtomicString& inEventType, double inElapsedTime)
{
    if (inEventType == EventNames::webkitTransitionEndEvent) {
        Document::ListenerType listenerType = Document::TRANSITIONEND_LISTENER;
        
        if (shouldSendEventForListener(listenerType)) {
            HTMLElement* element = elementForEventDispatch();
            String propertyName;
            if (m_property == cAnimateAll) {
                propertyName = String("");
            }
            else {
                propertyName = String(getPropertyName((CSSPropertyID)m_property));
            }
            if (element) {
                m_waitingForEndEvent = true;
                m_animationEventDispatcher.startTimer(element, propertyName, m_property, true, inEventType, inElapsedTime);
                return true; // did dispatch an event
            }
        }
    }
    
    return false; // didn't dispatch an event
}

void ImplicitAnimation::reset(RenderObject* renderer, const RenderStyle* from /* = 0 */, const RenderStyle* to /* = 0 */)
{
    ASSERT((!m_toStyle && !to) || m_toStyle != to);
    ASSERT((!m_fromStyle && !from) || m_fromStyle != from);
    if (m_fromStyle)
        m_fromStyle->deref(renderer->renderArena());
    if (m_toStyle)
        m_toStyle->deref(renderer->renderArena());

    m_fromStyle = const_cast<RenderStyle*>(from);   // it is read-only, other than the ref
    if (m_fromStyle)
        m_fromStyle->ref();

    m_toStyle = const_cast<RenderStyle*>(to);       // it is read-only, other than the ref
    if (m_toStyle)
        m_toStyle->ref();
        
    // restart the transition
    if (from && to)
        updateStateMachine(STATE_INPUT_RESTART_ANIMATION, -1);
}

void ImplicitAnimation::setOverridden(bool b)
{
    if (b != m_overridden) {
        m_overridden = b;
        updateStateMachine(m_overridden ? STATE_INPUT_PAUSE_OVERRIDE : STATE_INPUT_RESUME_OVERRIDE, -1);
    }
}

bool ImplicitAnimation::affectsProperty(int property) const
{
    return m_property == property ||
           (m_property == cAnimateAll && !AnimationControllerPrivate::propertiesEqual(property, m_fromStyle, m_toStyle));
}

#pragma mark -

void KeyframeAnimation::animate(CompositeAnimation* animation, RenderObject* renderer, const RenderStyle* currentStyle, 
                                    const RenderStyle* targetStyle, RenderStyle*& ioAnimatedStyle)
{
    // if we have not yet started, we will not have a valid start time, so just start the animation if needed
    if (isnew() && m_animation->animationPlayState() == AnimPlayStatePlaying && m_object->document()->loadComplete())
        updateStateMachine(STATE_INPUT_START_ANIMATION, -1);
    
    // If we get this far and the animation is done, it means we are cleaning up a just finished animation.
    // If so, we need to send back the targetStyle
    if (postactive()) {
        if (!ioAnimatedStyle)
            ioAnimatedStyle = const_cast<RenderStyle*>(targetStyle);
        return;
    }

    // if we are waiting for the start timer, we don't want to change the style yet
    // Special case - if the delay time is 0, then we do want to set the first frame of the
    // animation right away. This avoids a flash when the animation starts
    if (waitingToStart() && m_animation->animationDelay() > 0)
        return;
    
    // FIXME: we need to be more efficient about determining which keyframes we are animating between.
    // We should cache the last pair or something
    
    // find the first key
    double elapsedTime = (m_startTime > 0) ? ((running() ? currentTime() : m_pauseTime) - m_startTime) : 0;
    if (elapsedTime < 0)
        elapsedTime = 0;
        
    double t = m_animation->transitionDuration() ? (elapsedTime / m_animation->transitionDuration()) : 1;
    int i = (int) t;
    t -= i;
    if (m_animation->animationDirection() && (i & 1))
        t = 1 - t;

    RenderStyle* fromStyle = 0;
    RenderStyle* toStyle = 0;
    double scale = 1;
    double offset = 0;
    if (m_keyframes.get()) {
        Vector<KeyframeValue>::const_iterator end = m_keyframes->endKeyframes();
        for (Vector<KeyframeValue>::const_iterator it = m_keyframes->beginKeyframes(); it != end; ++it) {
            if (t < it->key) {
                // The first key should always be 0, so we should never succeed on the first key
                if (!fromStyle)
                    break;
                scale = 1 / (it->key - offset);
                toStyle = const_cast<RenderStyle*>(&(it->style));
                break;
            }
            
            offset = it->key;
            fromStyle = const_cast<RenderStyle*>(&(it->style));
        }
    }
    
    // if either style is 0 we have an invalid case, just stop the animation
    if (!fromStyle || !toStyle) {
        updateStateMachine(STATE_INPUT_END_ANIMATION, -1);
        return;
    }
    
    // run a cycle of animation.
    // We know we will need a new render style, so make one if needed
    if (!ioAnimatedStyle)
        ioAnimatedStyle = new (renderer->renderArena()) RenderStyle(*targetStyle);
    
    double prog = progress(scale, offset);

    HashSet<int>::const_iterator end = m_keyframes->endProperties();
    for (HashSet<int>::const_iterator it = m_keyframes->beginProperties(); it != end; ++it) {
        bool needsSWAnim = AnimationControllerPrivate::blendProperties(*it, ioAnimatedStyle, fromStyle, toStyle, prog);
        if (needsSWAnim)
            setAnimating();
        //ASSERT(needsSWAnim != m_waitedForResponse || preactive());
    }
}

bool KeyframeAnimation::startAnimation(double beginTime)
{
    return m_object ? m_object->startAnimation(beginTime, m_index) : false;
}

void KeyframeAnimation::endAnimation(bool reset)
{
    if (m_object) {
        // FIXME: need to determine actual index (in cases where there is more than one animation per Transition object)
        m_object->animationFinished(m_name, 0, reset);
        
        // restore the original (unanimated) style
        setChanged(m_object->element());
    }
}

void KeyframeAnimation::onAnimationStart(double inElapsedTime)
{
    sendAnimationEvent(EventNames::webkitAnimationStartEvent, inElapsedTime);
}

void KeyframeAnimation::onAnimationIteration(double inElapsedTime)
{
    sendAnimationEvent(EventNames::webkitAnimationIterationEvent, inElapsedTime);
}

void KeyframeAnimation::onAnimationEnd(double inElapsedTime)
{
    // FIXME: set the unanimated style on the element
    if (!sendAnimationEvent(EventNames::webkitAnimationEndEvent, inElapsedTime)) {
        // we didn't dispatch an event, which would call endAnimation(), so we'll just end
        // it here.
        endAnimation(true);
    }
}

bool KeyframeAnimation::sendAnimationEvent(const AtomicString& inEventType, double inElapsedTime)
{
    Document::ListenerType listenerType;
    if (inEventType == EventNames::webkitAnimationIterationEvent)
        listenerType = Document::ANIMATIONITERATION_LISTENER;
    else if (inEventType == EventNames::webkitAnimationEndEvent)
        listenerType = Document::ANIMATIONEND_LISTENER;
    else
        listenerType = Document::ANIMATIONSTART_LISTENER;
    
    if (shouldSendEventForListener(listenerType)) {
        HTMLElement* element = elementForEventDispatch();
        if (element) {
            m_waitingForEndEvent = true;
            m_animationEventDispatcher.startTimer(element, m_name, -1, true, inEventType, inElapsedTime);
            return true; // did dispatch an event
        }
    }
    
    return false; // didn't dispatch an event
}

void KeyframeAnimation::overrideAnimations()
{
    // this will override implicit animations that match the properties in the keyframe animation
    HashSet<int>::const_iterator end = m_keyframes->endProperties();
    for (HashSet<int>::const_iterator it = m_keyframes->beginProperties(); it != end; ++it)
        compositeAnimation()->overrideImplicitAnimations(*it);
}

void KeyframeAnimation::resumeOverriddenAnimations()
{
    // this will resume overridden implicit animations
    HashSet<int>::const_iterator end = m_keyframes->endProperties();
    for (HashSet<int>::const_iterator it = m_keyframes->beginProperties(); it != end; ++it)
        compositeAnimation()->resumeOverriddenImplicitAnimations(*it);
}

bool KeyframeAnimation::affectsProperty(int property) const
{
    HashSet<int>::const_iterator end = m_keyframes->endProperties();
    for (HashSet<int>::const_iterator it = m_keyframes->beginProperties(); it != end; ++it) {
        if ((*it) == property)
            return true;
    }
    return false;
}

#pragma mark -

AnimationController::AnimationController(Frame* frame)
: m_data(new AnimationControllerPrivate(frame))
{

}

AnimationController::~AnimationController()
{
    delete m_data;
}

void AnimationController::cancelAnimations(RenderObject* renderer)
{
    if (!m_data->hasAnimations())
        return;

    if (m_data->clear(renderer))
        setChanged(renderer->element());
}

RenderStyle* AnimationController::updateAnimations(RenderObject* renderer, RenderStyle* newStyle)
{
    // don't do anything if we're in the cache
    if (!renderer->document() || renderer->document()->inPageCache())
        return newStyle;
    
    RenderStyle* oldStyle = renderer->style();
        
    if ((!oldStyle || (!oldStyle->animations() && !oldStyle->transitions())) && 
                        (!newStyle->animations() && !newStyle->transitions()))
        return newStyle;
    
    RenderStyle* blendedStyle = newStyle;
    
    // Fetch our current set of implicit animations from a hashtable.  We then compare them
    // against the animations in the style and make sure we're in sync.  If destination values
    // have changed, we reset the animation.  We then do a blend to get new values and we return
    // a new style.
    ASSERT(renderer->element()); // FIXME: We do not animate generated content yet.
    
    CompositeAnimation* rendererAnimations = m_data->accessCompositeAnimation(renderer);
    blendedStyle = rendererAnimations->animate(renderer, oldStyle, newStyle);
    m_data->updateSWAnimationTimer();
    
    if (blendedStyle != newStyle) {
        // If the animations/transitions change opacity, transform or perspective, we neeed to update
        // the style to impose the stacking rules. Note that this is also done in CSSStyleSelector::adjustRenderStyle().
        if (blendedStyle->hasAutoZIndex() && ((blendedStyle->opacity() < 1.0f || blendedStyle->hasTransformOrPerspective())))
            blendedStyle->setZIndex(0);
    }
    return blendedStyle;
}

void AnimationController::setAnimationStartTime(RenderObject* obj, double t)
{
    CompositeAnimation* rendererAnimations = m_data->accessCompositeAnimation(obj);
    rendererAnimations->setAnimationStartTime(t);
}

void AnimationController::setTransitionStartTime(RenderObject* obj, int property, double t)
{
    CompositeAnimation* rendererAnimations = m_data->accessCompositeAnimation(obj);
    rendererAnimations->setTransitionStartTime(property, t);
}

bool AnimationController::isAnimatingPropertyOnRenderer(RenderObject* obj, int property) const
{
    return m_data->isAnimatingPropertyOnRenderer(obj, property);
}

void AnimationController::suspendAnimations(Document* document)
{
#ifdef DEBUG_STATE_MACHINE
    fprintf(stderr, "AnimationController %p suspendAnimations for document %p\n", this, document);
#endif
    m_data->suspendAnimations(document);
}

void AnimationController::resumeAnimations(Document* document)
{
#ifdef DEBUG_STATE_MACHINE
    fprintf(stderr, "AnimationController %p resumeAnimations for document %p\n", this, document);
#endif
    m_data->resumeAnimations(document);
}

void AnimationController::startUpdateRenderingDispatcher()
{
    m_data->startUpdateRenderingDispatcher();
}

void AnimationController::styleIsSetup()
{
    m_data->styleIsSetup();
}

void CompositeAnimation::setWaitingForStyleIsSetup(bool waiting)
{
    if (waiting)
        m_numStyleIsSetupWaiters++;
    else
        m_numStyleIsSetupWaiters--;
    m_animationController->setWaitingForStyleIsSetup(waiting);
}

}

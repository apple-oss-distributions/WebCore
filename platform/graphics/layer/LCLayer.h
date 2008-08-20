/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef LCLayer_h
#define LCLayer_h

#if ENABLE(HW_COMP)

#include "CSSPropertyNames.h"
#include "CSSStyleSelector.h"
#include "Color.h"
#include "FloatPoint.h"
#include "FloatRect.h"
#include "FloatQuad.h"
#include "IntRect.h"
#include "PlatformString.h"
#include "RenderLayer.h"
#include "RenderStyle.h"
#include "Transform3D.h"

#ifdef __OBJC__
#import "WKLayer.h"
@class NSView;
#else
typedef void *WKLayer;
class NSView;
#endif

namespace WebCore {

class TimingFunction;
class Image;

typedef WKLayer PlatformLayer;
typedef void* NativeLayer;      // might not be a WKLayer

class LCLayer {

public:
    class FloatValue {
    public:
        FloatValue(float inKey = nanf(0), float inValue = 0, const TimingFunction* inTimingFunction = 0)
        : mKey(inKey), mVal(inValue), mTF(0)
        {
            if (inTimingFunction)
                mTF = new TimingFunction(*inTimingFunction);
        }

        FloatValue(const FloatValue& inOther)
        : mKey(inOther.key()), mVal(inOther.value()), mTF(0)
        {
            if (inOther.timingFunction())
                mTF = new TimingFunction(*inOther.timingFunction());
        }

        ~FloatValue()
        {
            delete mTF;
        }
        
        const FloatValue& operator=(const FloatValue& inOther)
        {
            if (&inOther != this)
                set(inOther.key(), inOther.value(), inOther.timingFunction());
            return *this;
        }

        void set(float inKey, float inValue, const TimingFunction* inTimingFunction);
        
        float key() const { return mKey; }
        float value() const { return mVal; }
        const TimingFunction* timingFunction() const { return mTF; }

    private:
        float mKey;
        float mVal;
        const TimingFunction* mTF;       // owned
    };
    
    class FloatVector : public Vector<FloatValue> {
    public:
        void insert(float key, float val, const TimingFunction* tf);
    };

    class TransformValue {
    public:
        TransformValue(float inKey = nanf(0), const Transform3D& inValue = Transform3D(), const TimingFunction* inTimingFunction = 0)
        : mKey(inKey), mVal(inValue), mTF(0)
        {
            if (inTimingFunction)
                mTF = new TimingFunction(*inTimingFunction);
        }

        TransformValue(const TransformValue& inOther)
        : mKey(inOther.key()), mVal(inOther.value()), mTF(0)
        {
            if (inOther.timingFunction())
                mTF = new TimingFunction(*inOther.timingFunction());
        }

        ~TransformValue()
        {
            delete mTF;
        }

        const TransformValue& operator=(const TransformValue& inOther)
        {
            if (&inOther != this)
                set(inOther.key(), inOther.value(), inOther.timingFunction());
            return *this;
        }

        void set(float inKey, const Transform3D& inValue, const TimingFunction* inTimingFunction);

        float key() const { return mKey; }
        const Transform3D& value() const { return mVal; }
        const TimingFunction* timingFunction() const { return mTF; }

    private:
        float mKey;
        Transform3D mVal;
        const TimingFunction* mTF;       // owned
    };
    
    class TransformVector : public Vector<TransformValue> {
    public:
        void insert(float key, const Transform3D& val, const TimingFunction* tf);
    };

    virtual ~LCLayer();

    /* Static constructors
     * Use these so we can keep track of all layers
     * and possibly share contents
     */
    static LCLayer* layer();
    
    // The actual layer object used on the platform 
    PlatformLayer* platformLayer() const { return m_layer; }


    // platform behaviors
    static bool graphicsContextsFlipped();

#pragma mark -
#pragma mark Layer Name

    // Layer name
    String name() const { return m_name; }
    void setName(const String n);

#pragma mark -
#pragma mark Layer Geometry

    // The position of the layer (the location of its top-left corner in its parent)
    FloatPoint position() const { return m_position; }
    void setPosition(const FloatPoint& inPoint);
    
    // anchor point: (0, 0) is top left, (1, 1) is bottom right
    FloatPoint anchorPoint() const { return m_anchorPoint; }
    void setAnchorPoint(const FloatPoint& inPoint, float inZ);

    // The bounds of the layer
    FloatRect bounds() const { return m_bounds; }
    virtual void setBounds(const FloatRect& inRect);

    Transform3D transform() const { return m_transform; }
    // current transform taking animations/transitions into account
    Transform3D currentTransform() const;
    bool setTransform(const Transform3D* t, const Transition *anim, double time, short index);

    Transform3D childrenTransform() const { return m_childrenTransform; }
    void setChildrenTransform(const Transform3D& t);

    bool useTransformLayer() const { return m_transformLayer != NULL; }
    void setUseTransformLayer(bool useTransform);
    
    bool masksToBounds() const      { return m_masksToBounds; }
    void setMasksToBounds(bool inMasksToBounds);
    
    bool drawsContent() const     { return m_drawsContent; }
    void setDrawsContent(bool inDrawsContent);

#pragma mark -
#pragma mark Layer Style

    // The color used to paint the layer backgrounds
    Color backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(const Color& c, const Transition *anim, double time);
    void clearBackgroundColor();
    
    // @@ not sure if to do this or just use a clear color
    bool backgroundColorSet() const { return m_backgroundColorSet; }

    bool opaque() const { return m_opaque; }
    void setOpaque(bool o);

    float opacity() const { return m_opacity; }
    // current opacity taking animations/transitions into account
    float currentOpacity() const;
    bool setOpacity(float o, const Transition *anim, double time);

    bool backfaceVisibility() const { return m_backfaceVisibility; }
    void setBackfaceVisibility(bool b);
    
#pragma mark Layer Tree Structure

    // for hosting this LCLayer in a native layer hierarchy
    void setParentLayer(NativeLayer inLayer);

    LCLayer* parent() const { return m_parent; };
    
    const Vector<LCLayer*>& children() const { return m_children; }

    void addChild(LCLayer *layer);
    void addChildAtIndex(LCLayer *layer, int index);
    void addChildAbove(LCLayer *layer, LCLayer *sibling);
    void addChildBelow(LCLayer *layer, LCLayer *sibling);
    void replaceChild(LCLayer *oldChild, LCLayer *newChild);

    void removeAllChildren();

    void removeFromParent();

#pragma mark -
#pragma mark Contents

    RenderLayer* contents() const { return m_contents; }
    virtual void setContents(const RenderLayer* c);

    RenderLayerCompositePhase compositePhase() const { return m_compositePhase; }
    virtual void setCompositePhase(RenderLayerCompositePhase inPhase);
    
#pragma mark -
#pragma mark Animation

    bool animateTransform(const TransformVector& values, short index, const Transition *anim, double beginTime);
    bool animateFloat(CSSPropertyID type, const FloatVector& values, short index, const Transition *transition, double beginTime);
    
    void removeFinishedAnimations(const String& name, int index, bool reset);
    void removeFinishedTransitions(int property);
    void removeAllAnimations();

    void suspendAnimations();
    void resumeAnimations();
    
#pragma mark -
#pragma mark Rendering

    void setNeedsDisplay(bool includeDescendants = false);
    // mark the given rect (in layer coords) as needing dispay. never goes deep
    void setNeedsDisplayInRect(const FloatRect& inRect);
        
    // A child platform layer used in replaced layers (video and image) in
    // order to get rendering direct to the layer (with the flip)
    bool hasInnerLayer() const { return m_innerLayer != NULL; }
    void setHasInnerLayer(bool hasInnerLayer);
    
    PlatformLayer* innerPlatformLayer() const { return m_innerLayer; }
    void setInnerPlatformLayer(PlatformLayer* inLayer);
    void updateInnerLayerPosition();

    void setImageContent(Image* inImage);
    // setVideoContent(Movie* inMovie) ?

    // callback from the layer to draw its contents
    void paintCompositingLayerContents(GraphicsContext& inGraphicsContext, const IntRect& inClip);

    // use when the use scale changes. note that contentsScale may not return
    // the same value passed to setContentsScale(), because of clamping
    // and hysteresis.
    float contentsScale() const { return m_contentsScale; }
    void setContentsScale(float newScale);
    
#pragma mark -
#pragma mark Hit Testing
    
    // return the layer containing the given point, which is in the the coordinate system
    // of this layer's parent layer. HitTestData is an opaque set of data that is passed
    // back to the RenderLayer for hitTestLayerContents
    LCLayer* hitTest(const FloatPoint& inPoint, struct LayerHitTestData* inData) const;
    
    // return true if this layer contains the given point (which is in layer coordinates).
    // calls down to the native layer.
    bool layerContainsPoint(const FloatPoint& inPoint) const;

    // up-call used by the native layer hit testing to see if the given point
    // should be opaque for hit testing
    bool contentsContainPoint(const IntPoint& inPoint) const;
    
    FloatPoint convertPointToLayer(const FloatPoint& inPoint, LCLayer* inTargetLayer) const;
    FloatPoint convertPointFromLayer(const FloatPoint& inPoint, LCLayer* inSourceLayer) const;

    void convertQuadToLayer(FloatQuad& ioQuad, const LCLayer* inTargetLayer) const;
    
#pragma mark -
#pragma mark Debug

    void dumpLayer(TextStream& ts, int indent = 0) const;
    void dumpAnimation(CSSPropertyID type, short index) const;

    static String propertyIdToString(CSSPropertyID type);
    
    static bool showDebugBorders();

    static bool showRepaintCounter();
    int repaintCount() const { return m_repaintCount; }
    int incrementRepaintCount() { return ++m_repaintCount; }

protected:
    
    LCLayer();

    virtual void dumpProperties(TextStream& ts, int indent) const;
    virtual String dumpName() const { return String("LCLayer"); }

    void setParent(LCLayer* inLayer);
    
    PlatformLayer* hostLayerForSublayers() const;
    PlatformLayer* layerForSuperlayer() const;
    
    PlatformLayer* primaryLayer() const  { return m_transformLayer ? m_transformLayer : m_layer; }
    
    // returns the layer on which animations of the passed type are performed
    PlatformLayer* animatedLayer(CSSPropertyID type) const
    { return (type == CSS_PROP_BACKGROUND_COLOR) ? m_layer : primaryLayer(); }

    bool shouldUseTiledLayer(const FloatRect& rect) const;
    void swapFromOrToTiledLayer(bool inUseTiledLayer);
    
    void setBasicAnimation(CSSPropertyID type, short index, void* fromVal, void* toVal, 
                                bool isTransition, const Transition *transition, double time);
    void setKeyframeAnimation(CSSPropertyID type, short index, void* keys, void* values, void* timingFunctions, 
                                bool isTransition, const Transition *transition, double time);

    void setAnimation(CSSPropertyID type);
    void setAllAnimations();
    bool addAnimationEntry(CSSPropertyID type, short index, double& beginTime, 
                            bool isTransition, const Transition* transition);
    void animDone(CSSPropertyID type, short index);
    int findAnimationEntry(CSSPropertyID type, short index) const;
    void removeAnimation(int inAnimIndex, bool reset);
    void removeAllAnimationsForProperty(CSSPropertyID type);

    float clampedContentsScaleForScale(float inScale) const;

public:
    static bool gInSynchronousCommit;
    
protected:
    static const int kMaxPixelDimension = 1024;
    static const int kTiledLayerTileSize = 480;
    static const int kMaxPixelsForLayer = kMaxPixelDimension*kMaxPixelDimension;
    
    String m_name;
    
    FloatPoint m_position;
    FloatPoint m_anchorPoint;
    FloatRect m_bounds;
    Transform3D m_transform;
    Transform3D m_childrenTransform;
        
    float m_anchorZ;

    Color m_backgroundColor;
    bool m_backgroundColorSet;
    bool m_opaque;
    float m_opacity;
    bool m_backfaceVisibility;

    RenderLayer* m_contents;
    RenderLayerCompositePhase m_compositePhase;
    
    Vector<LCLayer*> m_children;
    LCLayer* m_parent;

    PlatformLayer* m_layer;
    PlatformLayer* m_transformLayer;
    PlatformLayer* m_innerLayer;

    bool m_usingTiledLayer;
    bool m_masksToBounds;
    bool m_drawsContent;
    
    float m_contentsScale;
    
    class AnimationEntry {
        public:
        AnimationEntry()
        : beginTime(-1)
        , pauseTime(-2)
        , endTime(-1)
        , type(CSS_PROP_INVALID)
        , isCurrent(true)
        , isTransition(true) { }
                            
        double beginTime, pauseTime, endTime;
        RefPtr<Transition> animLayer;
        CSSPropertyID type  : 10;
        short index         : 16;       // currently unused; was used by additive animation. always 0.
        bool isCurrent      : 1;
        bool isTransition   : 1;
    };
    
    void*                               m_animationDidStartCB;
    
    Vector<AnimationEntry>              m_animations;      // running animations/transitions
    
    static struct LayerHitTestData*    s_hitTestData;      // temporarily assigned while hit testing
    
    int m_repaintCount;

}; // class LCLayer

} // namespace WebCore

#endif // ENABLE(HW_COMP)

#endif // LCLayer_h


/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */


// RenderLayer.h methods

#if ENABLE(HW_COMP)

public:
    bool isDocumentLayer() const { return m_object->node()->isDocumentNode(); }

    // this flag is always false on the desktop version of WebCore because the document
    // layer document layer renders its own content and is not just a placeholder. Other
    // platforms may be different
    bool isPlaceholderDocumentLayer() const;

    // update z-order and overflow lists, and do compositing layer updates.
    // recurses to child layers
    void updateLayers();
    void updateThisLayer();
    void updatePageZoomFactor(float newZoom);

    bool has3DContent() const;

    void computeCompositingLayerRequirements(struct CompositingState& ioCompState);
    void rebuildCompositingLayerTree(struct CompositingState& ioCompState);

    // Gets the nearest enclosing render layer that draws into its own LCLayer
    RenderLayer* enclosingCompositingAncestor() const;
    // Gets the nearest enclosing render layer that draws into its own LCLayer (including self)
    RenderLayer* enclosingCompositingLayer() const;

    // Gets the nearest enclosing render layer that draws into its own LCLayer (including self), and has overflow clip style
    RenderLayer* enclosingCompositingClippingLayer() const;

    bool needsContentsCompositingLayer() const;
    void paintIntoLayer(RenderLayer* rootLayer, GraphicsContext*, const IntRect& paintDirtyRect,
                    bool haveTransparency, PaintRestriction paintRestriction, RenderLayerCompositePhase inCompositePhase, RenderObject* paintingRoot);
    void paintCompositingLayerContents(GraphicsContext& inGraphicsContext, RenderLayerCompositePhase inCompositePhase, const IntRect& inClip);
    
    // inPoint is in the coordinate system of this RenderLayer
    bool compositingLayerContentsContainPoint(const IntPoint& inPoint, RenderLayerCompositePhase inCompositePhase, struct LayerHitTestData* inData);

    // indicate that the layer contents need to be repainted. only has an effect
    // if layer compositing is being used
    void setNeedsRepaint();
    void setNeedsRepaintInRect(const IntRect& r);       // r is in the coordinate space of the layer's render object

    // enable compositing mode. exiting compositing mode is not implemented.
    void enableCompositingMode(bool inEnable = true) const;
    // are we in compositing mode? (info stored on the FrameView)
    bool inCompositingMode() const;
    
    // has intrinsic need for compositing layer
    bool requiresCompositingLayer() const;
    // needs a layer for internal or external reasons
    bool shouldCreateCompositingLayer() const;
    
    bool hasCompositingLayer() const;
    LCLayer* compositingLayer() const { return m_layer; }

    bool requiresClippingLayer() const;
    bool hasClippingLayer() const   { return m_clippingLayer != 0; }
    LCLayer* clippingLayer() const  { return m_clippingLayer; }

    // this is only ever called by RenderView to set up the root layer. All other
    // LCLayers are created by the RenderLayer.
    void setCompositingLayer(LCLayer* inLayer);

    // returns true if this RenderLayer only has content that can be rendered directly
    // by the compositing layer, without drawing (e.g. solid background color).
    bool isSimpleContainerCompositingLayer();
    bool useInnerContentLayer(bool& outHasBoxDecorations) const;
    // notification from the renderer that its content changed; used by RenderImage
    void rendererContentChanged();
    
    bool rendererHasBackground() const;
    const Color& rendererBackgroundColor();

    IntSize contentOffsetInCompostingLayer();
    IntSize contentSizeInCompositingLayer() const;
    
    IntRect calculateCompositingLayerBounds(const RenderLayer* inRoot, IntRect& outLayerBoundingBox);
    
    FloatPoint compositingLayerToLayerCoordinates(const FloatPoint& inPoint);
    FloatPoint layerToCompositingLayerCoordinates(const FloatPoint& inPoint);

    // public for DRT
    void forceCompositingLayer(bool inForce = true);
    bool forcedCompositingLayer() const { return m_forceCompositingLayer; }
    bool hasCompositingDescendant() const { return m_hasCompositingDescendant; }

private:

    void setParent(RenderLayer* parent);

    void setAsCompositingChildOfLayer(RenderLayer* inCompAncestor);
    
    void setCompositingParent(RenderLayer* inCompLayer);        // hook the LCLayers together
    RenderLayer* compositingParent() const;                     // consults the LCLayer hierachy

    void createCompositingLayer();
    void destroyCompositingLayer();
    LCLayer* hostingCompositingLayer() const;

    void parentInRootLayer();
    void unparentChildsLayer(RenderLayer* oldChild);

    void updateCompositingLayer(bool hasOldStyle, bool inScheduleUpdate = true, bool inNeedsRepaint = true);
    void updateCompositingLayerPosition();
    
    // rect is in the coords of the layer
    void repaintCompositingLayerRect(const IntRect& inRect, bool inIncludeDescendents);

    void setCompositingContentNeedsDisplay();
    void setCompositingContentNeedsDisplayInRect(const IntRect& inRect);
    
    bool computeIsSimpleContainerCompositingLayer();

    // returns true if this RenderLayer needs to draw content
    bool hasNonCompositingContent();

    // returns true if a running transition or animation needs a compositing layer
    bool animationRequiresCompositingLayer() const;

    // hit test down to the compositing layer
    RenderLayer* findHitCompositingLayer(RenderLayer* rootLayer, const HitTestRequest& request, HitTestResult& result, const IntRect& hitTestRect, const IntPoint& hitTestPoint);

    // convert points, taking transforms into account
    FloatPoint convertPointToLayer(const FloatPoint& inPoint, RenderLayer* inTargetLayer);

    FloatPoint convertToPageUsingCompositingLayer(const FloatPoint& inPoint);
    FloatPoint convertFromPageUsingCompositingLayer(const FloatPoint& inPoint);

    void convertQuadToPageUsingCompositingLayer(FloatQuad& ioQuad);

#endif  // ENABLE(HW_COMP)

    


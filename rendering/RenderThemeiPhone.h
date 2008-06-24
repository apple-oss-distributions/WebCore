/*
 * Copyright (C) 2005, 2006, 2007, 2008, Apple Inc.  All rights reserved.
 */


#ifndef RENDER_THEME_PURPLE_H
#define RENDER_THEME_PURPLE_H

#include "GraphicsContext.h"
#include "RenderTheme.h"


namespace WebCore {
    
class RenderStyle;
    
class RenderThemePurple : public RenderTheme {
public:
    
    virtual int popupInternalPaddingRight(RenderStyle*) const;
                
    virtual Color systemColor(int cssValueId) const;

protected:

    virtual short baselinePosition(const RenderObject* o) const;

    virtual bool isControlStyled(const RenderStyle* style, const BorderData& border, const BackgroundLayer& background, const Color& backgroundColor) const;
    
    // Methods for each appearance value.
    virtual void adjustCheckboxStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintCheckboxDecorations(RenderObject*, const RenderObject::PaintInfo&, const IntRect&);

    virtual void adjustRadioStyle(CSSStyleSelector*, RenderStyle*, Element*) const;    
    virtual bool paintRadioDecorations(RenderObject*, const RenderObject::PaintInfo&, const IntRect&);
    
    virtual bool paintButton(RenderObject*, const RenderObject::PaintInfo&, const IntRect&);
    virtual void setButtonSize(RenderStyle*) const;
    
    virtual void adjustTextFieldStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintTextFieldDecorations(RenderObject*, const RenderObject::PaintInfo&, const IntRect&);

    virtual bool paintTextAreaDecorations(RenderObject*, const RenderObject::PaintInfo&, const IntRect&);

    virtual void adjustMenuListButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintMenuListButtonDecorations(RenderObject*, const RenderObject::PaintInfo&, const IntRect&);
    
private:
    bool paintButtonDecorationsForAnyAppearance(RenderObject* o, const RenderObject::PaintInfo& i, const IntRect& r);

    typedef enum
    {
        InsetGradient,
        ShineGradient,
        ShadeGradient,
        ConvexGradient,
        ConcaveGradient
    } GradientName;
    
    Color * shadowColor() const;
    GradientRef gradientWithName(GradientName aGradientName) const;
    FloatRect addRoundedBorderClip(const FloatRect& aRect, const BorderData&, GraphicsContext* aContext);
};
    
}

#endif // RENDER_THEME_PURPLE_H


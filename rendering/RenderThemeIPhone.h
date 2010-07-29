/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc.  All rights reserved.
 */

#ifndef RENDER_THEME_IPHONE_H
#define RENDER_THEME_IPHONE_H


#include "RenderTheme.h"

namespace WebCore {
    
class RenderStyle;
class GraphicsContext;
    
class RenderThemeIPhone : public RenderTheme {
public:
    static PassRefPtr<RenderTheme> create();

    virtual int popupInternalPaddingRight(RenderStyle*) const;
    
    static void adjustButtonBorderRadius(RenderStyle*, RenderButton*);

protected:

    virtual int baselinePosition(const RenderObject* o) const;

    virtual bool isControlStyled(const RenderStyle* style, const BorderData& border, const FillLayer& background, const Color& backgroundColor) const;
    
    // Methods for each appearance value.
    virtual void adjustCheckboxStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintCheckboxDecorations(RenderObject*, const RenderObject::PaintInfo&, const IntRect&);

    virtual void adjustRadioStyle(CSSStyleSelector*, RenderStyle*, Element*) const;    
    virtual bool paintRadioDecorations(RenderObject*, const RenderObject::PaintInfo&, const IntRect&);
    
    virtual void adjustButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintButtonDecorations(RenderObject* o, const RenderObject::PaintInfo& i, const IntRect& r);
    virtual bool paintPushButtonDecorations(RenderObject* o, const RenderObject::PaintInfo& i, const IntRect& r);
    virtual void setButtonSize(RenderStyle*) const;
    
    virtual bool paintTextFieldDecorations(RenderObject*, const RenderObject::PaintInfo&, const IntRect&);

    virtual bool paintTextAreaDecorations(RenderObject*, const RenderObject::PaintInfo&, const IntRect&);

    virtual void adjustMenuListButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintMenuListButtonDecorations(RenderObject*, const RenderObject::PaintInfo&, const IntRect&);
    
    virtual Color platformActiveSelectionBackgroundColor() const;
    virtual Color platformInactiveSelectionBackgroundColor() const;
        
private:
    RenderThemeIPhone() { }
    virtual ~RenderThemeIPhone() { }

    typedef enum
    {
        InsetGradient,
        ShineGradient,
        ShadeGradient,
        ConvexGradient,
        ConcaveGradient
    } IPhoneGradientName;

    const Color& shadowColor() const;
    IPhoneGradientRef gradientWithName(IPhoneGradientName aGradientName) const;
    FloatRect addRoundedBorderClip(const FloatRect& aRect, const BorderData&, GraphicsContext* aContext);
};
    
}


#endif // RENDER_THEME_IPHONE_H


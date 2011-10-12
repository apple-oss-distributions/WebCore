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
    
    static void adjustRoundBorderRadius(RenderStyle*, RenderBox*);

protected:

    virtual int baselinePosition(const RenderObject* o) const;

    virtual bool isControlStyled(const RenderStyle* style, const BorderData& border, const FillLayer& background, const Color& backgroundColor) const;
    
    // Methods for each appearance value.
    virtual void adjustCheckboxStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintCheckboxDecorations(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustRadioStyle(CSSStyleSelector*, RenderStyle*, Element*) const;    
    virtual bool paintRadioDecorations(RenderObject*, const PaintInfo&, const IntRect&);
    
    virtual void adjustButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintButtonDecorations(RenderObject* o, const PaintInfo& i, const IntRect& r);
    virtual bool paintPushButtonDecorations(RenderObject* o, const PaintInfo& i, const IntRect& r);
    virtual void setButtonSize(RenderStyle*) const;
    
    virtual bool paintTextFieldDecorations(RenderObject*, const PaintInfo&, const IntRect&);

    virtual bool paintTextAreaDecorations(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustMenuListButtonStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintMenuListButtonDecorations(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSliderTrackStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSliderTrack(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSliderThumbSize(RenderObject*) const;
    virtual bool paintSliderThumbDecorations(RenderObject*, const PaintInfo&, const IntRect&);

    virtual void adjustSearchFieldStyle(CSSStyleSelector*, RenderStyle*, Element*) const;
    virtual bool paintSearchFieldDecorations(RenderObject*, const PaintInfo&, const IntRect&);
    
    virtual Color platformActiveSelectionBackgroundColor() const;
    virtual Color platformInactiveSelectionBackgroundColor() const;

    virtual bool shouldShowPlaceholderWhenFocused() const;
    virtual bool shouldHaveSpinButton(HTMLInputElement*) const;

private:
    RenderThemeIPhone() { }
    virtual ~RenderThemeIPhone() { }

    const Color& shadowColor() const;
    FloatRect addRoundedBorderClip(RenderObject* box, GraphicsContext*, const IntRect&);
};
    
}


#endif // RENDER_THEME_IPHONE_H


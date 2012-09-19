/*
 * Copyright (C) 2005, 2006, 2007, 2008 Apple Inc.  All rights reserved.
 */

#ifndef RenderThemeIOS_h
#define RenderThemeIOS_h


#include "RenderTheme.h"

namespace WebCore {
    
class RenderStyle;
class GraphicsContext;
    
class RenderThemeIOS : public RenderTheme {
public:
    static PassRefPtr<RenderTheme> create();

    virtual int popupInternalPaddingRight(RenderStyle*) const OVERRIDE;
    
    static void adjustRoundBorderRadius(RenderStyle*, RenderBox*);

protected:

    virtual int baselinePosition(const RenderObject* o) const OVERRIDE;

    virtual bool isControlStyled(const RenderStyle* style, const BorderData& border, const FillLayer& background, const Color& backgroundColor) const OVERRIDE;
    
    // Methods for each appearance value.
    virtual void adjustCheckboxStyle(StyleResolver*, RenderStyle*, Element*) const OVERRIDE;
    virtual bool paintCheckboxDecorations(RenderObject*, const PaintInfo&, const IntRect&) OVERRIDE;

    virtual void adjustRadioStyle(StyleResolver*, RenderStyle*, Element*) const OVERRIDE;
    virtual bool paintRadioDecorations(RenderObject*, const PaintInfo&, const IntRect&) OVERRIDE;
    
    virtual void adjustButtonStyle(StyleResolver*, RenderStyle*, Element*) const OVERRIDE;
    virtual bool paintButtonDecorations(RenderObject* o, const PaintInfo& i, const IntRect& r) OVERRIDE;
    virtual bool paintPushButtonDecorations(RenderObject* o, const PaintInfo& i, const IntRect& r) OVERRIDE;
    virtual void setButtonSize(RenderStyle*) const OVERRIDE;

    virtual bool paintFileUploadIconDecorations(RenderObject* inputRenderer, RenderObject* buttonRenderer, const PaintInfo&, const IntRect&, Icon*, FileUploadDecorations) OVERRIDE;

    virtual bool paintTextFieldDecorations(RenderObject*, const PaintInfo&, const IntRect&) OVERRIDE;
    virtual bool paintTextAreaDecorations(RenderObject*, const PaintInfo&, const IntRect&) OVERRIDE;

    virtual void adjustMenuListButtonStyle(StyleResolver*, RenderStyle*, Element*) const OVERRIDE;
    virtual bool paintMenuListButtonDecorations(RenderObject*, const PaintInfo&, const IntRect&) OVERRIDE;

    virtual void adjustSliderTrackStyle(StyleResolver*, RenderStyle*, Element*) const OVERRIDE;
    virtual bool paintSliderTrack(RenderObject*, const PaintInfo&, const IntRect&) OVERRIDE;

    virtual void adjustSliderThumbSize(RenderStyle*) const OVERRIDE;
    virtual bool paintSliderThumbDecorations(RenderObject*, const PaintInfo&, const IntRect&) OVERRIDE;

    virtual void adjustSearchFieldStyle(StyleResolver*, RenderStyle*, Element*) const OVERRIDE;
    virtual bool paintSearchFieldDecorations(RenderObject*, const PaintInfo&, const IntRect&) OVERRIDE;
    
    virtual Color platformActiveSelectionBackgroundColor() const OVERRIDE;
    virtual Color platformInactiveSelectionBackgroundColor() const OVERRIDE;

    virtual Color platformTapHighlightColor() const OVERRIDE { return 0x4D1A1A1A; }

    virtual bool shouldShowPlaceholderWhenFocused() const OVERRIDE;
    virtual bool shouldHaveSpinButton(HTMLInputElement*) const OVERRIDE;

private:
    RenderThemeIOS() { }
    virtual ~RenderThemeIOS() { }

    const Color& shadowColor() const;
    FloatRect addRoundedBorderClip(RenderObject* box, GraphicsContext*, const IntRect&);
};
    
}


#endif // RenderThemeIOS_h


#ifndef BOUNDINGBOXRENDERDATA_H
#define BOUNDINGBOXRENDERDATA_H
#include "skiaincludes.h"

#include <QWeakPointer>
#include "updatable.h"
#include "Animators/animator.h"
#include <QMatrix>
class PixmapEffectRenderData;
class BoundingBox;
#include "sharedpointerdefs.h"

class RenderDataCustomizerFunctor;
struct BoundingBoxRenderData : public _ScheduledExecutor {
    BoundingBoxRenderData(BoundingBox *parentBoxT);

    virtual ~BoundingBoxRenderData();

    virtual void copyFrom(const BoundingBoxRenderDataSPtr &src);
    bool copied = false;

    bool relBoundingRectSet = false;

    Animator::UpdateReason reason;

    BoundingBoxRenderDataSPtr makeCopy();

    bool redo = false;

    bool renderedToImage = false;
    QMatrix transform;
    QMatrix parentTransform;
    QMatrix relTransform;
    QRectF relBoundingRect;
    QRectF globalBoundingRect;
    qreal opacity = 1.;
    qreal resolution;
    qreal effectsMargin;
    int relFrame;

    // for motion blur
    bool useCustomRelFrame = false;
    qreal customRelFrame;
    QList<QRectF> otherGlobalRects;
    BoundingBoxRenderDataSPtr motionBlurTarget;
    // for motion blur

    QList<PixmapEffectRenderDataSPtr> pixmapEffects;
    SkPoint drawPos = SkPoint::Make(0.f, 0.f);
    SkBlendMode blendMode = SkBlendMode::kSrcOver;
    QRectF maxBoundsRect;
    bool maxBoundsEnabled = true;

    bool parentIsTarget = true;
    QWeakPointer<BoundingBox> parentBox;

    virtual void updateRelBoundingRect();
    void drawRenderedImageForParent(SkCanvas *canvas);
    virtual void renderToImage();
    sk_sp<SkImage> renderedImage;

    void _processUpdate();

    void beforeUpdate();

    void afterUpdate();

    void schedulerProccessed();

    virtual bool allDataReady() { return true; }

    void dataSet();

    void clearPixmapEffects() {
        pixmapEffects.clear();
        effectsMargin = 0.;
    }

    virtual QPointF getCenterPosition() {
        return relBoundingRect.center();
    }

    void appendRenderCustomizerFunctor(RenderDataCustomizerFunctor *customizer) {
        mRenderDataCustomizerFunctors.append(customizer);
    }

    void prependRenderCustomizerFunctor(RenderDataCustomizerFunctor *customizer) {
        mRenderDataCustomizerFunctors.prepend(customizer);
    }

    void parentBeingProcessed();
protected:
    void addSchedulerNow();
    QList<RenderDataCustomizerFunctor*> mRenderDataCustomizerFunctors;
    bool mDelayDataSet = false;
    bool mDataSet = false;
    virtual void drawSk(SkCanvas *canvas) = 0;
};

class RenderDataCustomizerFunctor {
public:
    RenderDataCustomizerFunctor();
    virtual ~RenderDataCustomizerFunctor();
    virtual void customize(const BoundingBoxRenderDataSPtr& data) = 0;
    void operator()(const BoundingBoxRenderDataSPtr& data);
};

class ReplaceTransformDisplacementCustomizer : public RenderDataCustomizerFunctor {
public:
    ReplaceTransformDisplacementCustomizer(const qreal &dx,
                                           const qreal &dy);

    void customize(const BoundingBoxRenderDataSPtr& data);
protected:
    qreal mDx, mDy;
};

class MultiplyTransformCustomizer : public RenderDataCustomizerFunctor {
public:
    MultiplyTransformCustomizer(const QMatrix &transform,
                                const qreal &opacity = 1.);

    void customize(const BoundingBoxRenderDataSPtr& data);
protected:
    QMatrix mTransform;
    qreal mOpacity = 1.;
};

class MultiplyOpacityCustomizer : public RenderDataCustomizerFunctor {
public:
    MultiplyOpacityCustomizer(const qreal &opacity);

    void customize(const BoundingBoxRenderDataSPtr& data);
protected:
    qreal mOpacity;
};

#endif // BOUNDINGBOXRENDERDATA_H
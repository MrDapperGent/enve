#ifndef BOXTARGETPROPERTY_H
#define BOXTARGETPROPERTY_H
#include <QtCore>
#include "property.h"
class BoundingBox;
class QrealAnimator;

class BoxTargetProperty;

#include "Boxes/boundingbox.h"
struct BoxTargetPropertyWaitingForBoxLoad : public FunctionWaitingForBoxLoad {
    BoxTargetPropertyWaitingForBoxLoad(const int &boxIdT,
                            BoxTargetProperty *targetPropertyT);

    void boxLoaded(BoundingBox *box);

    BoxTargetProperty *targetProperty;
};

class BoxTargetProperty : public Property {
    Q_OBJECT
public:
    BoxTargetProperty();

    BoundingBox *getTarget() const;
    void setTarget(BoundingBox *box);

    void makeDuplicate(Property *property);
    Property *makeDuplicate();

    bool SWT_isBoxTargetProperty() { return true; }
    void writeProperty(QIODevice *target);
    void readProperty(QIODevice *target);
signals:
    void targetSet(BoundingBox *);
private:
    QWeakPointer<BoundingBox> mTarget;
};

#endif // BOXTARGETPROPERTY_H

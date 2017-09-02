#ifndef CONNECTEDTOMAINWINDOW_H
#define CONNECTEDTOMAINWINDOW_H
#include <QString>

#define getAtIndexOrGiveNull(index, list) (( (index) >= (list).count() || (index) < 0 ) ? NULL : (list).at( (index) ))

#define Q_FOREACHInverted(item, list) item = getAtIndexOrGiveNull((list).count() - 1, (list)); \
    for(int i = (list).count() - 1; i >= 0; i--, item = getAtIndexOrGiveNull(i, (list)) )
#define Q_FOREACHInverted2(item, list) for(int i = (list).count() - 1; i >= 0; i--) { \
            item = getAtIndexOrGiveNull(i, (list));


class MainWindow;
class UndoRedo;
class UpdateScheduler;
class QKeyEvent;
class Gradient;

class ConnectedToMainWindow {
public:
    ConnectedToMainWindow();
    virtual ~ConnectedToMainWindow() {}

    void startNewUndoRedoSet();
    void finishUndoRedoSet();

    void createDetachedUndoRedoStack();
    void deleteDetachedUndoRedoStack();

    void addUndoRedo(UndoRedo *undoRedo);
    void callUpdateSchedulers();
    MainWindow *getMainWindow();
    virtual void schedulePivotUpdate();
    bool isShiftPressed();
    bool isCtrlPressed();
    bool isAltPressed();

    int getCurrentFrameFromMainWindow();
    int getFrameCount();
    bool isRecordingAllPoints();
    void graphUpdateAfterKeysChanged();
    void graphScheduleUpdateAfterKeysChanged();
    bool isShiftPressed(QKeyEvent *event);
    bool isCtrlPressed(QKeyEvent *event);
    bool isAltPressed(QKeyEvent *event);
protected:
    MainWindow *mMainWindow;
};

#endif // CONNECTEDTOMAINWINDOW_H

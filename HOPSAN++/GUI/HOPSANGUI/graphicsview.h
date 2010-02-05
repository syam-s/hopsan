#ifndef GRAPHICSVIEW_H
#define GRAPHICSVIEW_H

#include <QGraphicsView>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QByteArray>
#include <QString>
#include <QDataStream>
#include <QIODevice>
#include "componentguiclass.h"
#include <QGraphicsSvgItem>
#include <QPoint>
#include <QCursor>
#include <QWheelEvent>

class GraphicsView : public QGraphicsView
{
public:
    GraphicsView(QWidget *parent = 0);
    ~GraphicsView();

    //QByteArray *data;
    //QDataStream *stream;
    //QString *text;
    ComponentGuiClass *guiComponent;

protected:
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void wheelEvent(QWheelEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
};

#endif // GRAPHICSVIEW_H

#include "MagicEraserTool.h"
#include "QtAnnotation.h"
#include "AnnotationWorkstationExtensionPlugin.h"
#include "annotation/Annotation.h"
#include "../PathologyViewer.h"
#include "interfaces/ShortcutManager.h"
#include <QAction>
#include <QGraphicsItem>
#include <QApplication>
#include <QPainter>
#include <QWheelEvent>
#include <QtMath>

MagicEraserTool::MagicEraserTool(AnnotationWorkstationExtensionPlugin* annotationPlugin, PathologyViewer* viewer) :
  AnnotationTool(annotationPlugin, viewer),
  _brushRadius(50.0f),
  _minBrushRadius(10.0f),
  _maxBrushRadius(200.0f),
  _brushStep(5.0f),
  _isErasing(false),
  _brushIndicator(nullptr)
{
}

MagicEraserTool::~MagicEraserTool() {
  if (_brushIndicator) {
    delete _brushIndicator;
    _brushIndicator = nullptr;
  }
}

std::string MagicEraserTool::name() {
  return std::string("magiceraser");
}

void MagicEraserTool::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton && _viewer) {
    _isErasing = true;
    _deletedInSession.clear();
    QPointF scenePos = _viewer->mapToScene(event->pos());
    eraseAnnotationsInBrush(scenePos);
    event->accept();
  } else {
    event->ignore();
  }
}

void MagicEraserTool::mouseMoveEvent(QMouseEvent* event) {
  if (_viewer) {
    QPointF scenePos = _viewer->mapToScene(event->pos());

    if (_isErasing) {
      eraseAnnotationsInBrush(scenePos);
    }

    updateBrushIndicator();
  }
  AnnotationTool::mouseMoveEvent(event);
}

void MagicEraserTool::mouseReleaseEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    _isErasing = false;
    _deletedInSession.clear();
    event->accept();
  } else {
    event->ignore();
  }
}

void MagicEraserTool::wheelEvent(QWheelEvent* event) {
  if (_viewer && (event->modifiers() & Qt::ControlModifier || event->modifiers() & Qt::ShiftModifier)) {
    int numDegrees = event->angleDelta().y();
    float delta = numDegrees > 0 ? _brushStep : -_brushStep;
    _brushRadius = qBound(_minBrushRadius, _brushRadius + delta, _maxBrushRadius);
    updateBrushIndicator();
    event->accept();
  } else {
    event->ignore();
  }
}

void MagicEraserTool::setActive(bool active) {
  AnnotationTool::setActive(active);

  if (active) {
    if (!_brushIndicator) {
      _brushIndicator = new QGraphicsEllipseItem();
      _brushIndicator->setPen(QPen(QColor(255, 0, 0, 128), 2));
      _brushIndicator->setBrush(QBrush(QColor(255, 0, 0, 32)));
      _brushIndicator->setZValue(1000);
    }
    if (_viewer && _viewer->scene()) {
      _viewer->scene()->addItem(_brushIndicator);
      updateBrushIndicator();
    }
  } else {
    if (_brushIndicator) {
      if (_viewer && _viewer->scene()) {
        _viewer->scene()->removeItem(_brushIndicator);
      }
      delete _brushIndicator;
      _brushIndicator = nullptr;
    }
  }
}

void MagicEraserTool::updateBrushIndicator() {
  if (_brushIndicator && _viewer) {
    QPoint viewPos = _viewer->mapFromGlobal(QCursor::pos());
    QPointF scenePos = _viewer->mapToScene(viewPos);

    float diameter = _brushRadius * 2.0f;
    _brushIndicator->setRect(scenePos.x() - _brushRadius, scenePos.y() - _brushRadius, diameter, diameter);
  }
}

void MagicEraserTool::eraseAnnotationsInBrush(const QPointF& scenePos) {
  if (!_viewer || !_annotationPlugin) {
    return;
  }

  QList<QGraphicsItem*> items = _viewer->scene()->items(
    QRectF(scenePos.x() - _brushRadius, scenePos.y() - _brushRadius, _brushRadius * 2, _brushRadius * 2),
    Qt::IntersectsItemShape
  );

  for (QGraphicsItem* item : items) {
    QVariant data = item->data(0);
    quintptr ptr = data.toULongLong();

    if (ptr != 0) {
      QtAnnotation* annot = reinterpret_cast<QtAnnotation*>(ptr);

      if (annot && !_deletedInSession.contains(annot)) {
        QPointF localPos = annot->mapFromScene(scenePos);

        if (annot->contains(localPos)) {
          _deletedInSession.insert(annot);
          _annotationPlugin->deleteAnnotation(annot);
        }
      }
    }
  }
}

QPixmap MagicEraserTool::generateIcon() {
  int size = 32;
  QPixmap pixmap(size, size);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing);

  painter.setPen(QPen(QColor(200, 50, 50), 2));
  painter.setBrush(QBrush(QColor(220, 100, 100)));

  painter.drawRect(6, 6, 14, 14);
  painter.drawRect(12, 12, 14, 14);

  return pixmap;
}

QAction* MagicEraserTool::getToolButton() {
  if (!_button) {
    _button = new QAction("&Magic Eraser", this);
    _button->setObjectName(QString::fromStdString(name()));
    _button->setIcon(QIcon(generateIcon()));
    _button->setShortcut(ShortcutManager::getShortcut("tool_magiceraser", "E"));
  }
  return _button;
}

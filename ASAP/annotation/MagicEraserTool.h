#ifndef MAGICERASERTOOL_H
#define MAGICERASERTOOL_H

#include "AnnotationTool.h"
#include "core/Point.h"
#include "annotationplugin_export.h"
#include <QCursor>
#include <QGraphicsEllipseItem>
#include <QSet>

class AnnotationWorkstationExtensionPlugin;
class PathologyViewer;
class QtAnnotation;

class ANNOTATIONPLUGIN_EXPORT MagicEraserTool : public AnnotationTool {
  Q_OBJECT

public:
  MagicEraserTool(AnnotationWorkstationExtensionPlugin* annotationPlugin, PathologyViewer* viewer);
  ~MagicEraserTool();

  std::string name() override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void setActive(bool active) override;
  QAction* getToolButton() override;

private:
  void updateBrushIndicator();
  void eraseAnnotationsInBrush(const QPointF& scenePos);
  QPixmap generateIcon();

  float _brushRadius;
  float _minBrushRadius;
  float _maxBrushRadius;
  float _brushStep;
  bool _isErasing;
  QGraphicsEllipseItem* _brushIndicator;
  QSet<QtAnnotation*> _deletedInSession;
};

#endif

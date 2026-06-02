#ifndef CLASSTOGGLETOOL_H
#define CLASSTOGGLETOOL_H

#include "AnnotationTool.h"
#include "core/Point.h"
#include "annotationplugin_export.h"

class AnnotationWorkstationExtensionPlugin;
class PathologyViewer;
class PolyQtAnnotation;
class QString;

class ANNOTATIONPLUGIN_EXPORT ClassToggleTool : public AnnotationTool {
  Q_OBJECT

public:
  ClassToggleTool(AnnotationWorkstationExtensionPlugin* annotationPlugin, PathologyViewer* viewer);
  ~ClassToggleTool();
  std::string name() override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void setActive(bool active) override;
  QAction* getToolButton() override;

private:
  void writeLog(const QString& message);

  PolyQtAnnotation* _hoveredAnnotation;
  bool _logEnabled;
  QString _logFilePath;
};

#endif

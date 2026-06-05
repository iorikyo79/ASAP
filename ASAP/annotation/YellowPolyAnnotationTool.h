#ifndef YELLOWPOLYANNOTATIONTOOL_H
#define YELLOWPOLYANNOTATIONTOOL_H

#include "PolyAnnotationTool.h"
#include "annotationplugin_export.h"

class AnnotationWorkstationExtensionPlugin;
class PathologyViewer;

class ANNOTATIONPLUGIN_EXPORT YellowPolyAnnotationTool : public PolyAnnotationTool {
  Q_OBJECT

public:
  YellowPolyAnnotationTool(AnnotationWorkstationExtensionPlugin* annotationPlugin, PathologyViewer* viewer);
  std::string name() override;
  QAction* getToolButton() override;
  QColor getForcedColor() const override { return QColor("#F4FA58"); }
};

#endif

#include "ClassToggleTool.h"
#include "PolyQtAnnotation.h"
#include "AnnotationWorkstationExtensionPlugin.h"
#include "AnnotationUndoCommands.h"
#include "annotation/Annotation.h"
#include "../PathologyViewer.h"
#include "interfaces/ShortcutManager.h"
#include <QAction>
#include <QGraphicsItem>
#include <QApplication>
#include <QStyleHints>
#include <QSettings>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

ClassToggleTool::ClassToggleTool(AnnotationWorkstationExtensionPlugin* annotationPlugin, PathologyViewer* viewer) :
  AnnotationTool(annotationPlugin, viewer),
  _hoveredAnnotation(nullptr),
  _logEnabled(false),
  _logFilePath("")
{
  QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DIAG", "ASAP");
  _logEnabled = settings.value("ClassToggleDebugLog", false).toBool();
  if (_logEnabled) {
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/logs";
    QDir().mkpath(logDir);
    _logFilePath = logDir + "/classtoggle_debug.log";
    writeLog("=== ClassToggleTool initialized ===");
  }
}

ClassToggleTool::~ClassToggleTool() {
  if (_logEnabled) {
    writeLog("=== ClassToggleTool destroyed ===");
  }
}

void ClassToggleTool::writeLog(const QString& message) {
  if (!_logEnabled) return;

  QFile logFile(_logFilePath);
  if (logFile.open(QIODevice::Append | QIODevice::Text)) {
    QTextStream out(&logFile);
    out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
        << " - " << message << "\n";
    logFile.close();
  }
}

std::string ClassToggleTool::name() {
  return std::string("classtoggle");
}

void ClassToggleTool::mousePressEvent(QMouseEvent* event) {
  qDebug() << "[ClassToggleTool] mousePressEvent triggered";
  writeLog("mousePressEvent triggered");

  if (_viewer) {
    QPoint clickPos = event->pos();
    QPointF scenePos = _viewer->mapToScene(clickPos);

    qDebug() << "[ClassToggleTool] clickPos (widget):" << clickPos;
    writeLog("clickPos (widget): " + QString("(%1, %2)").arg(clickPos.x()).arg(clickPos.y()));
    qDebug() << "[ClassToggleTool] scenePos (scene):" << scenePos;
    writeLog("scenePos (scene): " + QString("(%1, %2)").arg(scenePos.x()).arg(scenePos.y()));

    QList<QtAnnotation*> annotations = _annotationPlugin->getQtAnnotations();
    qDebug() << "[ClassToggleTool] Total annotations:" << annotations.size();
    writeLog(QString("Total annotations: %1").arg(annotations.size()));

    for (int i = 0; i < annotations.size(); ++i) {
      QtAnnotation* annot = annotations[i];
      PolyQtAnnotation* polyAnnotation = dynamic_cast<PolyQtAnnotation*>(annot);

      if (polyAnnotation) {
        // 좌표계 통일: scene 좌표계로 변환
        std::shared_ptr<Annotation> annotation = polyAnnotation->getAnnotation();
        if (annotation) {
          std::vector<Point> coords = annotation->getCoordinates();
          QString coordStr = QString("Annotation %1 coords (scene): ").arg(i);
          qDebug() << "[ClassToggleTool] Annotation" << i << "has" << coords.size() << "coordinates";
          writeLog(coordStr);

          for (size_t j = 0; j < coords.size() && j < 10; ++j) {  // 최대 10개만 출력
            Point coord = coords[j];
            // 좌표는 getSceneScale()을 고려해야 scene 좌표계로 변환
            float sceneX = coord.getX() * _viewer->getSceneScale();
            float sceneY = coord.getY() * _viewer->getSceneScale();
            QString pt = QString("  [%1]: (%2, %3)").arg(j).arg(sceneX).arg(sceneY);
            qDebug() << "[ClassToggleTool]" << pt;
            writeLog(pt);
          }
          if (coords.size() > 10) {
            writeLog(QString("  ... and %1 more points").arg(coords.size() - 10));
          }

          bool contains = polyAnnotation->contains(scenePos);
          qDebug() << "[ClassToggleTool] Annotation" << i << "contains click:" << contains;
          writeLog(QString("Annotation %1 contains click: %2").arg(i).arg(contains ? "true" : "false"));
        }
      }
    }

    // 두 번째 루프: 실제 처리
    for (QtAnnotation* annot : annotations) {
      PolyQtAnnotation* polyAnnotation = dynamic_cast<PolyQtAnnotation*>(annot);
      if (polyAnnotation && polyAnnotation->contains(scenePos)) {
        qDebug() << "[ClassToggleTool] Found clicked polygon";
        writeLog("Found clicked polygon");
        std::string currentColor = polyAnnotation->getAnnotation()->getColor();
        QString colorStr = QString::fromStdString(currentColor);
        qDebug() << "[ClassToggleTool] Current color:" << colorStr;
        writeLog("Current color: " + colorStr);

        const std::string yellowColor = "#F4FA58";
        const std::string polyColor = "#0000FF";

        std::string newColor = (currentColor == yellowColor) ? polyColor : yellowColor;
        QString newColorStr = QString::fromStdString(newColor);
        qDebug() << "[ClassToggleTool] New color will be:" << newColorStr;
        writeLog("New color will be: " + newColorStr);

        if (currentColor != newColor) {
          qDebug() << "[ClassToggleTool] Pushing SetAnnotationColorCommand to undo stack";
          writeLog("Pushing SetAnnotationColorCommand to undo stack");
          _annotationPlugin->undoStack()->push(
            new SetAnnotationColorCommand(polyAnnotation, newColor, _annotationPlugin));
        } else {
          qDebug() << "[ClassToggleTool] Current color equals new color, skipping command";
          writeLog("Current color equals new color, skipping command");
        }
        event->accept();
        return;
      }
    }
    qDebug() << "[ClassToggleTool] No valid polygon clicked at this position";
    writeLog("No valid polygon clicked at this position");
    event->accept();
  } else {
    qDebug() << "[ClassToggleTool] ERROR: _viewer is null!";
    writeLog("ERROR: _viewer is null!");
  }
}

void ClassToggleTool::mouseMoveEvent(QMouseEvent* event) {
  if (_viewer) {
    if (_hoveredAnnotation) {
      _hoveredAnnotation->setHover(false);
      _hoveredAnnotation = nullptr;
      _viewer->scene()->update();
    }

    QPointF scenePos = _viewer->mapToScene(event->pos());

    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "DIAG", "ASAP");
    bool hoverMaskEnabled = settings.value("annotationHoverMask", true).toBool();
    qDebug() << "[ClassToggleTool] mouseMoveEvent - hoverMaskEnabled:" << hoverMaskEnabled;
    writeLog("mouseMoveEvent - hoverMaskEnabled: " + QString(hoverMaskEnabled ? "true" : "false"));

    if (hoverMaskEnabled) {
      QList<QtAnnotation*> annotations = _annotationPlugin->getQtAnnotations();
      for (QtAnnotation* annot : annotations) {
        PolyQtAnnotation* polyAnnotation = dynamic_cast<PolyQtAnnotation*>(annot);
        if (polyAnnotation && polyAnnotation->contains(scenePos)) {
          qDebug() << "[ClassToggleTool] Hovering over polygon";
          writeLog("Hovering over polygon");
          _hoveredAnnotation = polyAnnotation;
          polyAnnotation->setHover(true);
          _viewer->scene()->update();
          break;
        }
      }
    }
  }
  AnnotationTool::mouseMoveEvent(event);
}

void ClassToggleTool::setActive(bool active) {
  qDebug() << "[ClassToggleTool] setActive:" << active;
  writeLog("setActive: " + QString(active ? "true" : "false"));
  AnnotationTool::setActive(active);
  if (!active && _hoveredAnnotation) {
    qDebug() << "[ClassToggleTool] Clearing hover state on deactivation";
    writeLog("Clearing hover state on deactivation");
    _hoveredAnnotation->setHover(false);
    _hoveredAnnotation = nullptr;
    if (_viewer && _viewer->scene()) {
      _viewer->scene()->update();
    }
  }
}

QAction* ClassToggleTool::getToolButton() {
  if (!_button) {
    _button = new QAction("&Class Toggle", this);
    _button->setObjectName(QString::fromStdString(name()));
    if (QApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
      _button->setIcon(QIcon(QPixmap(":/AnnotationWorkstationExtensionPlugin_icons/poly_dark.png")));
    }
    else {
      _button->setIcon(QIcon(QPixmap(":/AnnotationWorkstationExtensionPlugin_icons/poly.png")));
    }
    _button->setShortcut(ShortcutManager::getShortcut("tool_classtoggle", "T"));
  }
  return _button;
}

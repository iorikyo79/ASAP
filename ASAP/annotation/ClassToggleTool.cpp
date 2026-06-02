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
    QPointF scenePos = _viewer->mapToScene(event->pos());
    QString posStr = QString("(%1, %2)").arg(scenePos.x()).arg(scenePos.y());
    qDebug() << "[ClassToggleTool] scenePos:" << scenePos;
    writeLog("scenePos: " + posStr);

    QList<QGraphicsItem*> items = _viewer->scene()->items(scenePos);
    qDebug() << "[ClassToggleTool] Found" << items.size() << "items at click position";
    writeLog(QString("Found %1 items at click position").arg(items.size()));

    for (QGraphicsItem* item : items) {
      if (item->data(0).toString() == "QtAnnotation") {
        QtAnnotation* clicked = static_cast<QtAnnotation*>(item);
        qDebug() << "[ClassToggleTool] Found QtAnnotation";
        writeLog("Found QtAnnotation");
        PolyQtAnnotation* polyAnnotation = dynamic_cast<PolyQtAnnotation*>(clicked);
        if (polyAnnotation) {
          qDebug() << "[ClassToggleTool] Found PolyQtAnnotation, checking if click is inside...";
          writeLog("Found PolyQtAnnotation, checking if click is inside...");
          if (polyAnnotation->contains(scenePos)) {
            qDebug() << "[ClassToggleTool] Click is INSIDE polygon";
            writeLog("Click is INSIDE polygon");
            std::string currentColor = clicked->getAnnotation()->getColor();
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
                new SetAnnotationColorCommand(clicked, newColor, _annotationPlugin));
            } else {
              qDebug() << "[ClassToggleTool] Current color equals new color, skipping command";
              writeLog("Current color equals new color, skipping command");
            }
            event->accept();
            return;
          } else {
            qDebug() << "[ClassToggleTool] Click is OUTSIDE polygon";
            writeLog("Click is OUTSIDE polygon");
          }
        } else {
          qDebug() << "[ClassToggleTool] Item is not a PolyQtAnnotation";
          writeLog("Item is not a PolyQtAnnotation");
        }
      } else {
        qDebug() << "[ClassToggleTool] Item is not a QtAnnotation";
        writeLog("Item is not a QtAnnotation");
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
      QList<QGraphicsItem*> items = _viewer->scene()->items(scenePos);
      for (QGraphicsItem* item : items) {
        if (item->data(0).toString() == "QtAnnotation") {
          QtAnnotation* qtAnnotation = static_cast<QtAnnotation*>(item);
          PolyQtAnnotation* polyAnnotation = dynamic_cast<PolyQtAnnotation*>(qtAnnotation);
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

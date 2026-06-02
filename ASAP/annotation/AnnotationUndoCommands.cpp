#include "AnnotationUndoCommands.h"
#include "QtAnnotation.h"
#include "QtAnnotationGroup.h"
#include "AnnotationWorkstationExtensionPlugin.h"
#include "annotation/AnnotationService.h"
#include "annotation/Annotation.h"
#include "annotation/AnnotationList.h"
#include "annotation/AnnotationGroup.h"
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QGraphicsScene>
#include <QPixmap>
#include <QIcon>
#include <algorithm>

// ============================================================================
// MoveCoordinateCommand
// ============================================================================

MoveCoordinateCommand::MoveCoordinateCommand(QtAnnotation* annotation, int index,
  float dx, float dy, QUndoCommand* parent)
  : QUndoCommand(parent), _annotation(annotation), _index(index), _dx(dx), _dy(dy) {
}

int MoveCoordinateCommand::id() const {
  return MOVE_COORDINATE_CMD_ID;
}

bool MoveCoordinateCommand::mergeWith(const QUndoCommand* other) {
  if (other->id() != id()) {
    return false;
  }
  const MoveCoordinateCommand* cmd = static_cast<const MoveCoordinateCommand*>(other);
  if (_annotation != cmd->_annotation || _index != cmd->_index) {
    return false;
  }
  _dx += cmd->_dx;
  _dy += cmd->_dy;
  return true;
}

void MoveCoordinateCommand::undo() {
  _annotation->moveCoordinateBy(_index, Point(-_dx, -_dy));
}

void MoveCoordinateCommand::redo() {
  _annotation->moveCoordinateBy(_index, Point(_dx, _dy));
}

// ============================================================================
// InsertCoordinateCommand
// ============================================================================

InsertCoordinateCommand::InsertCoordinateCommand(QtAnnotation* annotation, int index,
  float x, float y, QUndoCommand* parent)
  : QUndoCommand(parent), _annotation(annotation), _index(index), _x(x), _y(y) {
}

void InsertCoordinateCommand::undo() {
  _annotation->removeCoordinate(_index);
}

void InsertCoordinateCommand::redo() {
  _annotation->insertCoordinate(_index, _x, _y);
}

// ============================================================================
// RemoveCoordinateCommand
// ============================================================================

RemoveCoordinateCommand::RemoveCoordinateCommand(QtAnnotation* annotation, int index,
  QUndoCommand* parent)
  : QUndoCommand(parent), _annotation(annotation), _index(index), _x(0), _y(0) {
  // Save the coordinate before removal
  Point p = _annotation->getAnnotation()->getCoordinate(index);
  _x = p.getX();
  _y = p.getY();
}

void RemoveCoordinateCommand::undo() {
  _annotation->insertCoordinate(_index, _x, _y);
}

void RemoveCoordinateCommand::redo() {
  _annotation->removeCoordinate(_index);
}

// ============================================================================
// CreateAnnotationCommand
// ============================================================================

CreateAnnotationCommand::CreateAnnotationCommand(
  AnnotationWorkstationExtensionPlugin* plugin, QtAnnotation* annotation,
  QUndoCommand* parent)
  : QUndoCommand(parent), _plugin(plugin), _annotation(annotation) {
}

void CreateAnnotationCommand::redo() {
  // Add to annotation list
  _plugin->_qtAnnotations.append(_annotation);
  _plugin->_annotationService->getList()->addAnnotation(_annotation->getAnnotation());

  // Find parent tree item based on annotation group
  QTreeWidgetItem* parentItem = _plugin->_treeWidget->invisibleRootItem();
  std::shared_ptr<AnnotationGroup> group = _annotation->getAnnotation()->getGroup();
  if (group) {
    QTreeWidgetItemIterator it(_plugin->_treeWidget);
    while (*it) {
      if (QtAnnotationGroup* grp = (*it)->data(1, Qt::UserRole).value<QtAnnotationGroup*>()) {
        if (grp->getAnnotationGroup() == group) {
          parentItem = *it;
          break;
        }
      }
      ++it;
    }
  }

  // Create tree widget item
  QTreeWidgetItem* newAnnotation = new QTreeWidgetItem(parentItem);
  newAnnotation->setText(1, QString::fromStdString(_annotation->getAnnotation()->getName()));
  newAnnotation->setText(2, QString::fromStdString(_annotation->getAnnotation()->getTypeAsString()));
  newAnnotation->setFlags(newAnnotation->flags() & ~Qt::ItemIsDropEnabled);
  newAnnotation->setFlags(newAnnotation->flags() | Qt::ItemIsEditable);
  newAnnotation->setData(1, Qt::UserRole, QVariant::fromValue<QtAnnotation*>(_annotation));
  newAnnotation->setSelected(true);

  QColor color(QString::fromStdString(_annotation->getAnnotation()->getColor()));
  int cHeight = _plugin->_treeWidget->visualItemRect(newAnnotation).height();
  if (_plugin->_treeWidget->topLevelItemCount() > 0) {
    cHeight = _plugin->_treeWidget->visualItemRect(_plugin->_treeWidget->topLevelItem(0)).height();
  }
  QPixmap iconPM(cHeight, cHeight);
  iconPM.fill(color);
  QIcon icon(iconPM);
  newAnnotation->setIcon(0, icon);
  newAnnotation->setData(0, Qt::UserRole, color);

  _plugin->_treeWidget->resizeColumnToContents(0);
  _plugin->_treeWidget->resizeColumnToContents(1);
  _plugin->_activeAnnotation = _annotation;
  _plugin->_annotToItem[_annotation] = newAnnotation;

  QObject::connect(_annotation, SIGNAL(annotationChanged(QtAnnotation*)),
    _plugin, SLOT(updateAnnotationToolTip(QtAnnotation*)));
  _plugin->updateAnnotationToolTip(_annotation);

  _annotation->setVisible(_plugin->_annotationsVisible);
}

void CreateAnnotationCommand::undo() {
  // Disconnect signal
  QObject::disconnect(_annotation, SIGNAL(annotationChanged(QtAnnotation*)),
    _plugin, SLOT(updateAnnotationToolTip(QtAnnotation*)));

  // Remove tree widget item
  QMap<QtAnnotation*, QTreeWidgetItem*>::iterator it = _plugin->_annotToItem.find(_annotation);
  if (it != _plugin->_annotToItem.end()) {
    it.value()->setSelected(false);
    delete it.value();
    _plugin->_annotToItem.erase(it);
  }

  // Remove from annotation list
  if (_plugin->_annotationService) {
    std::vector<std::shared_ptr<Annotation> > annots = _plugin->_annotationService->getList()->getAnnotations();
    int annotInd = std::find(annots.begin(), annots.end(), _annotation->getAnnotation()) - annots.begin();
    if (annotInd < static_cast<int>(annots.size())) {
      _plugin->_annotationService->getList()->removeAnnotation(annotInd);
    }
  }

  // Remove from internal lists
  _plugin->_qtAnnotations.removeOne(_annotation);
  _plugin->_selectedAnnotations.remove(_annotation);
  if (_plugin->_activeAnnotation == _annotation) {
    _plugin->_activeAnnotation = NULL;
  }

  // Hide from scene
  _annotation->setVisible(false);
}

// ============================================================================
// DeleteAnnotationCommand
// ============================================================================

DeleteAnnotationCommand::DeleteAnnotationCommand(
  AnnotationWorkstationExtensionPlugin* plugin, QtAnnotation* annotation,
  QUndoCommand* parent)
  : QUndoCommand(parent), _plugin(plugin), _annotation(annotation), _removed(false) {
}

DeleteAnnotationCommand::~DeleteAnnotationCommand() {
  // If the command is destroyed while the annotation is in "deleted" state
  // (removed from model but not restored), clean up the QtAnnotation.
  if (_removed && _annotation) {
    QGraphicsScene* scene = _annotation->scene();
    if (scene) {
      scene->removeItem(_annotation);
    }
    delete _annotation;
  }
}

void DeleteAnnotationCommand::redo() {
  // Disconnect signal
  QObject::disconnect(_annotation, SIGNAL(annotationChanged(QtAnnotation*)),
    _plugin, SLOT(updateAnnotationToolTip(QtAnnotation*)));

  // Remove tree widget item
  QMap<QtAnnotation*, QTreeWidgetItem*>::iterator it = _plugin->_annotToItem.find(_annotation);
  if (it != _plugin->_annotToItem.end()) {
    it.value()->setSelected(false);
    delete it.value();
    _plugin->_annotToItem.erase(it);
  }

  // Remove from annotation list
  if (_plugin->_annotationService) {
    std::vector<std::shared_ptr<Annotation> > annots = _plugin->_annotationService->getList()->getAnnotations();
    int annotInd = std::find(annots.begin(), annots.end(), _annotation->getAnnotation()) - annots.begin();
    if (annotInd < static_cast<int>(annots.size())) {
      _plugin->_annotationService->getList()->removeAnnotation(annotInd);
    }
  }

  // Remove from internal lists
  _plugin->_qtAnnotations.removeOne(_annotation);
  _plugin->_selectedAnnotations.remove(_annotation);
  if (_plugin->_activeAnnotation == _annotation) {
    _plugin->_activeAnnotation = NULL;
  }

  // Hide from scene (keep alive for potential undo)
  _annotation->setVisible(false);
  _removed = true;
}

void DeleteAnnotationCommand::undo() {
  _removed = false;

  // Show in scene
  _annotation->setVisible(_plugin->_annotationsVisible);

  // Add back to annotation list
  _plugin->_annotationService->getList()->addAnnotation(_annotation->getAnnotation());

  // Find parent tree item based on annotation group
  QTreeWidgetItem* parentItem = _plugin->_treeWidget->invisibleRootItem();
  std::shared_ptr<AnnotationGroup> group = _annotation->getAnnotation()->getGroup();
  if (group) {
    QTreeWidgetItemIterator it(_plugin->_treeWidget);
    while (*it) {
      if (QtAnnotationGroup* grp = (*it)->data(1, Qt::UserRole).value<QtAnnotationGroup*>()) {
        if (grp->getAnnotationGroup() == group) {
          parentItem = *it;
          break;
        }
      }
      ++it;
    }
  }

  // Recreate tree widget item
  QTreeWidgetItem* newAnnotation = new QTreeWidgetItem(parentItem);
  newAnnotation->setText(1, QString::fromStdString(_annotation->getAnnotation()->getName()));
  newAnnotation->setText(2, QString::fromStdString(_annotation->getAnnotation()->getTypeAsString()));
  newAnnotation->setFlags(newAnnotation->flags() & ~Qt::ItemIsDropEnabled);
  newAnnotation->setFlags(newAnnotation->flags() | Qt::ItemIsEditable);
  newAnnotation->setData(1, Qt::UserRole, QVariant::fromValue<QtAnnotation*>(_annotation));

  QColor color(QString::fromStdString(_annotation->getAnnotation()->getColor()));
  int cHeight = _plugin->_treeWidget->visualItemRect(newAnnotation).height();
  if (_plugin->_treeWidget->topLevelItemCount() > 0) {
    cHeight = _plugin->_treeWidget->visualItemRect(_plugin->_treeWidget->topLevelItem(0)).height();
  }
  QPixmap iconPM(cHeight, cHeight);
  iconPM.fill(color);
  QIcon icon(iconPM);
  newAnnotation->setIcon(0, icon);
  newAnnotation->setData(0, Qt::UserRole, color);

  // Add back to internal lists
  _plugin->_qtAnnotations.append(_annotation);
  _plugin->_annotToItem[_annotation] = newAnnotation;
  _plugin->_activeAnnotation = _annotation;

  QObject::connect(_annotation, SIGNAL(annotationChanged(QtAnnotation*)),
    _plugin, SLOT(updateAnnotationToolTip(QtAnnotation*)));
  _plugin->updateAnnotationToolTip(_annotation);

  _plugin->_treeWidget->resizeColumnToContents(0);
  _plugin->_treeWidget->resizeColumnToContents(1);
}

// ============================================================================
// SetAnnotationColorCommand
// ============================================================================

SetAnnotationColorCommand::SetAnnotationColorCommand(
  QtAnnotation* annotation, const std::string& newColor,
  AnnotationWorkstationExtensionPlugin* plugin, QUndoCommand* parent)
  : QUndoCommand(parent), _annotation(annotation), _newColor(newColor), _plugin(plugin) {
  _oldColor = _annotation->getAnnotation()->getColor();
}

void SetAnnotationColorCommand::redo() {
  _annotation->getAnnotation()->setColor(_newColor);

  QMap<QtAnnotation*, QTreeWidgetItem*>::iterator it = _plugin->_annotToItem.find(_annotation);
  if (it != _plugin->_annotToItem.end()) {
    QTreeWidgetItem* item = it.value();

    QColor newColor(QString::fromStdString(_newColor));
    int cHeight = _plugin->_treeWidget->visualItemRect(item).height());
    if (_plugin->_treeWidget->topLevelItemCount() > 0) {
      cHeight = _plugin->_treeWidget->visualItemRect(_plugin->_treeWidget->topLevelItem(0)).height();
    }
    QPixmap iconPM(cHeight, cHeight);
    iconPM.fill(newColor);
    QIcon icon(iconPM);
    item->setIcon(0, icon);
    item->setData(0, Qt::UserRole, newColor);

    _plugin->_treeWidget->resizeColumnToContents(0);
    _plugin->_treeWidget->resizeColumnToContents(1);
  }

  _annotation->update();
  if (_annotation->scene()) {
    _annotation->scene()->update();
  }
}

void SetAnnotationColorCommand::undo() {
  _annotation->getAnnotation()->setColor(_oldColor);

  QMap<QtAnnotation*, QTreeWidgetItem*>::iterator it = _plugin->_annotToItem.find(_annotation);
  if (it != _plugin->_annotToItem.end()) {
    QTreeWidgetItem* item = it.value();

    QColor oldColor(QString::fromStdString(_oldColor));
    int cHeight = _plugin->_treeWidget->visualItemRect(item).height());
    if (_plugin->_treeWidget->topLevelItemCount() > 0) {
      cHeight = _plugin->_treeWidget->visualItemRect(_plugin->_treeWidget->topLevelItem(0)).height();
    }
    QPixmap iconPM(cHeight, cHeight);
    iconPM.fill(oldColor);
    QIcon icon(iconPM);
    item->setIcon(0, icon);
    item->setData(0, Qt::UserRole, oldColor);

    _plugin->_treeWidget->resizeColumnToContents(0);
    _plugin->_treeWidget->resizeColumnToContents(1);
  }

  _annotation->update();
  if (_annotation->scene()) {
    _annotation->scene()->update();
  }
}

#ifndef ANNOTATIONUNDOCOMMANDS_H
#define ANNOTATIONUNDOCOMMANDS_H

#include <QUndoCommand>
#include <QTreeWidgetItem>
#include <QColor>
#include <QString>
#include <QMap>
#include <QPixmap>
#include <QIcon>
#include <memory>
#include "core/Point.h"

class QtAnnotation;
class AnnotationWorkstationExtensionPlugin;
class Annotation;

// Command IDs for mergeWith discrimination
const int MOVE_COORDINATE_CMD_ID = 1001;

class MoveCoordinateCommand : public QUndoCommand {
public:
  MoveCoordinateCommand(QtAnnotation* annotation, int index,
    float dx, float dy, QUndoCommand* parent = nullptr);
  int id() const override;
  bool mergeWith(const QUndoCommand* other) override;
  void undo() override;
  void redo() override;
private:
  QtAnnotation* _annotation;
  int _index;
  float _dx;
  float _dy;
};

class InsertCoordinateCommand : public QUndoCommand {
public:
  InsertCoordinateCommand(QtAnnotation* annotation, int index,
    float x, float y, QUndoCommand* parent = nullptr);
  void undo() override;
  void redo() override;
private:
  QtAnnotation* _annotation;
  int _index;
  float _x;
  float _y;
};

class RemoveCoordinateCommand : public QUndoCommand {
public:
  RemoveCoordinateCommand(QtAnnotation* annotation, int index,
    QUndoCommand* parent = nullptr);
  void undo() override;
  void redo() override;
private:
  QtAnnotation* _annotation;
  int _index;
  float _x;
  float _y;
};

class CreateAnnotationCommand : public QUndoCommand {
public:
  CreateAnnotationCommand(AnnotationWorkstationExtensionPlugin* plugin,
    QtAnnotation* annotation, QUndoCommand* parent = nullptr);
  void undo() override;
  void redo() override;
private:
  AnnotationWorkstationExtensionPlugin* _plugin;
  QtAnnotation* _annotation;
};

class DeleteAnnotationCommand : public QUndoCommand {
public:
  DeleteAnnotationCommand(AnnotationWorkstationExtensionPlugin* plugin,
    QtAnnotation* annotation, QUndoCommand* parent = nullptr);
  ~DeleteAnnotationCommand();
  void undo() override;
  void redo() override;
private:
  AnnotationWorkstationExtensionPlugin* _plugin;
  QtAnnotation* _annotation;
  bool _removed;
};

class SetAnnotationColorCommand : public QUndoCommand {
public:
  SetAnnotationColorCommand(QtAnnotation* annotation, const std::string& newColor,
    AnnotationWorkstationExtensionPlugin* plugin, QUndoCommand* parent = nullptr);
  void undo() override;
  void redo() override;
private:
  QtAnnotation* _annotation;
  std::string _oldColor;
  std::string _newColor;
  AnnotationWorkstationExtensionPlugin* _plugin;
};

#endif // ANNOTATIONUNDOCOMMANDS_H

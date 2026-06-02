# ASAP Customizable Keyboard Shortcuts - Implementation Plan

## 1. Feature Summary

ASAP에 하드코딩된 18개 단축키를 모두 `QSettings`에서 읽어오도록 변경하고, `QKeySequenceEdit` 위젯을 사용하는 단축키 설정 다이얼로그를 추가하여 사용자가 단축키를 자유롭게 커스터마이징할 수 있도록 한다.

## 2. Architecture Decision

### 선택: 코어 유틸리티 라이브러리 (ASAPLib 내 신규 헤더-only 유틸리티)

**선택 이유:**

1. **의존성 방향 준수**: 단축키 설정을 읽는 기능은 `core` 수준의 유틸리티여야 한다. 모든 플러그인(AnnotationPlugin, ZoomToolPlugin, PanToolPlugin)과 메인 윈도우(ASAP_Window)가 접근해야 하므로, 최하위 라이브러리인 `ASAPLib`에 배치한다.
2. **플러그인 독립성 유지**: ZoomTool, PanTool은 `_annotationPlugin`에 접근할 수 없는 독립 플러그인이다. 중앙 집중식 `ShortcutManager`를 통해 QSettings만으로 단축키를 읽을 수 있어야 한다.
3. **기존 패턴 준수**: ASAP은 이미 `QSettings(QSettings::IniFormat, QSettings::UserScope, "DIAG", "ASAP")`를 전역적으로 사용 중이다. 새로운 설정도 동일한 QSettings 인스턴스에 저장한다.

**거부된 대안:**

- **새 플러그인으로 구현**: 단축키 설정은 메인 윈도우와 모든 플러그인이 공유해야 하므로 플러그인으로 격리하면 순환 의존성이 발생한다.
- **각 컴포넌트에서 독립적으로 관리**: 중복 코드가 많아지고 단축키 충돌 검출이 불가능해진다.

### 핵심 설계

```
ShortcutManager (신규, ASAPLib/interfaces/ 에 배치)
    - 단일 헤더 ShortcutManager.h / ShortcutManager.cpp
    - QSettings에서 단축키 읽기/쓰기
    - 기본값 제공 (현재 하드코딩된 값)
    - 단축키 충돌 검출
    - 싱글톤 접근 (QSettings 기반이므로 상태 비저장)

ShortcutSettingsDialog (신규, AnnotationPlugin 에 배치)
    - onOptionsButtonPressed() 다이얼로그에 탭 추가
    - QKeySequenceEdit 위젯으로 단축키 편집
    - 변경 시 즉시 QSettings에 저장
    - 충돌 시 경고
```

## 3. File Change List

### 3.1 신규 파일

| 파일 경로 | 설명 |
|-----------|------|
| `ASAP/interfaces/ShortcutManager.h` | 단축키 관리 유틸리티 클래스 선언 |
| `ASAP/interfaces/ShortcutManager.cpp` | 단축키 관리 유틸리티 클래스 구현 |
| `ASAP/annotation/ShortcutSettingsWidget.h` | 단축키 설정 위젯 선언 (QKeySequenceEdit 포함) |
| `ASAP/annotation/ShortcutSettingsWidget.cpp` | 단축키 설정 위젯 구현 |

### 3.2 수정 파일

| 파일 경로 | 변경 내용 |
|-----------|-----------|
| `ASAP/CMakeLists.txt` | HEADERS/SOURCE에 `interfaces/ShortcutManager.h`, `interfaces/ShortcutManager.cpp` 추가 |
| `ASAP/annotation/CMakeLists.txt` | ANNOTATION_PLUGIN_HEADERS/SOURCE에 `ShortcutSettingsWidget.h`, `ShortcutSettingsWidget.cpp` 추가 |
| `ASAP/annotation/AnnotationWorkstationExtensionPlugin.cpp` | (1) `keyPressEvent()`: 하드코딩된 `Qt::Key::Key_H/E/C`를 `ShortcutManager` 조회로 교체. (2) `eventFilter()` 트리 위젯 Delete 키: `ShortcutManager` 조회로 교체. (3) `onOptionsButtonPressed()`: `ShortcutSettingsWidget`을 다이얼로그에 통합. (4) `initialize()`: `_annotationTools` 생성 후 각 Tool의 `getToolButton()`이 호출되기 전에 단축키를 QSettings에서 읽어와 적용할 수 있는 초기화 로직 추가. |
| `ASAP/annotation/AnnotationWorkstationExtensionPlugin.h` | `#include "ShortcutManager.h"` 추가; `void applyToolShortcuts()` private 메서드 선언 |
| `ASAP/annotation/AnnotationTool.cpp` | `keyPressEvent()`: 하드코딩된 `Qt::Key_Escape`, `Qt::Key_Delete + ShiftModifier`, `Qt::Key_Delete`를 `ShortcutManager`에서 읽은 `QKeySequence`와 비교하도록 변경 |
| `ASAP/annotation/AnnotationTool.h` | `#include "ShortcutManager.h"` 추가 |
| `ASAP/annotation/DotAnnotationTool.cpp` | `getToolButton()`: `_button->setShortcut(QKeySequence("d"))`를 `ShortcutManager::getShortcut("tool_dotannotation")`로 교체 |
| `ASAP/annotation/PolyAnnotationTool.cpp` | `getToolButton()`: `_button->setShortcut(QKeySequence("p"))`를 `ShortcutManager::getShortcut("tool_polyannotation")`로 교체 |
| `ASAP/annotation/SplineAnnotationTool.cpp` | `getToolButton()`: `_button->setShortcut(QKeySequence("s"))`를 `ShortcutManager::getShortcut("tool_splineannotation")`로 교체 |
| `ASAP/annotation/RectangleAnnotationTool.cpp` | `getToolButton()`: `_button->setShortcut(QKeySequence("r"))`를 `ShortcutManager::getShortcut("tool_rectangleannotation")`로 교체 |
| `ASAP/annotation/MeasurementAnnotationTool.cpp` | `getToolButton()`: `_button->setShortcut(QKeySequence("m"))`를 `ShortcutManager::getShortcut("tool_measurementannotation")`로 교체 |
| `ASAP/annotation/PointSetAnnotationTool.cpp` | `getToolButton()`: `_button->setShortcut(QKeySequence("i"))`를 `ShortcutManager::getShortcut("tool_pointsetannotation")`로 교체 |
| `ASAP/basictools/ZoomTool.cpp` | `getToolButton()`: `_button->setShortcut(QKeySequence("z"))`를 `ShortcutManager::getShortcut("tool_zoom")`로 교체 |
| `ASAP/basictools/PanTool.cpp` | `getToolButton()`: `_button->setShortcut(QKeySequence("x"))`를 `ShortcutManager::getShortcut("tool_pan")`로 교체 |
| `ASAP/ASAP_Window.cpp` | (1) `retranslateUi()`: `actionOpen->setShortcut("Ctrl+O")` 및 `actionClose->setShortcut("Ctrl+C")`를 `ShortcutManager` 조회로 교체. (2) `keyPressEvent()`: `Qt::Key::Key_F1`을 `ShortcutManager` 조회로 교체. |
| `ASAP/ASAP_Window.h` | `#include "interfaces/ShortcutManager.h"` 추가 |

## 4. Shortcut Key Registry

QSettings에 저장될 단축키 식별자와 기본값:

```
[Shortcuts]
# Category 1: Annotation Plugin (keyPressEvent - 즉시 적용)
annotation_toggle_visibility=H
annotation_delete_selected=E
annotation_change_color=C

# Category 2: Annotation Tool (keyPressEvent - 즉시 적용)
annotation_cancel=Esc
annotation_remove_last_point=Del
annotation_cancel_entire=Shift+Del

# Category 3: Tool Switching (QAction shortcut - 다음 시작 시 적용)
tool_dotannotation=D
tool_polyannotation=P
tool_splineannotation=S
tool_rectangleannotation=R
tool_measurementannotation=M
tool_pointsetannotation=I
tool_zoom=Z
tool_pan=X

# Category 4: Window
window_open_file=Ctrl+O
window_close_file=Ctrl+C
window_show_shortcuts=F1

# Category 5: Tree Widget (eventFilter - 즉시 적용)
tree_delete_item=Del
```

**참고**: `annotation_remove_last_point` (Category 2)와 `tree_delete_item` (Category 5)은 기본값이 모두 `Del`이다. 이는 이미 현재 코드에서 동일한 키가 서로 다른 컨텍스트(활성 툴 vs 트리 위젯 포커스)에서 사용되고 있으므로 충돌이 아니다. 키 이벤트는 포커스 체인을 따라 전달되므로 컨텍스트에 따라 올바른 핸들러가 처리한다.

## 5. Implementation Phases

### Phase 1: ShortcutManager 유틸리티 (의존성: 없음)

**산출물**: `ASAP/interfaces/ShortcutManager.h`, `ASAP/interfaces/ShortcutManager.cpp`

**상세 설계**:

```cpp
// ShortcutManager.h
class ShortcutManager {
public:
    // 단축키 ID로 QSettings에서 QKeySequence 읽기. 없으면 defaultSeq 반환.
    static QKeySequence getShortcut(const QString& id, const QString& defaultSeq);

    // 단축키 ID로 QSettings에 QKeySequence 저장.
    static void setShortcut(const QString& id, const QKeySequence& seq);

    // 주어진 단축키가 다른 ID에서 이미 사용 중인지 확인.
    // 충돌하는 ID 목록 반환. 빈 목록이면 충돌 없음.
    static QStringList findConflicts(const QString& excludeId, const QKeySequence& seq);

    // 모든 단축키 ID와 현재 설정값/기본값 반환 (설정 다이얼로그용).
    struct ShortcutEntry {
        QString id;
        QString displayName;
        QKeySequence currentSeq;
        QKeySequence defaultSeq;
    };
    static QList<ShortcutEntry> getAllShortcuts();

    // 단축키를 기본값으로 리셋.
    static void resetToDefaults();
};
```

**QSettings 키 구조**: `Shortcuts/<id>` (예: `Shortcuts/tool_zoom`)

**검증 방법**: ShortcutManager 단위 테스트. QSettings에 값이 없을 때 기본값 반환, 값이 있을 때 저장된 값 반환, 충돌 검출 동작 확인.

### Phase 2: ASAP_Window 및 basictools 플러그인에 ShortcutManager 적용 (의존성: Phase 1)

**산출물**: 수정된 `ASAP_Window.cpp`, `ZoomTool.cpp`, `PanTool.cpp`

**변경 상세**:

1. **ASAP_Window.cpp**:
   - `retranslateUi()`에서 `actionOpen->setShortcut(...)` / `actionClose->setShortcut(...)`를 `ShortcutManager::getShortcut()` 호출로 교체
   - `keyPressEvent()`에서 `event->key() == Qt::Key::Key_F1`을 `event->matches(QKeySequence(ShortcutManager::getShortcut("window_show_shortcuts", "F1")))` 형태로 교체
   - QAction 단축키는 설정 변경 후 다음 시작 시 적용 (QAction은 한 번 설정되면 유지되므로)

2. **ZoomTool.cpp / PanTool.cpp**:
   - `getToolButton()`에서 `setShortcut(QKeySequence("z"))` / `setShortcut(QKeySequence("x"))`를 `ShortcutManager::getShortcut("tool_zoom", "Z")` / `ShortcutManager::getShortcut("tool_pan", "X")`로 교체

**검증 방법**: ASAP 실행 후 Z/X 키로 Zoom/Pan 툴 전환, Ctrl+O로 파일 열기 동작 확인. QSettings에 다른 값을 저장하고 재시작 후 변경된 단축키 동작 확인.

### Phase 3: AnnotationTool 및 Annotation Plugin에 ShortcutManager 적용 (의존성: Phase 1)

**산출물**: 수정된 `AnnotationTool.cpp`, `AnnotationWorkstationExtensionPlugin.cpp`, 모든 AnnotationTool 하위 클래스의 `getToolButton()`

**변경 상세**:

1. **AnnotationTool.cpp keyPressEvent()**:
   - 현재 `Qt::Key::Key_Escape`, `Qt::Key::Key_Delete` 비교를 `QKeySequence` 기반 비교로 변경
   - `keyPressEvent`에서 `event->key()` 단일 비교 대신 `matchesKeySequence(event, sequence)` 헬퍼 사용
   - 구현 방식: `QKeySequence`의 `count()`가 1이면 단일 키이므로 `[0]`에서 `Qt::Key`와 modifier를 추출하여 비교

   ```cpp
   // 핵심 비교 로직 (의사코드)
   bool matchesKeySequence(QKeyEvent* event, const QKeySequence& seq) {
       if (seq.count() != 1) return false;
       Qt::Key key = Qt::Key(seq[0] & ~Qt::KeyboardModifierMask);
       Qt::KeyboardModifiers mods = Qt::KeyboardModifiers(seq[0] & Qt::KeyboardModifierMask);
       return event->key() == key && event->modifiers() == mods;
   }
   ```

2. **AnnotationWorkstationExtensionPlugin.cpp keyPressEvent()**:
   - `Qt::Key::Key_H/E/C` 비교를 `matchesKeySequence(event, ShortcutManager::getShortcut("annotation_toggle_visibility", "H"))` 형태로 교체
   - `eventFilter()`의 트리 위젯 Delete 키: `matchesKeySequence(kpEvent, ShortcutManager::getShortcut("tree_delete_item", "Del"))`로 교체

3. **AnnotationTool 하위 클래스 getToolButton()**:
   - 각 Tool의 `getToolButton()`에서 하드코딩된 단축키를 `ShortcutManager::getShortcut()` 호출로 교체

**검증 방법**: H 키로 어노테이션 표시/숨김, E 키로 삭제, C 키로 색상 변경 동작 확인. Escape/Delete/Shift+Delete가 AnnotationTool에서 올바르게 동작하는지 확인.

### Phase 4: ShortcutSettingsWidget UI 구현 (의존성: Phase 1, Phase 2, Phase 3)

**산출물**: `ASAP/annotation/ShortcutSettingsWidget.h`, `ASAP/annotation/ShortcutSettingsWidget.cpp`

**상세 설계**:

```
+-----------------------------------------------------------+
| Set options for annotation tools                     [X]   |
+-----------------------------------------------------------+
| [General] [Shortcuts]                                     |
|                                                           |
|  Shortcuts tab:                                           |
|  +-------------------------------------------------------+|
|  | Action                    | Shortcut                  ||
|  |---------------------------|---------------------------||
|  | Toggle annotations        | [ QKeySequenceEdit: H   ] ||
|  | Delete selected           | [ QKeySequenceEdit: E   ] ||
|  | Change color              | [ QKeySequenceEdit: C   ] ||
|  | Cancel annotation         | [ QKeySequenceEdit: Esc ] ||
|  | Remove last point         | [ QKeySequenceEdit: Del ] ||
|  | Cancel entire annotation  | [ QKeySequenceEdit: S.. ] ||
|  | Dot Annotation Tool       | [ QKeySequenceEdit: D   ] ||
|  | Poly Annotation Tool      | [ QKeySequenceEdit: P   ] ||
|  | ...                         ...                       ||
|  | Open File                 | [ QKeySequenceEdit: C.. ] ||
|  | Close File                | [ QKeySequenceEdit: C.. ] ||
|  | Show Shortcuts            | [ QKeySequenceEdit: F1  ] ||
|  +-------------------------------------------------------+|
|  [Reset to Defaults]                                      |
|                                                           |
|                              [Cancel]  [Ok]               |
+-----------------------------------------------------------+
```

**구현 요구사항**:
- `QTabWidget`을 사용하여 기존 옵션(General)과 새 단축키 탭(Shortcuts)을 분리
- `ShortcutSettingsWidget`은 `QScrollArea` 내에 `QFormLayout` 기반으로 각 단축키별 `QKeySequenceEdit` 위젯 배치
- 각 `QKeySequenceEdit`의 `keySequenceChanged` 시그널에서 `ShortcutManager::findConflicts()` 호출
- 충돌 발견 시 해당 행의 배경색을 노란색으로 변경하고 툴팁으로 충돌 정보 표시
- "Reset to Defaults" 버튼으로 모든 단축키를 기본값으로 복원
- "Ok" 클릭 시 QSettings에 저장, "Cancel" 클릭 시 변경 취소

**AnnotationWorkstationExtensionPlugin::onOptionsButtonPressed() 통합**:
- 기존 `QDialog`를 `QTabWidget`으로 감싸기
- 첫 번째 탭: 기존 General 옵션 (선택 감도, 색상, 단순화)
- 두 번째 탭: `ShortcutSettingsWidget`

**검증 방법**: 옵션 다이얼로그 열기, 단축키 탭 확인, QKeySequenceEdit으로 단축키 변경, 충돌 경고 표시 확인, Ok 후 QSettings에 저장 확인, ASAP 재시작 후 변경된 단축키 동작 확인.

### Phase 5: 즉시 적용 로직 구현 (의존성: Phase 4)

**산출물**: 설정 다이얼로그에서 변경된 단축키의 즉시/지연 적용 로직

**상세 설계**:

단축키 적용 시점은 두 가지 카테고리로 구분된다:

1. **keyPressEvent 기반 (즉시 적용 가능)**: Category 1, 2, 4(F1), 5
   - 이 핫키들은 `keyPressEvent`에서 QSettings를 실시간으로 읽지 않고, 초기화 시 캐시된 `QKeySequence`와 비교한다.
   - 설정 다이얼로그에서 "Ok"를 누르면 `ShortcutManager` 캐시를 갱신하는 것이 아니라, QSettings에 저장하고 다음 keyPressEvent 호출 시 QSettings에서 다시 읽도록 한다.
   - **구현 방식**: `keyPressEvent`에서 매번 `ShortcutManager::getShortcut()`을 호출하면 QSettings 디스크 읽기 오버헤드가 발생하므로, `ShortcutManager` 내부에 `QHash<QString, QKeySequence>` 캐시를 유지하고 `setShortcut()` 시 캐시를 갱신하는 방식을 사용한다.

2. **QAction 기반 (다음 시작 시 적용)**: Category 3, 4(Ctrl+O, Ctrl+C)
   - `getToolButton()`은 최초 1회만 `QAction`을 생성하고 `_button`에 저장한다. 이후 호출은 캐시된 `_button`을 반환한다.
   - QAction의 shortcut은 런타임에 `setShortcut()`으로 변경할 수 있지만, 설정 다이얼로그에서 변경 시 모든 Tool의 QAction을 찾아 업데이트해야 한다.
   - **구현 방식**: `ShortcutSettingsWidget`에서 "Ok"를 누르면, 저장된 변경 사항 중 QAction 기반 단축키에 대해 `applyToolShortcuts()`를 호출하여 활성화된 모든 Tool의 QAction shortcut을 갱신한다. `AnnotationWorkstationExtensionPlugin`은 `_annotationTools` 벡터를 순회하며 각 Tool의 `_button` shortcut을 재설정한다. ZoomTool/PanTool은 별도 플러그인이므로 `PathologyViewer`를 통해 접근하거나, ASAP_Window에서 `findChildren<QAction*>()`으로 모든 Tool QAction을 찾아 갱신한다.

**검증 방법**:
- 설정 다이얼로그에서 H 키를 다른 키(예: T)로 변경 후 Ok 클릭
- T 키로 어노테이션 표시/숨김 전환 확인 (즉시 적용)
- 설정 다이얼로그에서 D 키(Dot Annotation)를 F로 변경 후 Ok 클릭
- F 키로 Dot Annotation Tool 전환 확인 (QAction 즉시 갱신)
- ASAP 재시작 후에도 변경된 단축키 유지 확인

## 6. Implementation Order (Dependency Graph)

```
Phase 1: ShortcutManager
    |
    +---> Phase 2: ASAP_Window + basictools (병렬)
    |
    +---> Phase 3: AnnotationTool + Annotation Plugin (병렬)
    |
    +---> Phase 4: ShortcutSettingsWidget UI (Phase 2, 3 완료 후)
              |
              +---> Phase 5: Immediate Apply Logic
```

Phase 2와 Phase 3은 Phase 1에만 의존하므로 병렬 개발 가능하다.

## 7. Test Plan

### Phase 1 테스트: ShortcutManager 단위 테스트

| 테스트 케이스 | 기대 결과 |
|---------------|-----------|
| `getShortcut("tool_zoom", "Z")` - QSettings에 값 없음 | `QKeySequence("Z")` 반환 |
| `setShortcut("tool_zoom", QKeySequence("F"))` 후 `getShortcut("tool_zoom", "Z")` | `QKeySequence("F")` 반환 |
| `findConflicts("tool_zoom", QKeySequence("P"))` - tool_polyannotation이 P 사용 중 | `["tool_polyannotation"]` 반환 |
| `findConflicts("tool_zoom", QKeySequence("Z"))` - 자기 자신 | 빈 목록 반환 (자기 자신은 충돌 아님) |
| `getAllShortcuts()` | 18개 항목 리스트 반환, 각 항목에 id/displayName/currentSeq/defaultSeq 포함 |
| `resetToDefaults()` 후 `getShortcut("tool_zoom", "Z")` | 기본값 "Z" 반환 |

### Phase 2 테스트: ASAP_Window + basictools 통합 테스트

| 테스트 케이스 | 기대 결과 |
|---------------|-----------|
| ASAP 실행 후 Z 키 입력 | Zoom Tool 활성화 |
| ASAP 실행 후 X 키 입력 | Pan Tool 활성화 |
| QSettings에 `Shortcuts/tool_zoom=F` 저장 후 재시작, F 키 입력 | Zoom Tool 활성화 |
| Ctrl+O 입력 | 파일 열기 다이얼로그 표시 |
| F1 키 입력 | 단축키 개요 출력 (qDebug) |

### Phase 3 테스트: Annotation Plugin 통합 테스트

| 테스트 케이스 | 기대 결과 |
|---------------|-----------|
| H 키 입력 | 어노테이션 표시/숨김 전환 |
| E 키 입력 (선택된 어노테이션 있을 때) | 선택된 어노테이션 삭제 |
| C 키 입력 (선택된 어노테이션 있을 때) | 색상 선택 다이얼로그 표시 |
| Escape 키 입력 (어노테이션 생성 중) | 어노테이션 취소 |
| Delete 키 입력 (어노테이션 생성 중, 점 2개 이상) | 마지막 점 제거 |
| Shift+Delete 키 입력 (어노테이션 생성 중) | 어노테이션 전체 취소 |
| Delete 키 입력 (트리 위젯 포커스, 항목 선택됨) | 트리 항목 삭제 |
| D/P/S/R/M/I 키 입력 | 해당 Annotation Tool 전환 |

### Phase 4 테스트: ShortcutSettingsWidget UI 테스트

| 테스트 케이스 | 기대 결과 |
|---------------|-----------|
| 옵션 버튼 클릭 | 탭 다이얼로그에 General/Shortcuts 탭 표시 |
| Shortcuts 탭 | 18개 단축키 항목 표시, 각각 QKeySequenceEdit 위젯 |
| QKeySequenceEdit 클릭 후 새 키 입력 | 위젯에 새 키 시퀀스 표시 |
| 충돌하는 키 설정 (예: Zoom을 P로) | 충돌 경고 표시 (PolyAnnotationTool과 충돌) |
| "Reset to Defaults" 클릭 | 모든 단축키가 기본값으로 복원 |
| "Ok" 클릭 후 QSettings 확인 | 변경된 단축키가 QSettings에 저장됨 |
| "Cancel" 클릭 | QSettings 변경 없음 |

### Phase 5 테스트: 즉시 적용 테스트

| 테스트 케이스 | 기대 결과 |
|---------------|-----------|
| 설정에서 H를 T로 변경 후 Ok | 즉시 T 키로 어노테이션 토글 동작 |
| 설정에서 D를 F로 변경 후 Ok | 즉시 F 키로 Dot Annotation Tool 전환 |
| 설정에서 Ctrl+O를 Ctrl+Shift+O로 변경 후 Ok | 즉시 Ctrl+Shift+O로 파일 열기 동작 |
| ASAP 재시작 | 모든 변경된 단축키가 유지됨 |

## 8. Risks and Mitigations

### Risk 1: keyPressEvent와 QAction 단축키의 이벤트 충돌

**문제**: QAction에 설정된 단축키(예: D)와 keyPressEvent에서 처리하는 단축키(예: H)가 서로 다른 경로로 처리된다. 만약 사용자가 QAction 단축키(D)를 H로 변경하면, keyPressEvent의 H 처리와 충돌할 수 있다.

**완화**: ShortcutManager의 `findConflicts()`가 모든 카테고리의 단축키를 교차 검사한다. 설정 다이얼로그에서 충돌 시 경고를 표시하고, 사용자가 충돌을 무시하고 저장할 수는 있지만 권장하지 않는다.

### Risk 2: Delete 키의 이중 사용

**문제**: Delete 키가 AnnotationTool(keyPressEvent)과 Tree Widget(eventFilter) 두 곳에서 사용된다. 현재는 포커스 기반으로 자연스럽게 분리되지만, 사용자가 한쪽만 다른 키로 변경하면 혼란이 발생할 수 있다.

**완화**: 기본값을 동일하게 유지하고, 설정 UI에서 두 단축키를 서로 다른 카테고리로 표시하여 사용자가 컨텍스트를 이해할 수 있도록 한다.

### Risk 3: ZoomTool/PanTool은 독립 플러그인

**문제**: ZoomTool과 PanTool은 ASAP_Window에서 `QPluginLoader`로 로드되는 독립 공유 라이브러리이다. 이들은 `_annotationPlugin`에 접근할 수 없으며, 설정 다이얼로그에서 단축키를 변경할 때 이 플러그인들의 QAction을 어떻게 갱신할지가 문제이다.

**완화**: ShortcutManager는 ASAPLib(공유 라이브러리)에 위치하므로 ZoomTool/PanTool에서 링크 가능하다. QAction 갱신은 ASAP_Window에서 `findChildren<QAction*>()`으로 모든 Tool 버튼을 순회하며 `objectName()`을 기반으로 `ShortcutManager`에서 새 단축키를 읽어 `setShortcut()`을 호출하는 방식으로 처리한다.

### Risk 4: QKeySequence 문자열 파싱

**문제**: `QKeySequence("Del")` vs `Qt::Key_Delete` 비교. `QKeyEvent::key()`는 `int`를 반환하고 `QKeySequence`는 `QString` 기반이므로, keyPressEvent에서의 비교 로직이 정확해야 한다.

**완화**: Phase 3에서 `matchesKeySequence(QKeyEvent*, const QKeySequence&)` 헬퍼 함수를 구현하여 이 변환을 캡슐화한다. `QKeySequence`의 `operator[]`를 사용하여 `Qt::Key`와 modifier를 정확히 추출한다.

### Risk 5: QKeySequenceEdit 위젯의 플랫폼별 동작 차이

**문제**: Qt6의 `QKeySequenceEdit`는 플랫폼에 따라 단일 키 입력 vs 복합 키 입력 처리가 다를 수 있다. 특히 단일 문자 키(D, H 등)는 `QKeySequenceEdit`에서 `Tab`, `Enter` 등과 충돌할 수 있다.

**완화**: Qt6에서 `QKeySequenceEdit`는 표준 위젯으로 단일 키 시퀀스를 잘 지원한다. 다만 설정 UI에서는 사용자에게 단일 키 할당 시 주의를 안내한다. 필요시 `QKeySequenceEdit` 대신 커스텀 위젯을 고려할 수 있으나, Phase 4에서 먼저 `QKeySequenceEdit`로 구현하고 사용성을 평가한다.

## 9. CMake Changes Summary

### ASAP/CMakeLists.txt
```cmake
set(HEADERS
    ...
    interfaces/ShortcutManager.h    # 추가
)
set(SOURCE
    ...
    interfaces/ShortcutManager.cpp  # 추가
)
```

### ASAP/annotation/CMakeLists.txt
```cmake
SET(ANNOTATION_PLUGIN_HEADERS
    ...
    ShortcutSettingsWidget.h        # 추가
)
SET(ANNOTATION_PLUGIN_SOURCE
    ...
    ShortcutSettingsWidget.cpp      # 추가
)
```

## 10. QSettings Storage Format

INI 파일 위치: `~/.config/DIAG/ASAP.conf` (Linux), `%APPDATA%/DIAG/ASAP.ini` (Windows)

```ini
[Shortcuts]
annotation_toggle_visibility=H
annotation_delete_selected=E
annotation_change_color=C
annotation_cancel=Esc
annotation_remove_last_point=Del
annotation_cancel_entire=Shift+Del
tool_dotannotation=D
tool_polyannotation=P
tool_splineannotation=S
tool_rectangleannotation=R
tool_measurementannotation=M
tool_pointsetannotation=I
tool_zoom=Z
tool_pan=X
window_open_file=Ctrl+O
window_close_file=Ctrl+C
window_show_shortcuts=F1
tree_delete_item=Del
```

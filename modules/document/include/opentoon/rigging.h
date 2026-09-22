#pragma once
#include "opentoon/document.h"

namespace opentoon {
// These operations mutate only a candidate Document owned by Session::apply.
Id makeCharacter(Document&, Id drawingLayer, std::string name);
[[nodiscard]] Id characterFor(const Document&, Id layer);
Id duplicateCharacter(Document&, Id character, double offsetX = 64, double offsetY = 64);
void attachDrawingAsPart(Document&, Id drawingLayer, Id parent, std::string role);
Id addPeg(Document&, Id child, std::string name);
void setPartRole(Document&, Id part, std::string role);
void reparentPreservingWorld(Document&, Id child, Id newParent);
void setPivotPreservingArtwork(Document&, Id layer, double x, double y);
Id createSubstitution(Document&, Id part, Frame frame, bool duplicateCurrent, std::string name);
void renameSubstitution(Document&, Id part, Id drawing, std::string name);
void selectSubstitution(Document&, Id part, Frame frame, Id drawing);
void removeSubstitution(Document&, Id part, Id drawing);
void reorderSubstitution(Document&, Id part, Id drawing, int direction);
Id stepSubstitution(Document&, Id part, Frame frame, int direction);
Id captureCharacterView(Document&, Id character, Frame frame, std::string name);
void applyCharacterView(Document&, Id character, Id view, Frame frame);
void updateCharacterView(Document&, Id character, Id view, Frame frame);
void renameCharacterView(Document&, Id character, Id view, std::string name);
Id duplicateCharacterView(Document&, Id character, Id view);
void removeCharacterView(Document&, Id character, Id view);
} // namespace opentoon

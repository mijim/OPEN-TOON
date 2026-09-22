#pragma once
#include "opentoon/document.h"

namespace opentoon {
// These operations mutate only a candidate Document owned by Session::apply.
Id makeCharacter(Document&, Id drawingLayer, std::string name);
void attachDrawingAsPart(Document&, Id drawingLayer, Id parent, std::string role);
Id addPeg(Document&, Id child, std::string name);
void setPartRole(Document&, Id part, std::string role);
void reparentPreservingWorld(Document&, Id child, Id newParent);
void setPivotPreservingArtwork(Document&, Id layer, double x, double y);
Id createSubstitution(Document&, Id part, Frame frame, bool duplicateCurrent, std::string name);
void renameSubstitution(Document&, Id part, Id drawing, std::string name);
void selectSubstitution(Document&, Id part, Frame frame, Id drawing);
void removeSubstitution(Document&, Id part, Id drawing);
} // namespace opentoon

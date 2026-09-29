#pragma once
#include "opentoon/document.h"

namespace opentoon {
// All mutations operate on a Session::apply candidate and remain undoable.
void validateMeshBinding(const Document&, const Layer&, const MeshBinding&);
[[nodiscard]] const MeshBinding* meshBindingFor(const Layer&, Id drawing);
void bindRegularImageMesh(Document&, Id part, Id drawing, int columns, int rows);
void bindRegularVectorMesh(Document&, Id part, Id drawing, int columns, int rows);
void moveMeshRestVertex(Document&, Id part, Id drawing, std::size_t vertex,
                        MeshPoint position);
void moveMeshPoseVertex(Document&, Id part, Id drawing, std::size_t vertex,
                        MeshPoint position);
void resetMeshPose(Document&, Id part, Id drawing);
void removeMeshBinding(Document&, Id part, Id drawing);
} // namespace opentoon

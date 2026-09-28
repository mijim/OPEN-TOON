#pragma once
#include "opentoon/document.h"

namespace opentoon {
void validateMeshDeformer(const Document&, const MeshBinding&);
void bindBoneChain(Document&, Id part, Id drawing, std::array<MeshPoint, 3> restJoints,
                   double elbowTransition);
void moveBoneRestJoint(Document&, Id part, Id drawing, int joint, MeshPoint position);
void setBoneElbowTransition(Document&, Id part, Id drawing, double radius);
void recordBonePose(Document&, Id part, Id drawing, Frame frame, double shoulderAngle,
                    double elbowAngle, Interpolation interpolation = Interpolation::Linear);
void bindCurveDeformer(Document&, Id part, Id drawing,
                       std::array<MeshPoint, 4> restControls);
void moveCurveRestControl(Document&, Id part, Id drawing, int control, MeshPoint position);
void recordCurvePose(Document&, Id part, Id drawing, Frame frame,
                     std::array<MeshPoint, 4> controls,
                     Interpolation interpolation = Interpolation::Linear);
void removeMeshDeformer(Document&, Id part, Id drawing);
[[nodiscard]] bool hasDeformerKeys(const Layer&, Frame start, Frame end);
[[nodiscard]] std::array<double, 2> sampleBoneAngles(const BoneChain&, Frame frame);
[[nodiscard]] std::array<MeshPoint, 4> sampleCurveControls(const CurveDeformer&, Frame frame);
[[nodiscard]] MeshBinding evaluateMeshBinding(const MeshBinding&, Frame frame);
} // namespace opentoon

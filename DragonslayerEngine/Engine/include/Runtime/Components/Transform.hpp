#pragma once

#include "ECS/Component.hpp"

#include "Math/Mat4.hpp"
#include "Math/Quat.hpp"
#include "Math/Vec3.hpp"

// TODO Create standalone transform math type and make this component use that instead!

DSTRUCT(Transform, .isSerializable = true, .hasMetadata = true, .isDisplayable = true) final {

    GENERATE_DSTRUCT_BODY(Transform)

    // TODO Rotation should come first larger size!

    Vec3 location = Vec3::ZERO;
    Vec3 scale = Vec3::ONE;
    Quat rotation = Quat::IDENTITY;

    //explicit Transform(const PxTransform& pxTransform) : location(ToDS(pxTransform.p)), rotation(ToDS(pxTransform.q)) {}
    // TODO Implement bunch of constructors so this type can be more easily created

    /*Transform& operator=(const physx::PxTransform& pxTransform) {
        location = ToDS(pxTransform.p);
        rotation = ToDS(pxTransform.q);
        return *this;
    }*/

    NO_DISCARD FORCE_INLINE Mat4 ToMat4() const {
        return Mat4::Translation(location) * rotation.ToRotationMatrix() * Mat4::Scaling(scale.Abs());
    }

    NO_DISCARD FORCE_INLINE Mat4 ToMat4NoScale() const {
        return Mat4::Translation(location) * rotation.ToRotationMatrix();
    }

    NO_DISCARD FORCE_INLINE Vec3 GetUpAxis() const {
        return rotation.ToRotationMatrix().GetUpAxis();
    }

    NO_DISCARD FORCE_INLINE Vec3 GetRightAxis() const {
        return rotation.ToRotationMatrix().GetRightAxis();
    }

    NO_DISCARD FORCE_INLINE Vec3 GetForwardAxis() const {
        return rotation.ToRotationMatrix().GetForwardAxis();
    }

    NO_DISCARD FORCE_INLINE bool HasUniformScale() const {
        return fabsf(scale.x - scale.y) < FLT_EPSILON && fabsf(scale.y - scale.z) < FLT_EPSILON;
    }

    void Serialize(VaultFile& file) const {
        file.Write(location.x);
        file.Write(location.y);
        file.Write(location.z);
        file.Write(scale.x);
        file.Write(scale.y);
        file.Write(scale.z);
        file.Write(rotation.t);
        file.Write(rotation.x);
        file.Write(rotation.y);
        file.Write(rotation.z);
    }

    static void Deserialize(void* obj, VaultFile& file) {
        new (obj) Transform();
        Transform& transform = *static_cast<Transform*>(obj);
        file.Read(transform.location.x);
        file.Read(transform.location.y);
        file.Read(transform.location.z);
        file.Read(transform.scale.x);
        file.Read(transform.scale.y);
        file.Read(transform.scale.z);
        file.Read(transform.rotation.t);
        file.Read(transform.rotation.x);
        file.Read(transform.rotation.y);
        file.Read(transform.rotation.z);
    }

#if WITH_EDITOR
    void OnHierarchy() {
        ImGui::InputVec3("Location", location);
        ImGui::InputVec3("Scale", scale);
        ImGui::InputQuat("Rotation", rotation);
    }
#endif

}; END_DSTRUCT(Transform)

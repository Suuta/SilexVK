
#pragma once

#include "Core/Core.h"


namespace Silex
{
    enum class CameraMovementDir
    {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down,
    };

    class Camera
    {
    public:

        Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f));

        void Update(float deltaTime);
        void SetViewportSize(uint32 width, uint32 height);

        glm::mat4 GetViewMatrix()       const { return View;        }
        glm::mat4 GetProjectionMatrix() const { return Projection;  }
        glm::vec3 GetPosition()         const { return Position;    }
        glm::vec3 GetFront()            const { return Front;       }
        glm::vec2 GetViewportSize()     const { return ViwpoerSize; }

        void SetPosition(glm::vec3 position) { Position = position; }

        float GetNearPlane() const { return NearPlane; }
        float GetFarPlane()  const { return FarPlane;  }
        float GetFOV()       const { return FOV;       }

        void Move(CameraMovementDir direction, float deltaTime);
        void ProcessMouseMovement(float xoffset, float yoffset);
        void ProcessMouseScroll(float offset);

    private:

        void UpdateCameraAxisVectors();

    private:

        glm::mat4 View;
        glm::mat4 Projection;

        glm::vec2 ViwpoerSize = { 1280.f, 720.f };
        glm::vec3 Position;
        glm::vec3 Front;
        glm::vec3 Up;
        glm::vec3 Right;
        glm::vec3 WorldUp;

        float NearPlane = 0.1f;    // これ以上下げない（精度の問題か、Gridの不具合あり）
        float FarPlane  = 1000.0f;
        float Yaw;
        float Pitch;

        float ScrollValue = 0.0f;

        float MovementSpeed;
        float MouseSensitivity;
        float FOV;
    };

}